// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftReward.h"
#include "TwoLeftPlayer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ATwoLeftReward::ATwoLeftReward()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(60.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = CollisionSphere;

	RewardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RewardMesh"));
	RewardMesh->SetupAttachment(RootComponent);
	RewardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATwoLeftReward::OnOverlapBegin);
}

// Called when the game starts or when spawned
void ATwoLeftReward::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATwoLeftReward::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		if (ATwoLeftPlayer* Player = Cast<ATwoLeftPlayer>(OtherActor))
		{
			if (!Player->bIsDead)
			{
				Player->ApplyReward(RewardType, RewardAmount);
				Destroy();
			}
		}
	}
}
