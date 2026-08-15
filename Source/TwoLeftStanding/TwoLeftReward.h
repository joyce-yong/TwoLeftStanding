// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwoLeftReward.generated.h"

UENUM(BlueprintType)
enum class ERewardType : uint8
{
	Health,
	DamageBoost,
	SpeedBoost
};

UCLASS()
class TWOLEFTSTANDING_API ATwoLeftReward : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATwoLeftReward();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* RewardMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	ERewardType RewardType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	float RewardAmount = 25.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
