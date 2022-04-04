#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/Actor.h"
#include "turbineActor.generated.h"

UCLASS()
class TURBINEPROJECT_API AturbineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AturbineActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintProtected),  Category = "Turbine")
	void SetwindDirection(float WindDirectionDegrees);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintProtected),  Category = "Turbine")
	void SetWindSpeed(float wsmps);
	
	UFUNCTION(BlueprintPure, Category = "Turbine")
	float GetTurbineElectricalCurrent();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* TurbineSkeletalMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	UPoseableMeshComponent* PoseMesh;

	UPROPERTY(VisibleDefaultsOnly, Category=Animation)
	UAnimSequence* AnimationSequence;

	UPROPERTY(VisibleDefaultsOnly, Category=Collision)
	UCapsuleComponent* CollisionCapsule;
	
private:

	// Choose Features
	// Choose BoneDetailsAndAnimation or ProgramaticallyMoveActor
	const bool BoneDetailsAndAnimationOnly = false;
	const bool ProgramaticallyMoveActor = true;

	const bool SmoothingFilterOnBlades = true;
	const bool DebugTurbine = false;

	// Constants
	const FName WindDirectionBone = "DirectionBone";
	const FName WindSpeedBone = "BladesBone";
	
	const FString TurbinePath = TEXT("/Game/Turbine/Meshes/TurbineModel.TurbineModel");
	const FString AnimationPath = TEXT("/Game/Turbine/Animations/TurbineModel_Anim.TurbineModel_Anim");
	const FString MeshName = TEXT("TurbineMesh");
	
	// Force 9 Wind 24.4 meter per second
	const float MaxWindSpeed = 25;
	const float RotationFor1MPS = 60;
	const float MaxBladeAcceleration = 0.3;
	float LastRecordedRotationAmount = 0;
	
	// -1 means a random value will be generated
	float WindSpeedMpS = -1.0f;
	
	// Ensure MinWindDirection and MaxWindDirection stay within -180 to 180
	float WindDirectionInDegrees = 0;
	const int MaxDeflectionAngle_WindDirection = 30;
	float MinWindDirection = -30;
	float MaxWindDirection = 30;
	const float PauseNumberOfTicks = 10;
	const float DirectionOfBlades = -1;

	// 12V system. Max 483 Watts. (40.25Amps)
	// 3.33A at 5m/s
	// 24.16A at 11m/s
	// 40.25A at 15m/s (factor 2.683)
	float ElectricalCurrantInAmps = 0;
	const float WindSpeedToCurrentFactor = 2.683;

	// ToDo. Divide current by number of devices connected and current draw by each one

	// Smoothing filter / Averaging for blades.
	// Wind Strength locally defined at random.
	TQueue<float> QWindSpeedDeflectionSmoothing;
	const int NumberOfAverages = 4;

	// Smooth linear travel
	// Direction of wind locally defined at random.
	const float MaxAccelerationWindward = 0.5; // Step distance.
	float MultiplicationFactor = 0; // Ramp steepness up or down.
	int StepCount = 0;
	float TargetWindwardAngle = 0;
	float DistanceToTravel = 0;
	
	UFUNCTION()
	void PrimeFIFO ();

	UFUNCTION()
	float AverageSpeedDeflection (float input);

	UFUNCTION()
	void PointWindward();

	UFUNCTION()
	void RotateBlades(float DeltaTime);

	// Left these in un-unsed.
	// If used, they solve Angle Wrap but there are problems around 0-deg instead. 
	// Multiplication Factor and switching polarity
	UFUNCTION()
	float NormaliseTo180 (float x);

	UFUNCTION()
	float NormaliseTo360 (float x);
};