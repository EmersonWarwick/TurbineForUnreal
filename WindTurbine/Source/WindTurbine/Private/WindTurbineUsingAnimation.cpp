//Copyright 2023 Emerson Warwick Limited


#include "WindTurbineUsingAnimation.h"

// Sets default values
AWindTurbineUsingAnimation::AWindTurbineUsingAnimation()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TurbineModelBundle(*TurbinePath);
	if (!TurbineModelBundle.Succeeded()) return;

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(*MeshName);
	SkeletalMesh->SetSkeletalMeshAsset(TurbineModelBundle.Object);
	SkeletalMesh->SetVisibility(true);
	SkeletalMesh->SetupAttachment(RootComponent);
	
	// UCapsuleComponent is required to stop characters walking through.
	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	CollisionCapsule->SetCapsuleSize(70.f, 200.f, true);
	CollisionCapsule->SetVisibility(true);
	CollisionCapsule->SetCollisionProfileName("Block Pawns");
	CollisionCapsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	CollisionCapsule->SetupAttachment(SkeletalMesh);
	CollisionCapsule->AddLocalOffset(FVector(0.0f, 0.0f, -160.0f));

}

// Called when the game starts or when spawned
void AWindTurbineUsingAnimation::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AWindTurbineUsingAnimation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Ensure MinWindDirection and MaxWindDirection stay within -180 to 180
void AWindTurbineUsingAnimation::SetwindDirection(float WindDirectionDegrees)
{
	WindDirectionInDegrees = WindDirectionDegrees;
	MinWindDirection = FMath::Max(WindDirectionInDegrees - MaxDeflectionAngle_WindDirection / 2, -180.0f);
	MaxWindDirection = FMath::Min(WindDirectionInDegrees + MaxDeflectionAngle_WindDirection / 2, 180.0f);
}

void AWindTurbineUsingAnimation::SetWindSpeed(float wsmps)
{
	WindSpeedMpS = wsmps;
}

float AWindTurbineUsingAnimation::GetTurbineElectricalCurrent()
{
	return ElectricalCurrantInAmps;
}


// TO-DO Need to slowly move towards goal
void AWindTurbineUsingAnimation::PointWindward()
{
	// Do not wobble when no wind
	if (WindSpeedMpS == 0) return;

	// Cannot GetBoneRotationByName from skeletal meshs
	// Get current position
	FRotator CurrentRotation = FRotator(0.0f); //SkeletalMesh->GetBoneRotationByName(WindDirectionBone, EBoneSpaces::ComponentSpace);

	// New move
	if (StepCount <= 0)
	{
		float CurrentYaw = CurrentRotation.Yaw; // Horizontal Rotation. +/- 180

		float RandomWindAngle = FMath::RandRange(MinWindDirection, MaxWindDirection);

		// Keep direction within bounds
		TargetWindwardAngle = FMath::Max(MinWindDirection, FMath::Min(RandomWindAngle, MaxWindDirection));

		// ToDo Less 'wobble' when wind at low speeds

		// Get start at x & y = 0; Get distance between two points. Remember -- = + 
		DistanceToTravel = TargetWindwardAngle - CurrentYaw;
		// Calculate number of steps to reach new goal
		StepCount = abs(DistanceToTravel / MaxAccelerationWindward);
		// Calculate ramp up or down. div - by - = + So we made StepCount + / absolute.
		// Plus then, StepCount is ready for counting down.
		MultiplicationFactor = DistanceToTravel / StepCount;
		// Sometimes RandomWindAngle is so close to current position.
		// MultiplicationFactor) will be infinite. So we just exit the function
		if (isinf(MultiplicationFactor)) return;
	}
	else
	{
		StepCount--;
	}

	// Calculate next move
	float CalculatedYaw = MaxAccelerationWindward * MultiplicationFactor;

	// Calculate new position and Set. Keeping InPitch & InRoll the same.
	FRotator RequestedRotationPosition = CurrentRotation + FRotator(0, CalculatedYaw, 0);

	// Cannot GetBoneRotationByName from SkeletalMeshs
	// Send:
	//SkeletalMesh->SetBoneRotationByName(WindDirectionBone, RequestedRotationPosition, EBoneSpaces::ComponentSpace);

	if (DebugTurbine) // Change to true if you wish to debug this part.
	{
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : MinWindDirection %f"), MinWindDirection);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : MaxWindDirection %f"), MaxWindDirection);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : TargetWindwardAngle %f"), TargetWindwardAngle);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : DistanceToTravel %f"), DistanceToTravel);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : MultiplicationFactor %f"), MultiplicationFactor);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : StepCount %i"), StepCount);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : CalculatedYaw %f"), CalculatedYaw);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : YawSent %f"), RequestedRotationPosition.Yaw);
		UE_LOG(LogTemp, Warning, TEXT(
			"AWindTurbineUsingAnimation : --------------------"));
	}
}

