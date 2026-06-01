// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwoLeftProjectile.generated.h"

UCLASS()
class TWOLEFTSTANDING_API ATwoLeftProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATwoLeftProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Damage")
	float DamageAmount = 25.0f;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// VFX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UParticleSystem* ImpactFX;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactFX(FVector HitLocation, FRotator HitRotation);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
