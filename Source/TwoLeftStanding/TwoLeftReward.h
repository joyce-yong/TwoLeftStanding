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

	// How fast the item spins (degrees per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Effects")
	float RotationRate = 90.0f;

	// How fast the item bobs up and down
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Effects")
	float FloatSpeed = 5.0f;

	// How high the item bounces
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Effects")
	float FloatAmplitude = 10.0f;

	// Tracks time for the Sine wave math
	float RunningTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Effects")
	class USoundBase* PickupSound;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
