// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ATwoLeftProjectile::ATwoLeftProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	// If the bullet doesn't hit anything for 3s, destroy it automatically
	InitialLifeSpan = 3.0f;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionSphere->InitSphereRadius(15.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic")); // Block walls & characters
	RootComponent = CollisionSphere;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	CollisionSphere->OnComponentHit.AddDynamic(this, &ATwoLeftProjectile::OnHit);

}

// Called when the game starts or when spawned
void ATwoLeftProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Ignore Owner (Player/Turret)
	if (GetOwner() != nullptr)
	{
		CollisionSphere->IgnoreActorWhenMoving(GetOwner(), true);
	}
	
}

// Called every frame
void ATwoLeftProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATwoLeftProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Destroy bullet when it hits anything other than itself (for now)
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherActor != GetOwner()) && (OtherComp != nullptr))
	{
		// Only Server should deal damage
		if (HasAuthority())
		{
			UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, GetInstigatorController(), this, UDamageType::StaticClass());
		}

		Destroy();
	}
}
