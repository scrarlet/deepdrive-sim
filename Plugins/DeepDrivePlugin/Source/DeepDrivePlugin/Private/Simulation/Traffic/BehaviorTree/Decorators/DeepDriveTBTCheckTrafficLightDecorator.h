
#pragma once

#include "Simulation/Traffic/BehaviorTree/DeepDriveTBTDecoratorBase.h"
#include "Simulation/TrafficLight/DeepDriveTrafficLight.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDeepDriveTBTCheckTrafficLightDecorator, Log, All);

class DeepDriveTBTCheckTrafficLightDecorator	:	public	DeepDriveTBTDecoratorBase
{
public:

	DeepDriveTBTCheckTrafficLightDecorator(EDeepDriveTrafficLightPhase phase);

	virtual bool performCheck(DeepDriveTrafficBlackboard &blackboard, int32 pathPointIndex);

private:

	EDeepDriveTrafficLightPhase					m_Phase;

};
