// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwoLeftTurret.generated.h"

UENUM(BlueprintType)
enum class ETurretState : uint8
{
	Idle,
	Tracking,
	Firing
};

UCLASS()
class TWOLEFTSTANDING_API ATwoLeftTurret : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATwoLeftTurret();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Turret components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* DetectionSphere;

	// Combat properties
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<class ATwoLeftProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float RotationSpeed = 5.0f;

	// Health Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 70.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	// State & Targeting
	UPROPERTY(Replicated)
	ETurretState CurrentState = ETurretState::Idle;

	UPROPERTY(Replicated)
	class ATwoLeftPlayer* TargetPlayer;

	UPROPERTY()
	TArray<class ATwoLeftPlayer*> PlayersInRange;

	struct FTimerHandle FireTimerHandle;

	// Functions
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void CheckLineOfSight();
	void Fire();
	void FindNewTarget();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

};
