#include "turbineActor.h"

#include "CollisionAnalyzer/Public/ICollisionAnalyzer.h"
#include "Math/UnrealMathUtility.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Navigation/PathFollowingComponent.h"

AturbineActor::AturbineActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
    // Need this for creating USkeletalMeshComponent or UPoseableMeshComponent
	// Right click - Copy Reference in Unreal to get path
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TurbineSkeletalMeshBundle(*TurbinePath);
	// Don't crash the program if the USkeletalMesh is not where we think it is.
	if(!TurbineSkeletalMeshBundle.Succeeded()) return;
	
	//////
	// USkeletalMeshComponent is required if you:
	// Want to Get a list of bones or
	// Want the Actor like a dummy, to just to run an animation.
	//////
	if (BoneDetailsAndAnimationOnly)
	{
		TurbineSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(*MeshName);
		TurbineSkeletalMeshComponent->SetOnlyOwnerSee(false);
		TurbineSkeletalMeshComponent->SetSkeletalMesh(TurbineSkeletalMeshBundle.Object);
		SetRootComponent(TurbineSkeletalMeshComponent);
		//Get a list of bones in Output Log
		TArray<FName> bones;
		TurbineSkeletalMeshComponent->GetBoneNames(bones);
		for (FName Bone : bones)
		{
			UE_LOG(LogTemp, Warning, TEXT("AturbineActor BoneNames: %s"),  *Bone.ToString());
		}
	
		// This adds the animation.
		// Remember: Important line in BeginPlay() TurbineSkeletalMeshComponent->PlayAnimation(AnimationSequence, true);
		static ConstructorHelpers::FObjectFinder<UAnimSequence> AnimationSequenceBundle(*AnimationPath);
		if (!AnimationSequenceBundle.Succeeded()) return;
		// Tie in the Animation
		AnimationSequence = AnimationSequenceBundle.Object;
	}

	/////
	// UPoseableMeshComponent to move bones programatically.
	//////
	if (ProgramaticallyMoveActor)
	{
		PoseMesh = CreateDefaultSubobject<UPoseableMeshComponent>(*MeshName);
		PoseMesh->SetSkeletalMesh(TurbineSkeletalMeshBundle.Object);
		PoseMesh->SetVisibility(true);
		SetRootComponent(PoseMesh);
	}

	//////
	// UCapsuleComponent is required to stop characters walking through.
	//////
	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	CollisionCapsule->SetCapsuleSize(70.f,200.f,true);
	CollisionCapsule->SetVisibility(true);
	CollisionCapsule->SetCollisionProfileName("Block Pawns");
	CollisionCapsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	CollisionCapsule->SetupAttachment(RootComponent);
	CollisionCapsule->AddLocalOffset(FVector(0.0f,0.0f,-160.0f));
}

// Called when the game starts or when spawned
void AturbineActor::BeginPlay()
{
	Super::BeginPlay();

	if (BoneDetailsAndAnimationOnly)
	{
		// Play an animation. Looping True
		TurbineSkeletalMeshComponent->PlayAnimation(AnimationSequence, true);
	}
	
	if (ProgramaticallyMoveActor)
	{
		PrimeFIFO();
	}
}

// Called every frame
void AturbineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ProgramaticallyMoveActor)
	{
		// BlueprintSetters. Uncomment for testing
		//SetwindDirection(175.0f);
		//SetWindSpeed(1);
		PointWindward();
		RotateBlades(DeltaTime);
	}
}

// Ensure MinWindDirection and MaxWindDirection stay within -180 to 180
void AturbineActor::SetwindDirection(float WindDirectionDegrees)
{
	WindDirectionInDegrees = WindDirectionDegrees;
	MinWindDirection = FMath::Max(WindDirectionInDegrees - MaxDeflectionAngle_WindDirection / 2, -180.0f);
	MaxWindDirection = FMath::Min(WindDirectionInDegrees + MaxDeflectionAngle_WindDirection / 2, 180.0f);
}

void AturbineActor::SetWindSpeed(float wsmps)
{
	WindSpeedMpS = wsmps;
}

float AturbineActor::GetTurbineElectricalCurrent()
{
	return ElectricalCurrantInAmps;
}


// TO-DO Need to slowly move towards goal
void AturbineActor::PointWindward()
{
	// Do not wobble when no wind
	if (WindSpeedMpS == 0) return;
	
	// Get current position
	FRotator CurrentRotation = PoseMesh->GetBoneRotationByName(
		WindDirectionBone, EBoneSpaces::ComponentSpace);
	
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
	
	// Send:
	PoseMesh->SetBoneRotationByName(WindDirectionBone, RequestedRotationPosition ,EBoneSpaces::ComponentSpace);

	if (DebugTurbine) // Change to true if you wish to debug this part.
		{
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : MinWindDirection %f"), MinWindDirection);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : MaxWindDirection %f"), MaxWindDirection);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : TargetWindwardAngle %f"), TargetWindwardAngle);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : DistanceToTravel %f"), DistanceToTravel);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : MultiplicationFactor %f"), MultiplicationFactor);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : StepCount %i"), StepCount);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : CalculatedYaw %f"), CalculatedYaw);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : YawSent %f"), RequestedRotationPosition.Yaw);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : --------------------"));
		}
}

void AturbineActor::RotateBlades(float DeltaTime)
{
	
	FRotator CurrentBladesPosition = PoseMesh->GetBoneRotationByName(WindSpeedBone, EBoneSpaces::ComponentSpace);
	
	// Blades go round the stronger the wind
	// Find random value if not externally controlled
	// 
	if (WindSpeedMpS == -1.0f)
	{
		WindSpeedMpS = FMath::RandRange(0.0f,MaxWindSpeed);
	}
	else
	{
		WindSpeedMpS = FMath::Min(WindSpeedMpS ,MaxWindSpeed);
	}

	// ToDo Ramp up speed with acceleration.
	float DeflectionRequestedThisTick = WindSpeedMpS * RotationFor1MPS * DeltaTime; 

	if (SmoothingFilterOnBlades)
	{
		DeflectionRequestedThisTick = AverageSpeedDeflection(DeflectionRequestedThisTick) * DirectionOfBlades;
	}
	
	FRotator NewRotorPosition = FRotator(0, 0, DeflectionRequestedThisTick) + CurrentBladesPosition;
	PoseMesh->SetBoneRotationByName(WindSpeedBone, NewRotorPosition ,EBoneSpaces::ComponentSpace);

	// Remember for next time
	//LastRecordedRotationAmount = DeflectionRequestedThisTick;

	// Update Current Output
	// No more extra current above 15m/s wind speed
	ElectricalCurrantInAmps = FMath::Min(WindSpeedMpS, 15.0f) * WindSpeedToCurrentFactor; 

	if (DebugTurbine) // Change to true if you wish to debug this part.
		{
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : DeflectionRequestedThisTick Current %f"), CurrentBladesPosition.Roll);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : DeflectionRequestedThisTick Add %f"), DeflectionRequestedThisTick);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : DeflectionRequestedThisTick New %f"), NewRotorPosition.Roll);
		UE_LOG(LogTemp, Warning, TEXT(
			"AturbineActor : --------------------"));
		}
}

float AturbineActor::NormaliseTo180 (float x)
{
	x = fmod(x + 180,360);
	if (x < 0)
	{
		x += 360;
	}
	return x - 180;
}

float AturbineActor::NormaliseTo360 (float x)
{
	x = fmod(x,360);
	if (x < 0)
		x += 360;
	return x;
}

void AturbineActor::PrimeFIFO ()
{
	QWindSpeedDeflectionSmoothing.Empty();
	for (int i = 0; i < NumberOfAverages; i++)
	{
		QWindSpeedDeflectionSmoothing.Enqueue(0);
	}
}

float AturbineActor::AverageSpeedDeflection (float Input)
{
	if (SmoothingFilterOnBlades)
	{
		float Output = 0;
		float DequeuedFloat = 0;
		// Add new value and remove oldest value
		QWindSpeedDeflectionSmoothing.Enqueue(Input);
		QWindSpeedDeflectionSmoothing.Pop();
		// Cycle through queue
		for (int i = 0; i < NumberOfAverages; i++)
		{
			QWindSpeedDeflectionSmoothing.Dequeue(DequeuedFloat);
			Output = Output + DequeuedFloat;
			QWindSpeedDeflectionSmoothing.Enqueue(DequeuedFloat);
		}
		// Calculate Average
		Output = Output / NumberOfAverages;
		
		return Output;
	}
	return Input;
}

// Another example: Log values for development on screen. Change TEXT and variables to suit.
// GEngine->AddOnScreenDebugMessage(10,10, FColor::White, TEXT("Actioned Once"));

