// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftPlayer.h"
#include "TwoLeftProjectile.h"
#include "TwoLeftReward.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ATwoLeftPlayer::ATwoLeftPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

    // Rotate based on mouse (Yaw only)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    // Top-Down Camera
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 1200.0f;
    // Steep downward angle
    CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.0f;

    TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

    // Gun Mesh
    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    GunMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
    GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Revive Sphere
    ReviveSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ReviveSphere"));
    ReviveSphere->SetupAttachment(RootComponent);
    ReviveSphere->InitSphereRadius(200.0f);

    // Turn on when player is downed
    ReviveSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Bind Revive Overlaps
    ReviveSphere->OnComponentBeginOverlap.AddDynamic(this, &ATwoLeftPlayer::OnReviveOverlapBegin);
    ReviveSphere->OnComponentEndOverlap.AddDynamic(this, &ATwoLeftPlayer::OnReviveOverlapEnd);

	// Death Post Process
    DeathPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("DeathPostProcess"));
    DeathPostProcess->SetupAttachment(RootComponent);

    DeathPostProcess->bEnabled = false;

    // Override Saturation (black & white)
    DeathPostProcess->Settings.bOverride_ColorSaturation = true;
    DeathPostProcess->Settings.ColorSaturation = FVector4(0.0f, 0.0f, 0.0f, 1.0f);

    // Override Vignette (dark shadows around edges)
    DeathPostProcess->Settings.bOverride_VignetteIntensity = true;
    DeathPostProcess->Settings.VignetteIntensity = 1.0f;

}

// Called when the game starts or when spawned
void ATwoLeftPlayer::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = false;

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
        PC->bShowMouseCursor = true;
    }
	
}

// Called every frame
void ATwoLeftPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (IsLocallyControlled())
    {
        if (TopDownCamera && MPC_GlobalData)
        {
            FVector CamLoc = TopDownCamera->GetComponentLocation();
            FLinearColor CamLocAsColor = FLinearColor(CamLoc.X, CamLoc.Y, CamLoc.Z, 1.0f);
            UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), MPC_GlobalData, FName("CameraLocation"), CamLocAsColor);
        }

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            FHitResult Hit;
            // Cast a ray from the mouse to the world
            if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
            {
                // Calculate rotation to face the mouse hit
                FRotator LookRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Hit.ImpactPoint);
                LookRotation.Pitch = 0.f;
                LookRotation.Roll = 0.f;

                // Updates controller rotation
                PC->SetControlRotation(LookRotation);

                // Distance between player and mouse
				FVector MouseDirection = Hit.ImpactPoint - GetActorLocation();
				MouseDirection.Z = 0.0f;

				MouseDirection = MouseDirection.GetClampedToMaxSize(400.0f);

                CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, MouseDirection, DeltaTime, 3.0f);
            }
        }

		// Revive Timer Logic
        if (bIsBeingRevived && PlayerToRevive != nullptr)
        {
            ReviveProgress += DeltaTime;

            if (ReviveProgress >= 4.0f)
            {
                bIsBeingRevived = false;
                ReviveProgress = 0.0f;

                Server_CompleteRevive(PlayerToRevive);

                PlayerToRevive = nullptr;
                Server_StopReviveAnim();
            }
        }
        else
        {
            ReviveProgress = 0.0f;
        }
    }

}

// Called to bind functionality to input
void ATwoLeftPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATwoLeftPlayer::Move);
        EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ATwoLeftPlayer::Dash);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ATwoLeftPlayer::Fire);
        // Revive Input
        EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Started, this, &ATwoLeftPlayer::StartRevive);
        EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Completed, this, &ATwoLeftPlayer::StopRevive);
        EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Canceled, this, &ATwoLeftPlayer::StopRevive);
    }

}

void ATwoLeftPlayer::Move(const FInputActionValue& Value)
{
    if (bIsDead) return;

    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 8-directional movement
        AddMovementInput(FVector::ForwardVector, MovementVector.Y);
        AddMovementInput(FVector::RightVector, MovementVector.X);
    }
}

void ATwoLeftPlayer::Dash(const FInputActionValue& Value)
{
    if (bIsDead || !bCanDash) return;

    FVector DashDirection = GetCharacterMovement()->GetLastInputVector();

    if (DashDirection.IsNearlyZero())
    {
        DashDirection = GetActorForwardVector();
    }

    DashDirection.Normalize();

    // Turn off friction for the dash duration
    GetCharacterMovement()->GroundFriction = 0.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;

    LaunchCharacter(DashDirection * DashForce, true, true);

	// If we are not the host, call the server function to execute the dash on the server
    if (!HasAuthority())
    {
        Server_Dash(DashDirection);
	}

    bCanDash = false;
    GetWorldTimerManager().SetTimer(DashDurationTimer, this, &ATwoLeftPlayer::StopDash, 0.2f, false);
    GetWorldTimerManager().SetTimer(DashCooldownTimer, this, &ATwoLeftPlayer::ResetDash, DashCooldown, false);
}

void ATwoLeftPlayer::ResetDash()
{
    bCanDash = true;
}

void ATwoLeftPlayer::StopDash()
{
	// Restore friction after dash
    GetCharacterMovement()->GroundFriction = 8.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
}

void ATwoLeftPlayer::Fire(const FInputActionValue& Value)
{
    if (bIsDead || !bCanFire || bIsMenuOpen) return;

    FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 100.0f);

	FRotator BaseRotation = GetActorRotation();
	float RandomSpread = FMath::RandRange(-BulletSpread, BulletSpread);
	FRotator SpawnRotation = FRotator(BaseRotation.Pitch, BaseRotation.Yaw + RandomSpread, BaseRotation.Roll);

    Server_Fire(SpawnLocation, SpawnRotation);

    // Cooldown
    bCanFire = false;
    GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATwoLeftPlayer::ResetFire, FireRate, false);
}

void ATwoLeftPlayer::ResetFire()
{
    bCanFire = true;
}

void ATwoLeftPlayer::StartRevive(const FInputActionValue& Value)
{
    if (PlayerToRevive != nullptr && !bIsDead)
    {
        bIsBeingRevived = true;
        Server_PlayReviveAnim();
    }
}

void ATwoLeftPlayer::StopRevive(const FInputActionValue& Value)
{
    bIsBeingRevived = false;
    ReviveProgress = 0.0f;
    Server_StopReviveAnim();
}

void ATwoLeftPlayer::Server_Dash_Implementation(FVector DashDirection)
{
    GetCharacterMovement()->GroundFriction = 0.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;

    LaunchCharacter(DashDirection * DashForce, true, true);

    GetWorldTimerManager().SetTimer(DashDurationTimer, this, &ATwoLeftPlayer::StopDash, 0.2f, false);
}

void ATwoLeftPlayer::Server_Fire_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
    SpawnLocation = GunMesh->GetSocketLocation(TEXT("Muzzle"));

    if (ProjectileClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        // Spawn physical bullet
        GetWorld()->SpawnActor<ATwoLeftProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

        Multicast_PlayShootAnim();
    }
}

void ATwoLeftPlayer::Server_CompleteRevive_Implementation(ATwoLeftPlayer* TargetPlayer)
{
    if (TargetPlayer && TargetPlayer->bIsDead)
    {
        TargetPlayer->bIsDead = false;

        // 50% health
        float RevivedHealth = TargetPlayer->MaxHealth * 0.5f;
        TargetPlayer->CurrentHealth = RevivedHealth;

        TargetPlayer->OnRep_IsDead();

        // Debug
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Player Revived"));
    }
}

float ATwoLeftPlayer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (HasAuthority() && !bIsDead)
    {
        if (EventInstigator != nullptr)
        {
            if (ATwoLeftPlayer* AttackingPlayer = Cast<ATwoLeftPlayer>(EventInstigator->GetPawn()))
            {
                // Ignore damage if attacker is player
                return 0.0f;
            }
        }

        CurrentHealth -= DamageAmount;

        // Debug
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Player Health: %f"), CurrentHealth));

        if (CurrentHealth <= 0.0f)
        {
            CurrentHealth = 0.0f;
            bIsDead = true;

            OnRep_IsDead();

            // Debug
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Player is down"));
        }
    }

    return DamageAmount;
}

void ATwoLeftPlayer::ApplyReward(ERewardType RewardType, float Amount)
{
    if (!HasAuthority()) return;

    switch (RewardType)
    {
    case ERewardType::Health:
        ApplyHeal(Amount);
        break;
    case ERewardType::DamageBoost:
        ApplyDamageBoost(Amount, 5.0f);
        break;
    case ERewardType::SpeedBoost:
        ApplySpeedBoost(Amount, 5.0f);
        break;
    }
}

void ATwoLeftPlayer::ApplyHeal(float HealAmount)
{
    CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
    
    // Debug
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Healed! Current Health: %f"), CurrentHealth));
}

void ATwoLeftPlayer::ApplySpeedBoost(float Multiplier, float Duration)
{
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (!MoveComp) return;

    if (!bIsSpeedBoosted)
    {
        OriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
        bIsSpeedBoosted = true;
    }

    // Apply multiplier
    MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed * Multiplier;

    // Duration Extension Math
    float RemainingTime = GetWorldTimerManager().GetTimerRemaining(SpeedBoostTimerHandle);
    float TotalDuration = Duration + (RemainingTime > 0.0f ? RemainingTime : 0.0f);

    GetWorldTimerManager().SetTimer(SpeedBoostTimerHandle, this, &ATwoLeftPlayer::ResetSpeedBoost, TotalDuration, false);

    // Debug
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Speed Boost Applied!"));
}

void ATwoLeftPlayer::ResetSpeedBoost()
{
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed;
    }
    bIsSpeedBoosted = false;

    // Debug
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Speed Boost Expired"));
}

void ATwoLeftPlayer::ApplyDamageBoost(float Multiplier, float Duration)
{
    DamageMultiplier = Multiplier;
    bIsDamageBoosted = true;

    // Duration Extension Math
    float RemainingTime = GetWorldTimerManager().GetTimerRemaining(DamageBoostTimerHandle);
    float TotalDuration = Duration + (RemainingTime > 0.0f ? RemainingTime : 0.0f);

    GetWorldTimerManager().SetTimer(DamageBoostTimerHandle, this, &ATwoLeftPlayer::ResetDamageBoost, TotalDuration, false);

    // Debug
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Damage Boost Applied!"));
}

void ATwoLeftPlayer::ResetDamageBoost()
{
    DamageMultiplier = 1.0f;
    bIsDamageBoosted = false;

    // Debug
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Damage Boost Expired"));
}

void ATwoLeftPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATwoLeftPlayer, CurrentHealth);
    DOREPLIFETIME(ATwoLeftPlayer, bIsDead);
    DOREPLIFETIME(ATwoLeftPlayer, bIsBeingRevived);
}

void ATwoLeftPlayer::OnRep_IsDead()
{
    if (bIsDead)
    {
        if (IsLocallyControlled())
        {
            DeathPostProcess->bEnabled = true;
        }

        // Ignore everything
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

        // Keep blocking the floor
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

        ReviveSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    else
    {
        if (IsLocallyControlled())
        {
            DeathPostProcess->bEnabled = false;
        }

        GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
        ReviveSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void ATwoLeftPlayer::OnReviveOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (ATwoLeftPlayer* SurvivingPlayer = Cast<ATwoLeftPlayer>(OtherActor))
    {
        if (SurvivingPlayer != this && !SurvivingPlayer->bIsDead)
        {
            SurvivingPlayer->PlayerToRevive = this;
        }
    }
}

void ATwoLeftPlayer::OnReviveOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (ATwoLeftPlayer* SurvivingPlayer = Cast<ATwoLeftPlayer>(OtherActor))
    {
        if (SurvivingPlayer->PlayerToRevive == this)
        {
            SurvivingPlayer->PlayerToRevive = nullptr;
        }
    }
}

void ATwoLeftPlayer::Multicast_PlayShootAnim_Implementation()
{
    PlayShootAnimation();
}

void ATwoLeftPlayer::Server_PlayReviveAnim_Implementation()
{
    Multicast_PlayReviveAnim();
}

void ATwoLeftPlayer::Multicast_PlayReviveAnim_Implementation()
{
    PlayReviveAnimation();
}

void ATwoLeftPlayer::Server_StopReviveAnim_Implementation()
{
    Multicast_StopReviveAnim();
}

void ATwoLeftPlayer::Multicast_StopReviveAnim_Implementation()
{
    StopReviveAnimation();
}

void ATwoLeftPlayer::Multicast_StartDissolve_Implementation()
{
    PlayDissolveAnimation();
}
