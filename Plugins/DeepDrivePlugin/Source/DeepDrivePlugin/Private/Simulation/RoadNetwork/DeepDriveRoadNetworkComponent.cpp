

#include "DeepDrivePluginPrivatePCH.h"
#include "DeepDriveRoadNetworkComponent.h"

#include "Public/Simulation/DeepDriveSimulation.h"
#include "Public/Simulation/RoadNetwork/DeepDriveRoadLinkProxy.h"
#include "Public/Simulation/RoadNetwork/DeepDriveJunctionProxy.h"
#include "Public/Simulation/RoadNetwork/DeepDriveRoadSegmentProxy.h"
#include "Public/Simulation/RoadNetwork/DeepDriveRoadNetworkExtractor.h"
#include "Public/Simulation/RoadNetwork/DeepDriveRoute.h"
#include "Public/Simulation/Agent/DeepDriveAgent.h"
#include "Public/Simulation/Misc/DeepDriveRandomStream.h"

#include "Private/Simulation/RoadNetwork/DeepDriveRouteCalculator.h"

// Sets default values for this component's properties
UDeepDriveRoadNetworkComponent::UDeepDriveRoadNetworkComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UDeepDriveRoadNetworkComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UDeepDriveRoadNetworkComponent::Initialize(ADeepDriveSimulation &deepDriveSim)
{
	m_DeepDriveSimulation = &deepDriveSim;
	collectRoadNetwork();
}


// Called every frame
void UDeepDriveRoadNetworkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FVector UDeepDriveRoadNetworkComponent::GetRandomLocation(EDeepDriveLaneType PreferredLaneType, int32 relPos)
{
	FVector location = FVector::ZeroVector;

	TArray<uint32> keys;
	m_RoadNetwork.Links.GenerateKeyArray(keys);

	const int32 numLinks = keys.Num();
	if(numLinks > 0 && m_DeepDriveSimulation)
	{
		UDeepDriveRandomStream *randomStream = m_DeepDriveSimulation->GetRandomStream(FName("AgentPlacement"));
		SDeepDriveRoadLink &link = m_RoadNetwork.Links[ keys[randomStream->RandomIntegerInRange(0, numLinks - 1)] ];

		int32 laneInd = link.getRightMostLane(PreferredLaneType);
		if(laneInd)
		{
			SDeepDriveLane &lane = link.Lanes[laneInd];
			SDeepDriveRoadSegment &segment = m_RoadNetwork.Segments[lane.Segments[randomStream->RandomIntegerInRange(0, lane.Segments.Num() - 1)]];

			if(relPos == 0)
				location = segment.StartPoint;
			else if(relPos > 0)
				location = segment.EndPoint;
			else
			{
				if(segment.hasSpline())
				{
					location = segment.StartPoint;
				}
				else
				{
					location = FMath::Lerp(segment.StartPoint, segment.EndPoint, randomStream->RandomFloatInRange(0.0f, 1.0f));
				}
			}
		}
	}

	return location;
}

void UDeepDriveRoadNetworkComponent::PlaceAgentOnRoad(ADeepDriveAgent *Agent, const FVector &Location, bool OnClosestSegment)
{
	if(OnClosestSegment)
	{
		const uint32 segmentId = m_RoadNetwork.findClosestSegment(Location, EDeepDriveLaneType::MAJOR_LANE);
		if(segmentId)
		{
			const SDeepDriveRoadSegment *segment = &m_RoadNetwork.Segments[segmentId];
			FVector posOnSegment = segment->findClosestPoint(Location);
			FTransform transform(FRotator(0.0f, segment->getHeading(posOnSegment), 0.0f), posOnSegment, FVector(1.0f, 1.0f, 1.0f));
			Agent->SetActorTransform(transform, false, 0, ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		const uint32 linkId = m_RoadNetwork.findClosestLink(Location);
		if(linkId)
		{
			const SDeepDriveRoadLink *link = &m_RoadNetwork.Links[linkId];
			FTransform transform(FRotator(0.0f, link->Heading, 0.0f), link->StartPoint, FVector(1.0f, 1.0f, 1.0f));
			Agent->SetActorTransform(transform, false, 0, ETeleportType::TeleportPhysics);
		}
	}
}

void UDeepDriveRoadNetworkComponent::PlaceAgentOnRoadRandomly(ADeepDriveAgent *Agent)
{
}


ADeepDriveRoute* UDeepDriveRoadNetworkComponent::CalculateRoute(const FVector Start, const FVector Destination)
{
	ADeepDriveRoute *route = 0;
	const uint32 startLinkId = m_RoadNetwork.findClosestLink(Start);
	const uint32 destLinkId = m_RoadNetwork.findClosestLink(Destination);

	if(startLinkId && destLinkId)
	{
		UE_LOG(LogDeepDriveRoadNetwork, Log, TEXT("Calc route from %d(%s) to %d(%s)  %s/%s"), startLinkId, *(m_RoadNetwork.Links[startLinkId].StartPoint.ToString()), destLinkId, *(m_RoadNetwork.Links[destLinkId].EndPoint.ToString()), *(Start.ToString()), *(Destination.ToString()) );

		DeepDriveRouteCalculator routeCalculator(m_RoadNetwork);

		SDeepDriveRouteData routeData = routeCalculator.calculate(Start, Destination);

		if (routeData.Links.Num() > 0)
		{
			route = GetWorld()->SpawnActor<ADeepDriveRoute>(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), FActorSpawnParameters());

			if (route)
			{
				route->setShowRoute(ShowRoutes);
				route->initialize(m_RoadNetwork, routeData);
			}
		}

	}
	else
		UE_LOG(LogDeepDriveRoadNetwork, Log, TEXT("Calc route failed %d / %d"), startLinkId, destLinkId );

	return route;
}

ADeepDriveRoute* UDeepDriveRoadNetworkComponent::calculateRandomRoute(const FVector &Start)
{
	ADeepDriveRoute *route = 0;

	TArray<uint32> keys;
	m_RoadNetwork.Links.GenerateKeyArray(keys);

	const int32 numLinks = keys.Num();
	if (numLinks > 0 && m_DeepDriveSimulation)
	{
		UDeepDriveRandomStream *randomStream = m_DeepDriveSimulation->GetRandomStream(FName("AgentPlacement"));

		const uint32 startLinkId = m_RoadNetwork.findClosestLink(Start);

		do
		{
			uint32 destinationLinkId = 0;
			do
			{
				destinationLinkId = m_RoadNetwork.Links[keys[randomStream->RandomIntegerInRange(0, numLinks - 1)]].LinkId;
			} while (destinationLinkId == 0 || destinationLinkId == startLinkId || m_RoadNetwork.Links[destinationLinkId].FromJunctionId == 0);


			UE_LOG(LogDeepDriveRoadNetwork, Log, TEXT("Calc random route from %d to %d"), startLinkId, destinationLinkId);

			FVector destination;

			SDeepDriveRoadLink &link = m_RoadNetwork.Links[destinationLinkId];
			int32 laneInd = link.getRightMostLane(EDeepDriveLaneType::MAJOR_LANE);
			if(laneInd >= 0)
			{
				SDeepDriveLane &lane = link.Lanes[laneInd];
				SDeepDriveRoadSegment &segment = m_RoadNetwork.Segments[lane.Segments[randomStream->RandomIntegerInRange(0, lane.Segments.Num() - 1)]];

				destination = segment.getLocationOnSegment(randomStream->RandomFloatInRange(0.25f, 0.75f));
			}
			else
				destination = m_RoadNetwork.Links[destinationLinkId].EndPoint;

			DeepDriveRouteCalculator routeCalculator(m_RoadNetwork);
			SDeepDriveRouteData routeData = routeCalculator.calculate(Start, destination);

			if (routeData.Links.Num() > 2)
			{
				route = GetWorld()->SpawnActor<ADeepDriveRoute>(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), FActorSpawnParameters());

				if (route)
				{
					route->initialize(m_RoadNetwork, routeData);
				}
			}
		} while (route == 0);
	}

	return route;
}

ADeepDriveRoute* UDeepDriveRoadNetworkComponent::CalculateRoute(const TArray<uint32> &routeLinks)
{
	ADeepDriveRoute *route = 0;
	route = GetWorld()->SpawnActor<ADeepDriveRoute>(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), FActorSpawnParameters());

	if (route)
	{
		SDeepDriveRouteData routeData;
		routeData.Start = FVector::ZeroVector;
		routeData.Destination = FVector::ZeroVector;
		routeData.Links = routeLinks;

		route->initialize(m_RoadNetwork, routeData);
	}
	return route;
}

void UDeepDriveRoadNetworkComponent::collectRoadNetwork()
{
	if(m_Extractor == 0)
		m_Extractor = new DeepDriveRoadNetworkExtractor(GetWorld(), m_RoadNetwork);

	m_Extractor->extract();

	UE_LOG(LogDeepDriveRoadNetwork, Log, TEXT("Collected road network %d juntions %d links %d segments"), m_RoadNetwork.Junctions.Num(), m_RoadNetwork.Links.Num(), m_RoadNetwork.Segments.Num() );
}

uint32 UDeepDriveRoadNetworkComponent::getRoadLink(ADeepDriveRoadLinkProxy *linkProxy)
{
	return m_Extractor ? m_Extractor->getRoadLink(linkProxy) : 0;
}

float UDeepDriveRoadNetworkComponent::calcHeading(const FVector &from, const FVector &to)
{
	FVector2D dir = FVector2D(to - from);
	dir.Normalize();
	return FMath::RadiansToDegrees(FMath::Atan2(dir.Y, dir.X));

}
