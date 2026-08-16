// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftPlayerState.h"
#include "Net/UnrealNetwork.h"

ATwoLeftPlayerState::ATwoLeftPlayerState()
{
	PlayerKills = 0;
	bReplicates = true;
}

void ATwoLeftPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATwoLeftPlayerState, PlayerKills);
}

void ATwoLeftPlayerState::AddKill()
{
	if (HasAuthority())
	{
		PlayerKills++;
		OnKillAwarded();
	}
}
