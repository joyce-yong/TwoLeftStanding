// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TwoLeftPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class TWOLEFTSTANDING_API ATwoLeftPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATwoLeftPlayerState();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 PlayerKills;

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddKill();

	UFUNCTION(BlueprintImplementableEvent, Category = "Score")
	void OnKillAwarded();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
};
