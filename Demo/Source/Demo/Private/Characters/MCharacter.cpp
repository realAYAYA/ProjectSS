// Fill out your copyright notice in the Description page of Project Settings.


#include "MCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include "MCharacterDataAsset.h"
#include "MPlayerState.h"
#include "GameplayAbilitySystem/Components/MAbilitySystemComponent.h"
#include "GameplayAbilitySystem/AttributeSets/MAttributeSet.h"
#include "Components/MCharacterMovementComponent.h"

// Sets default values
AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMCharacterMovementComponent>(AMCharacter::CharacterMovementComponentName))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// Character doesnt have a rifle at start
	//bHasRifle = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -89.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));

	// Create a SpringArmComponent
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArmComponent->SetupAttachment(GetCapsuleComponent());
	SpringArmComponent->SetRelativeLocation(FVector(0.f, 0.f, 8.5f));
	SpringArmComponent->TargetArmLength = 400.0f;
	SpringArmComponent->bUsePawnControlRotation = true;
	
	// Create a CameraComponent	
	ThirdPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCameraComponent->SetupAttachment(SpringArmComponent);

	// Gameplay Ability System
	AbilitySystemComponent = CreateDefaultSubobject<UMAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMAttributeSet>(TEXT("AttributeSet"));
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxMoveSpeedAttribute()).AddUObject(this, &AMCharacter::OnMaxMovementSpeedChanged);

	
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(CharacterDataAsset))
		CharacterData = CharacterDataAsset->CharacterData;
}

void AMCharacter::SetMyName_Implementation(const FString& InName)
{
	MyName = InName;
}

const UMAttributeSet* AMCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

UAbilitySystemComponent* AMCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AMCharacter::ApplyGameplayEffectToSelf(const TSubclassOf<UGameplayEffect> Effect, const FGameplayEffectContextHandle& InEffectContext)
{
	if (!Effect.Get())
		return false;

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(Effect, 1, InEffectContext);
	if (SpecHandle.IsValid())
	{
		const FActiveGameplayEffectHandle ActiveGameplayEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		return ActiveGameplayEffectHandle.WasSuccessfullyApplied();
	}

	return false;
}

void AMCharacter::GiveAbilities()
{
	if (HasAuthority() && AbilitySystemComponent)
	{
		int32 i = 0;
		for (const auto& DefaultAbility : CharacterData.Abilities)
		{
			if (!DefaultAbility)
				continue;
			
			FGameplayAbilitySpec GameplayAbilitySpec = FGameplayAbilitySpec(DefaultAbility);
			AbilitySystemComponent->GiveAbility(GameplayAbilitySpec);

			const UGameplayAbility* Ability = NewObject<UGameplayAbility>(this, DefaultAbility);
			if (Ability && Ability->AbilityTags.Num() > 0 && Ability->AbilityTags.IsValidIndex(0))
			{
				const FGameplayTag& Tag = Ability->AbilityTags.First();
				InputSkillMap.Add(++i, Tag);
			}
				
		}
	}
}

void AMCharacter::ApplyStartupEffects()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		FGameplayEffectContextHandle EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
		EffectContextHandle.AddSourceObject(this);

		for (const auto& CharacterEffect : CharacterData.Effects)
		{
			ApplyGameplayEffectToSelf(CharacterEffect, EffectContextHandle);
		}
	}
}

void AMCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	GiveAbilities();
	ApplyStartupEffects();
}

void AMCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyStartupEffects();
}

// Called when the game starts or when spawned
void AMCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Add Input Mapping Context
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

// Called every frame
void AMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AMCharacter::TryJump);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMCharacter::MoveEnd);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMCharacter::Look);

		// 技能栏映射
		//EnhancedInputComponent->BindAction(InputAction1, ETriggerEvent::Triggered, this, &AMCharacter::TryActiveAbility1);
	}
}

void AMCharacter::Move(const FInputActionValue& Value)
{
	// 检测是否可以移动
	for (const FGameplayTag& Tag : MoveLimitTags)
	{
		if (GetAbilitySystemComponent()->HasMatchingGameplayTag(Tag))
			return;
	}
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);

		//AbilitySystemComponent->MovementInputX = MovementVector.X;
		//AbilitySystemComponent->MovementInputY = MovementVector.Y;
		AbilitySystemComponent->Move();

		OnMoveInput.Broadcast(MovementVector.X, MovementVector.Y);
	}
}

void AMCharacter::MoveEnd(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		// add movement
		AbilitySystemComponent->MoveEnd();
		
		OnMoveInput.Broadcast(0, 0);
	}
}

void AMCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

		//AbilitySystemComponent->LookInputYaw = LookAxisVector.X;
		//AbilitySystemComponent->LookInputPitch = LookAxisVector.Y;

		OnLookInput.Broadcast(LookAxisVector.X, LookAxisVector.Y);
	}
}

void AMCharacter::TryJump(const FInputActionValue& Value)
{
	AbilitySystemComponent->Jump();

	OnJumpInput.Broadcast(Value.Get<float>());
}

void AMCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	
	AbilitySystemComponent->JumpEnd();
	
	OnJumpInput.Broadcast(0.0f);
}

void AMCharacter::TryActiveAbility(const FInputActionValue& Value)
{
	// Todo 按键映射机制
}

void AMCharacter::OnMaxMovementSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

AMCharacter* AMCharacter::GetCurrentTarget() const
{
	return CurrentTarget;
}

void AMCharacter::SetCurrentTarget_Implementation(AMCharacter* NewTarget)
{
	CurrentTarget = NewTarget;
}

void AMCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMCharacter, CharacterData);
	DOREPLIFETIME(AMCharacter, CurrentTarget);
	DOREPLIFETIME(AMCharacter, MyName);
}
