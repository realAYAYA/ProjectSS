#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	None		UMETA(Displayname = "None"),
	Warrior		UMETA(Displayname = "Warruir"),
	Mage		UMETA(Displayname = "Mage"),
};

UENUM(BlueprintType)
enum class ECamp : uint8
{
	None		UMETA(Displayname = "None"),
	Alliance 	UMETA(Displayname = "联盟"),
	Horde		UMETA(Displayname = "部落"),
};

UENUM(BlueprintType)
enum class EItemState : uint8
{
	None		UMETA(Displayname = "None"),
	Dropped		UMETA(Displayname = "Dropped"),
	Equipped	UMETA(Displayname = "Equipped"),
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Unknown				UMETA(Displayname = "Unknown"),
	
	Weapon				UMETA(Displayname = "MainWeapon"),
	TacticalEquipment	UMETA(Displayname = "TacticalEquipment"),
	Armor				UMETA(Displayname = "Armor"),
	MechPart			UMETA(Displayname = "MechPart"),

	WeaponPart			UMETA(Displayname = "WeaponPart"),
	Mission				UMETA(Displayname = "Mission"),
	
	Medicine			UMETA(Displayname = "Medicine"),
	
};

UENUM(BlueprintType)
enum class EProjectileType : uint8
{
	Unknown				UMETA(Displayname = "Unknown"),
};

UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Unknown				UMETA(Displayname = "Unknown"),
};