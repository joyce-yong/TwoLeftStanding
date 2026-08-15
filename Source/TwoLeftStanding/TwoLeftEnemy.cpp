// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftEnemy.h"
#include "TwoLeftPlayerState.h"

// Sets default values
ATwoLeftEnemy::ATwoLeftEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	ACharacter::SetReplicateMovement(true);

}

// Called when the game starts or when spawned
void ATwoLeftEnemy::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	
}

// Called every frame
void ATwoLeftEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ATwoLeftEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float ATwoLeftEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Only Server is allowed to calculate health math
	if (HasAuthority())
	{
		CurrentHealth -= DamageAmount;

		// Debug
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Enemy Health: %f"), CurrentHealth));

		if (CurrentHealth <= 0.0f)
		{
			// Add kill to the player who killed this enemy
			if (EventInstigator != nullptr)
			{
				if (ATwoLeftPlayerState* PS = EventInstigator->GetPlayerState<ATwoLeftPlayerState>())
				{
					PS->AddKill();
				}
			}
			Destroy();
		}
	}
	return DamageAmount;
}

