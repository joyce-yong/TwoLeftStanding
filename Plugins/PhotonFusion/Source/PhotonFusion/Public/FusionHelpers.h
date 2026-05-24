// Copyright 2026 Exit Games GmbH. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fusion/Aliases.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "FusionHelpers.generated.h"

namespace FusionCore
{
	struct Data;
}

class UFusionFunctionDescriptor;
class FJsonObject;

UENUM(BlueprintType)
enum class EFusionRPCTarget : uint8
{
	SendToAllClients = 0 UMETA(DisplayName = "Send To All Clients"),
	SendToObjectOwner = 1 UMETA(DisplayName = "Send To Owner"),
	SendToMasterClient = 2 UMETA(DisplayName = "Send To Master Client"),
	SendToEveryoneElse = 3 UMETA(DisplayName = "Send To Everyone Else")
};

UENUM()
enum class ERPCMode : uint8
{
	FusionRPC,
	UnrealRPC,
};

USTRUCT(BlueprintType)
struct FDefaultComponentInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Fusion")
	FName VariableName;

	UPROPERTY(BlueprintReadOnly, Category = "Fusion")
	FString TemplateName;

	UPROPERTY(BlueprintReadOnly, Category = "Fusion")
	TSubclassOf<UActorComponent> ComponentClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "Fusion")
	TObjectPtr<UActorComponent> ComponentTemplate = nullptr;
};

UENUM(BlueprintType)
enum class EFusionInstanceType : uint8
{
	Unknown = 0 UMETA(DisplayName = "Unknown"),
	Game  = 1 UMETA(DisplayName = "Game"),	
	Editor  = 2 UMETA(DisplayName = "Editor"),
	PieConsole1  = 3 UMETA(DisplayName = "PieConsole1"),
	PieConsole2  = 4 UMETA(DisplayName = "PieConsole2"),
	PieConsole3  = 5 UMETA(DisplayName = "PieConsole3"),
	PieConsole4 = 6 UMETA(DisplayName = "PieConsole4")
};



USTRUCT()
struct FKeyObjectId
{
	GENERATED_BODY()

	uint64 Packed{ 0 };

	FKeyObjectId() = default;

	FKeyObjectId(const FusionCore::PlayerId OriginValue, const FusionCore::Map MapValue, const uint32 CounterValue)
		: Packed(static_cast<uint64>(OriginValue)
		       | (static_cast<uint64>(MapValue)     << 16)
		       | (static_cast<uint64>(CounterValue) << 32))
	{}

	// ReSharper disable once CppNonExplicitConvertingConstructor
	FKeyObjectId(const FusionCore::ObjectId& ObjectId)
		: Packed(static_cast<uint64>(ObjectId)) {}

	explicit FKeyObjectId(const uint64& packed) : Packed(packed) {}

	// ReSharper disable once CppNonExplicitConversionOperator
	operator FusionCore::ObjectId() const
	{
		return FusionCore::ObjectId(static_cast<uint64_t>(Packed));
	}

	FKeyObjectId& operator=(const FusionCore::ObjectId& Other)
	{
		Packed = static_cast<uint64>(Other);
		return *this;
	}

	bool IsNone() const { return Packed == 0; }
	bool IsSome() const { return Packed != 0; }

	bool operator==(const FKeyObjectId& Other) const { return Packed == Other.Packed; }
	bool operator!=(const FKeyObjectId& Other) const { return Packed != Other.Packed; }

	// ReSharper disable once CppNonExplicitConversionOperator
	operator uint64() const { return Packed; }
};

FORCEINLINE uint32 GetTypeHash(const FKeyObjectId& InId)
{
	return GetTypeHash(InId.Packed);
}

/**
 * 
 */
UCLASS()
class PHOTONFUSION_API UFusionHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Fusion")
	static void FusionClientTravel(const UObject* WorldContextObject, FName LevelName, bool bAbsolute, FString Options);
	
	UFUNCTION(BlueprintCallable, Category = "Fusion")
	static void InvokeCustomRPC(UObject* Source, FString EventName, int32 RPCId, EFusionRPCTarget Target, const TArray<uint8>& Buffer);

	UFUNCTION(BlueprintCallable, Category = "Fusion")
	static UFusionFunctionDescriptor* GetFunctionDescriptor(UObject* Source, FString EventName);
	
	UFUNCTION(BlueprintCallable, Category = "Fusion", CustomThunk, meta=(CustomStructureParam="Value"))
	static void AddParamToBuffer(const int32& Value, UObject* Source, UFusionFunctionDescriptor* Descriptor, int32 PropertyIndex, UPARAM(ref) TArray<uint8>& Buffer);
	
	DECLARE_FUNCTION(execAddParamToBuffer);

	UFUNCTION(BlueprintCallable, Category="Fusion")
	static TArray<FDefaultComponentInfo> GetDefaultOwnerComponents(UClass* Class);

	static FString GetTypesHeader(const class UFusionClient* Client, TArray<struct FTypeData>& TypesData);
	
	static int32 PieConsoleGameIndex();
	static bool IsPieConsoleGame();

	UFUNCTION(BlueprintPure, Category = "Fusion", meta = (DisplayName="Get PIE Instance Id"))
	static FGuid GetPIEInstanceId();

	UFUNCTION(BlueprintPure, Category = "Fusion", meta = (DisplayName="Get PIE Console Game Index"))
	static EFusionInstanceType GetInstanceType();
	
	static UWorld* GetWorldByName(const FString& WorldName);

	static bool IsAllowedWorldInstance(UFusionClient* Client, UWorld* World);
	static bool IsAllowedWorldContext(UFusionClient* Client, const FWorldContext& WorldContext);
	
	static EFusionInstanceType WorldContextType(const FWorldContext& WorldContext);

	static FGuid InstanceId;

	static uint32 SafeObjectNameHash(const UObject* Object);
	static uint32 SafeObjectNameHash(ANSICHAR* String, uint32 Length);

	UFUNCTION(BlueprintCallable, Category = "Fusion", Meta=(DefaultToSelf="Actor", DeterminesOutputType="ComponentClass"))
	static UActorComponent* AddActorComponent(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass);

	UFUNCTION(BlueprintCallable, Category = "Fusion")
	static bool DestroyActorComponent(UActorComponent* Component);

	static TSharedPtr<FJsonObject> DeserializeMapPayload(const FString PayloadString);
};


