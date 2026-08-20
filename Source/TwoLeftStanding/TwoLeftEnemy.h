// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TwoLeftEnemy.generated.h"

UCLASS()
class TWOLEFTSTANDING_API ATwoLeftEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATwoLeftEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Health Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Stats")
	float MaxHealth = 100.0f;

	// Trigger OnRep_CurrentHealth on clients whenever CurrentHealth changes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "Stats")
	float CurrentHealth;

	// Called on Clients automatically when CurrentHealth replicates
	UFUNCTION()
	void OnRep_CurrentHealth();

	// Blueprint Event you can call/implement in Blueprints to pass values to your WBP_EnemyHealthBar
	UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
	void OnHealthChanged(float NewCurrentHealth, float NewMaxHealth);

	// Drop Reward
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TSubclassOf<class ATwoLeftReward> RewardClassToDrop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 0.15f;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Required to register replicated variables
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};