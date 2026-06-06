// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftPlayer.h"
#include "TwoLeftProjectile.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
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

    TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

    // Revive Sphere
    ReviveSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ReviveSphere"));
    ReviveSphere->SetupAttachment(RootComponent);
    ReviveSphere->InitSphereRadius(150.0f);

    // Turn on when player is downed
    ReviveSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Bind Revive Overlaps
    ReviveSphere->OnComponentBeginOverlap.AddDynamic(this, &ATwoLeftPlayer::OnReviveOverlapBegin);
    ReviveSphere->OnComponentEndOverlap.AddDynamic(this, &ATwoLeftPlayer::OnReviveOverlapEnd);

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

    LaunchCharacter(DashDirection * DashForce, true, true);

	// If we are not the host, call the server function to execute the dash on the server
    if (!HasAuthority())
    {
        Server_Dash(DashDirection);
	}

    bCanDash = false;
    GetWorldTimerManager().SetTimer(DashCooldownTimer, this, &ATwoLeftPlayer::ResetDash, DashCooldown, false);
}

void ATwoLeftPlayer::ResetDash()
{
    bCanDash = true;
}

void ATwoLeftPlayer::Fire(const FInputActionValue& Value)
{
    if (bIsDead || !bCanFire) return;

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
    }
}

void ATwoLeftPlayer::StopRevive(const FInputActionValue& Value)
{
    bIsBeingRevived = false;
    ReviveProgress = 0.0f;
}

void ATwoLeftPlayer::Server_Dash_Implementation(FVector DashDirection)
{
    LaunchCharacter(DashDirection * DashForce, true, true);
}

void ATwoLeftPlayer::Server_Fire_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
    if (ProjectileClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        // Spawn physical bullet
        GetWorld()->SpawnActor<ATwoLeftProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
    }
}

void ATwoLeftPlayer::Server_CompleteRevive_Implementation(ATwoLeftPlayer* TargetPlayer)
{
    if (TargetPlayer && TargetPlayer->bIsDead)
    {
        TargetPlayer->bIsDead = false;

        // 50% health
        TargetPlayer->CurrentHealth = TargetPlayer->MaxHealth * 0.5f;

        TargetPlayer->OnRep_IsDead();

        // Debug
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Player Revived"));
    }
}

float ATwoLeftPlayer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (HasAuthority() && !bIsDead)
    {
        CurrentHealth -= DamageAmount;

        // Debug
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Player Health: %f"), CurrentHealth));

        if (CurrentHealth <= 0.0f)
        {
            CurrentHealth = 0.0f;
            bIsDead = true;

            OnRep_IsDead();

            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Player is down"));
        }
    }

    return DamageAmount;
}

void ATwoLeftPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATwoLeftPlayer, CurrentHealth);
    DOREPLIFETIME(ATwoLeftPlayer, bIsDead);
}

void ATwoLeftPlayer::OnRep_IsDead()
{
    if (bIsDead)
    {
        // Ignore everything
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

        // Keep blocking the floor
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

        ReviveSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    else
    {
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

            // (Later, we will trigger the "Press E" UI here)
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

            // (Later, we will hide the "Press E" UI here)
        }
    }
}
