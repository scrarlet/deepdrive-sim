
#include "Simulation/Traffic/BehaviorTree/Creators/DeepDriveJunctionRightOfWayBTCreator.h"

#include "Private/Simulation/Traffic/Path/DeepDrivePathDefines.h"

#include "Simulation/Traffic/BehaviorTree/DeepDriveTrafficBehaviorTree.h"
#include "Simulation/Traffic/BehaviorTree/DeepDriveTrafficBehaviorTreeNode.h"

#include "Simulation/Traffic/BehaviorTree/Tasks/DeepDriveTBTStopAtLocationTask.h"
#include "Simulation/Traffic/BehaviorTree/Decorators/DeepDriveTBTCheckOncomingTrafficDecorator.h"

#include "Simulation/Traffic/BehaviorTree/DeepDriveBehaviorTreeFactory.h"

bool DeepDriveJunctionRightOfWayBTCreator::s_isRegistered = DeepDriveJunctionRightOfWayBTCreator::registerCreator();

bool DeepDriveJunctionRightOfWayBTCreator::registerCreator()
{
	DeepDriveBehaviorTreeFactory &factory = DeepDriveBehaviorTreeFactory::GetInstance();
	factory.registerCreator("four_way_row", &DeepDriveJunctionRightOfWayBTCreator::createBehaviorTree);
	return true;
}

DeepDriveTrafficBehaviorTree* DeepDriveJunctionRightOfWayBTCreator::createBehaviorTree()
{
	DeepDriveTrafficBehaviorTree *behaviorTree = new DeepDriveTrafficBehaviorTree();
	if (behaviorTree)
	{
		DeepDriveTrafficBehaviorTreeNode *stopAtNode = behaviorTree->createNode(0);
		stopAtNode->addDecorator( new DeepDriveTBTCheckOncomingTrafficDecorator("WaitingLocation") );
		stopAtNode->addTask(new DeepDriveTBTStopAtLocationTask("WaitingLocation", 0.25f, true));
	}

	return behaviorTree;
}
