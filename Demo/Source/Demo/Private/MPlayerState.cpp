// Fill out your copyright notice in the Description page of Project Settings.


#include "MPlayerState.h"

#include "Net/UnrealNetwork.h"

void AMPlayerState::Req_Implementation()
{
	const int32 DataFromServer = FMath::RandRange(0, 100);
	Ack(DataFromServer);
}

void AMPlayerState::Ack_Implementation(const int32 InData)
{
	// print(InData);
}

void AMPlayerState::SetCharacterName_Implementation(const FString& InName)
{
	MyName = InName;
}

void AMPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AMPlayerState, MyName);
}