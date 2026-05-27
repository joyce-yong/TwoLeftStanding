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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Camera Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* TopDownCamera;

	// Enhanced Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* FireAction;

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

	// Functions
	void Move(const FInputActionValue& Value);

	void Dash(const FInputActionValue& Value);
	void ResetDash();

	void Fire(const FInputActionValue& Value);
	void ResetFire();

	UFUNCTION(Server, Reliable)
	void Server_Dash(FVector DashDirection);

	UFUNCTION(Server, Reliable)
	void Server_Fire(FVector SpawnLocation, FRotator SpawnRotation);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
