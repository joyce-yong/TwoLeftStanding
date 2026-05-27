// Fill out your copyright notice in the Description page of Project Settings.


#include "TwoLeftPlayer.h"
#include "TwoLeftProjectile.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"

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

}

// Called when the game starts or when spawned
void ATwoLeftPlayer::BeginPlay()
{
	Super::BeginPlay();

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
    }

}

void ATwoLeftPlayer::Move(const FInputActionValue& Value)
{
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
    if (!bCanDash) return;

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
    if (!bCanFire) return;

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
