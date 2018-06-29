

#pragma once

#include "Simulation/Agent/DeepDriveAgentControllerBase.h"
#include "DeepDriveAgentManualController.generated.h"

class ADeepDriveSplineTrack;

USTRUCT(BlueprintType)
struct FDeepDriveManualControllerConfiguration
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Control)
	FString		ConfigurationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Control)
	ADeepDriveSplineTrack	*Track;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Control)
	TArray<float>	StartDistances;

};


/**
 * 
 */
UCLASS()
class DEEPDRIVEPLUGIN_API ADeepDriveAgentManualController : public ADeepDriveAgentControllerBase
{
	GENERATED_BODY()
	
public:

	ADeepDriveAgentManualController();

	virtual bool Activate(ADeepDriveAgent &agent);

	virtual void MoveForward(float axisValue);

	virtual void MoveRight(float axisValue);

	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void Configure(const FDeepDriveManualControllerConfiguration &Configuration, int32 StartPositionSlot, ADeepDriveSimulation* DeepDriveSimulation);

private:

	ADeepDriveSplineTrack						*m_Track = 0;
	float										m_StartDistance = 0.0f;

};