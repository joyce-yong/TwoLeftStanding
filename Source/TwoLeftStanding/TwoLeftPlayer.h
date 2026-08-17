// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TwoLeftReward.h"
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materials")
	class UMaterialParameterCollection* MPC_GlobalData;

	// Gun Components
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gun")
	class UStaticMeshComponent* GunMesh;

	// Revive Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revive")
	class USphereComponent* ReviveSphere;

	// Death Visuals
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	class UPostProcessComponent* DeathPostProcess;

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
	struct FTimerHandle DashDurationTimer;

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

	// UI properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool bIsMenuOpen = false;

	// Reward properties
	bool bIsDamageBoosted = false;
	float DamageMultiplier = 1.0f;

	bool bIsSpeedBoosted = false;
	float OriginalMaxWalkSpeed = 600.0f;

	// Functions
	void Move(const FInputActionValue& Value); 

	void Dash(const FInputActionValue& Value);
	void StopDash();
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

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayShootAnim();

	UFUNCTION(Server, Reliable)
	void Server_PlayReviveAnim();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayReviveAnim();

	UFUNCTION(Server, Reliable)
	void Server_StopReviveAnim();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopReviveAnim();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Animations")
	void Multicast_StartDissolve();

	UFUNCTION(BlueprintImplementableEvent, Category = "Animations")
	void PlayShootAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Animations")
	void PlayReviveAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Animations")
	void StopReviveAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Animations")
	void PlayDissolveAnimation();

	void ApplyReward(ERewardType RewardType, float Amount);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Healing Logic
	void ApplyHeal(float HealAmount);

	// Speed Boost Logic
	FTimerHandle SpeedBoostTimerHandle;
	void ApplySpeedBoost(float Multiplier, float Duration = 5.0f);
	void ResetSpeedBoost();

	// Damage Boost Logic
	FTimerHandle DamageBoostTimerHandle;
	void ApplyDamageBoost(float Multiplier, float Duration = 5.0f);
	void ResetDamageBoost();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
