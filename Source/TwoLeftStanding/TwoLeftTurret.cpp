// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftTurret.h"
#include "TwoLeftPlayer.h"
#include "TwoLeftProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ATwoLeftTurret::ATwoLeftTurret()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(BaseMesh);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(BaseMesh);
	DetectionSphere->InitSphereRadius(600.0f);

	// Bind Overlaps
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATwoLeftTurret::OnOverlapBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ATwoLeftTurret::OnOverlapEnd);

}

// Called when the game starts or when spawned
void ATwoLeftTurret::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	
}

void ATwoLeftTurret::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATwoLeftTurret, TargetPlayer);
	DOREPLIFETIME(ATwoLeftTurret, CurrentState);
    DOREPLIFETIME(ATwoLeftTurret, CurrentHealth);
}

// Called every frame
void ATwoLeftTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetPlayer != nullptr)
	{
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(HeadMesh->GetComponentLocation(), TargetPlayer->GetActorLocation());

		LookAtRotation.Pitch = 0.0f;
		LookAtRotation.Roll = 0.0f;

		FRotator SmoothRotation = FMath::RInterpTo(HeadMesh->GetComponentRotation(), LookAtRotation, DeltaTime, RotationSpeed);
		HeadMesh->SetWorldRotation(SmoothRotation);
	}

}

void ATwoLeftTurret::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (HasAuthority())
    {
        if (ATwoLeftPlayer* Player = Cast<ATwoLeftPlayer>(OtherActor))
        {
			PlayersInRange.AddUnique(Player);

            if (TargetPlayer == nullptr)
            {
				FindNewTarget();
            }
        }
    }
}

void ATwoLeftTurret::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (HasAuthority())
    {
        if (ATwoLeftPlayer* Player = Cast<ATwoLeftPlayer>(OtherActor))
        {
            PlayersInRange.Remove(Player);

            if (Player == TargetPlayer)
            {
                TargetPlayer = nullptr;
                FindNewTarget();
            }
        }
    }
}

void ATwoLeftTurret::FindNewTarget()
{
    for (int i = PlayersInRange.Num() - 1; i >= 0; i--)
    {
        if (PlayersInRange[i] == nullptr || PlayersInRange[i]->IsActorBeingDestroyed())
        {
            PlayersInRange.RemoveAt(i);
        }
    }

    if (PlayersInRange.Num() > 0)
    {
        TargetPlayer = PlayersInRange[0];
        CurrentState = ETurretState::Tracking;

        if (!GetWorldTimerManager().IsTimerActive(FireTimerHandle))
        {
            GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATwoLeftTurret::CheckLineOfSight, FireRate, true, 0.5f);
        }
    }
    else
    {
        TargetPlayer = nullptr;
        CurrentState = ETurretState::Idle;
        GetWorldTimerManager().ClearTimer(FireTimerHandle);
    }
}

void ATwoLeftTurret::CheckLineOfSight()
{
    if (TargetPlayer == nullptr) return;

    // In a real game, you'd do a LineTrace (Raycast) here to make sure a wall isn't blocking them.
    // For now, if they are in the circle, we fire!
    CurrentState = ETurretState::Firing;
    Fire();
}

void ATwoLeftTurret::Fire()
{
    if (ProjectileClass)
    {
        // Spawn bullet
        FVector SpawnLocation = HeadMesh->GetSocketLocation(FName("Muzzle"));
        FRotator SpawnRotation = HeadMesh->GetSocketRotation(FName("Muzzle"));

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        GetWorld()->SpawnActor<ATwoLeftProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
    }
}

float ATwoLeftTurret::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (HasAuthority() && CurrentHealth > 0.0f)
    {
        CurrentHealth -= DamageAmount;

        // Debug
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Turret Health: %f"), CurrentHealth));

        if (CurrentHealth <= 0.0f)
        {
            Destroy();
        }
    }

    return DamageAmount;
}

