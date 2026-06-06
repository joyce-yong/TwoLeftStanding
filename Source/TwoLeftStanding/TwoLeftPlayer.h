// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TwoLeftPlayer.generated.h"

UCLASS()
class TWOLEFTSTANDING_API ATwoLeftPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATwoLeftPlayer();

	// Camera Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* TopDownCamera;

	// Revive Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revive")
	class USphereComponent* ReviveSphere;

	// Enhanced Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* ReviveAction;

	// Dash properties
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashForce = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashCooldown = 1.0f;

	bool bCanDash = true;
	struct FTimerHandle DashCooldownTimer;

	// Combat properties
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<class ATwoLeftProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRate = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float BulletSpread = 3.0f;

	bool bCanFire = true;
	struct FTimerHandle FireTimerHandle;

	// Health properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	bool bIsDead;

	// Revive properties
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Revive")
	bool bIsBeingRevived;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revive")
	float ReviveProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revive")
	class ATwoLeftPlayer* PlayerToRevive;

	// Functions
	void Move(const FInputActionValue& Value); 

	void Dash(const FInputActionValue& Value);
	void ResetDash();

	void Fire(const FInputActionValue& Value);
	void ResetFire();

	void StartRevive(const FInputActionValue& Value);
	void StopRevive(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void Server_Dash(FVector DashDirection);

	UFUNCTION(Server, Reliable)
	void Server_Fire(FVector SpawnLocation, FRotator SpawnRotation);

	UFUNCTION(Server, Reliable)
	void Server_CompleteRevive(class ATwoLeftPlayer* TargetPlayer);

	UFUNCTION()
	void OnRep_IsDead();

	UFUNCTION()
	void OnReviveOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnReviveOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
