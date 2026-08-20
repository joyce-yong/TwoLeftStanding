// Fill out your copyright notice in the Description page of Project Settings.

#include "TwoLeftEnemy.h"
#include "TwoLeftPlayerState.h"
#include "TwoLeftReward.h"
#include "Net/UnrealNetwork.h" // Required for DOREPLIFETIME
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

// Sets default values
ATwoLeftEnemy::ATwoLeftEnemy()
{
	// Set this character to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	ACharacter::SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void ATwoLeftEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CurrentHealth = MaxHealth;
	}

	OnHealthChanged(CurrentHealth, MaxHealth);
}

// Register variables for Replication
void ATwoLeftEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATwoLeftEnemy, MaxHealth);
	DOREPLIFETIME(ATwoLeftEnemy, CurrentHealth);
}

// Called on CLIENTS when CurrentHealth updates
void ATwoLeftEnemy::OnRep_CurrentHealth()
{
	OnHealthChanged(CurrentHealth, MaxHealth);
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
		CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

		// Debug
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Enemy Health: %f"), CurrentHealth));

		// OnRep functions do NOT automatically run on the Server, 
		// so we call the BP Event manually for the host view.
		OnHealthChanged(CurrentHealth, MaxHealth);

		if (CurrentHealth <= 0.0f)
		{
			// Reward drop chance
			if (RewardClassToDrop != nullptr)
			{
				float RandomRoll = FMath::FRand();

				if (RandomRoll <= DropChance)
				{
					FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

					GetWorld()->SpawnActor<ATwoLeftReward>(RewardClassToDrop, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
				}
			}

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

void ATwoLeftEnemy::PerformMeleeAttack(FName SocketName, float AttackRadius, float DamageAmount)
{
	// Server handles damage math and hit registration in multiplayer
	if (!HasAuthority()) return;

	// Default hit location in front of actor if no socket is provided
	FVector TraceLocation = GetActorLocation() + (GetActorForwardVector() * 100.0f);

	if (SocketName != NAME_None && GetMesh() && GetMesh()->DoesSocketExist(SocketName))
	{
		TraceLocation = GetMesh()->GetSocketLocation(SocketName);
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Don't hit self

	TArray<FHitResult> HitResults;

	// Perform Multi Sphere Sweep to detect any Pawns (Players)
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceLocation,
		TraceLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(AttackRadius),
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor != this)
			{
				// Safely apply damage passing Controller (EventInstigator) and Enemy (DamageCauser)
				UGameplayStatics::ApplyDamage(
					HitActor,
					DamageAmount,
					GetController(),
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}