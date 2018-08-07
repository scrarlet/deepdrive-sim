
#include "DeepDrivePluginPrivatePCH.h"
#include "Public/Simulation/DeepDriveSimulation.h"
#include "Public/Simulation/Agent/DeepDriveAgent.h"
#include "Public/Capture/CaptureCameraComponent.h"
#include "Components/SplineComponent.h"
#include "Simulation/Agent/DeepDriveAgentControllerBase.h"

#include "WheeledVehicleMovementComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetRenderingLibrary.h"

DEFINE_LOG_CATEGORY(LogDeepDriveAgent);

int32 ADeepDriveAgent::s_nextAgentId = 1;

ADeepDriveAgent::ADeepDriveAgent()
{
	m_AgentId = s_nextAgentId++;

	ChaseCameraStick = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseCameraStick"));
	ChaseCameraStick->SetupAttachment(GetRootComponent());

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(ChaseCameraStick);

	InteriorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InteriorCamera"));
	InteriorCamera->SetupAttachment(GetRootComponent());

	OrbitCameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("OrbitCameraArm"));
	OrbitCameraArm->SetupAttachment(GetRootComponent());

	OrbitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("OrbitCamera"));
	OrbitCamera->SetupAttachment(OrbitCameraArm);
}

void ADeepDriveAgent::BeginPlay()
{
	Super::BeginPlay();

	FVector origin;
	GetActorBounds(true, origin, m_Dimensions);

	m_prevVelocity = m_AngularVelocity = m_prevAngularVelocity = FVector(0.0f, 0.0f, 0.0f);
}

void ADeepDriveAgent::Tick( float DeltaSeconds )
{
	Super::Tick(DeltaSeconds);

	FVector curVelocity = GetVelocity();
	m_Acceleration = (curVelocity - m_prevVelocity) / DeltaSeconds;
	m_prevVelocity = curVelocity;

	if(GetMesh())
	{
		m_AngularVelocity = GetMesh()->GetPhysicsAngularVelocityInDegrees();
		m_AngularAcceleration = (m_AngularVelocity - m_prevVelocity) / DeltaSeconds;
		m_prevAngularVelocity = m_AngularAcceleration;
	}

	if(m_AgentController == 0)
		m_AgentController = Cast<ADeepDriveAgentControllerBase>(GetController());

	if(m_AgentController && m_AgentController->updateAgentOnTrack())
	{
		++m_NumberOfLaps;
		UE_LOG(LogDeepDriveAgent, Log, TEXT("Laps finished %d"), m_NumberOfLaps );
	}
}

int32 ADeepDriveAgent::RegisterCaptureCamera(float fieldOfView, int32 captureWidth, int32 captureHeight, FVector relativePosition, FVector relativeRotation, const FString &label)
{
	int32 camId = 0;

	UTextureRenderTarget2D *colorTexture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), captureWidth, captureHeight, ETextureRenderTargetFormat::RTF_RGBA16f);
	UTextureRenderTarget2D *depthTexture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), captureWidth, captureHeight, ETextureRenderTargetFormat::RTF_R16f);

	if(colorTexture && depthTexture)
	{
		UCaptureCameraComponent *captureCamCmp = NewObject<UCaptureCameraComponent>(this);
		if(captureCamCmp)
		{
			captureCamCmp->SetupAttachment(RootComponent);
			captureCamCmp->SetRelativeLocation(relativePosition);
			captureCamCmp->SetRelativeRotation(FRotator(relativeRotation.Y, relativeRotation.Z, relativeRotation.X));
			captureCamCmp->RegisterComponent();

			captureCamCmp->Initialize(colorTexture, depthTexture, fieldOfView);

			camId = captureCamCmp->getCameraId();
			m_CaptureCameras.Add(camId, captureCamCmp);

			if (m_CaptureCameras.Num() == 1)
			{
				if (m_Simulation && m_Simulation->ViewModes.Contains("WorldNormal"))
				{
					const FDeepDriveViewMode &viewMode = m_Simulation->ViewModes["WorldNormal"];
					//viewMode.Material = Material.Succeeded() ? Material.Object : 0;
					captureCamCmp->setViewMode(&viewMode);
				}
			}

			OnCaptureCameraAdded(colorTexture, label);
			//OnCaptureCameraComponentAdded(captureCamCmp);

		}
	}

	return camId;
}

void ADeepDriveAgent::SetControlValues(float steering, float throttle, float brake, bool handbrake)
{
	SetSteering(steering);
	SetThrottle(throttle);
	SetBrake(brake);
	SetHandbrake(handbrake);
}

void ADeepDriveAgent::SetSteering(float steering)
{
	m_curSteering = steering;
	GetVehicleMovementComponent()->SetSteeringInput(steering);
}

void ADeepDriveAgent::SetThrottle(float throttle)
{
	m_curThrottle = throttle;
	GetVehicleMovementComponent()->SetThrottleInput(throttle);
}

void ADeepDriveAgent::SetBrake(float brake)
{
	m_curBrake = brake;
	GetVehicleMovementComponent()->SetBrakeInput(brake);
}

void ADeepDriveAgent::SetHandbrake(bool on)
{
	m_curHandbrake = on;
}

void ADeepDriveAgent::ActivateCamera(EDeepDriveAgentCameraType cameraType)
{
	ChaseCamera->Deactivate();
	InteriorCamera->Deactivate();
	OrbitCamera->Deactivate();

	switch(cameraType)
	{
		case EDeepDriveAgentCameraType::FREE_CAMERA:
			SetInstrumentsVisibility(false);
			break;

		case EDeepDriveAgentCameraType::CHASE_CAMERA:
			ChaseCamera->Activate();
			SetInstrumentsVisibility(true);
			break;

		case EDeepDriveAgentCameraType::INTERIOR_CAMERA:
			InteriorCamera->Activate();
			SetInstrumentsVisibility(false);
			break;

		case EDeepDriveAgentCameraType::ORBIT_CAMERA:
			OrbitCamera->Activate();
			SetInstrumentsVisibility(false);
			break;
	}
}

void ADeepDriveAgent::DeactivateCameras()
{
	ChaseCamera->Deactivate();
	InteriorCamera->Deactivate();
	OrbitCamera->Deactivate();
}

void ADeepDriveAgent::SetCenterOfTrackSpline(USplineComponent *Spline)
{
	m_CenterOfTrackSpline = Spline;
}

void ADeepDriveAgent::SetOrbitCameraRotation(float pitch, float yaw)
{
	OrbitCameraArm->SetRelativeRotation(FRotator(pitch, yaw, 0.0f));
}

void ADeepDriveAgent::OnLapStart()
{
}

void ADeepDriveAgent::OnLapFinished()
{
}

void ADeepDriveAgent::reset()
{
	m_NumberOfLaps = 0;
	GetVehicleMovementComponent()->SetThrottleInput(0.0f);
	GetVehicleMovementComponent()->SetSteeringInput(0.0f);
	GetVehicleMovementComponent()->SetBrakeInput(1.0f);
	GetMesh()->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
}

float ADeepDriveAgent::getSpeed() const
{
	return GetVehicleMovementComponent()->GetForwardSpeed();
}

float ADeepDriveAgent::getSpeedKmh() const
{
	return GetVehicleMovementComponent()->GetForwardSpeed() * 0.036f;
}

float ADeepDriveAgent::getDistanceAlongRoute() const
{
	float res = 0.0f;
	if (m_CenterOfTrackSpline)
	{
		const float closestKey = m_CenterOfTrackSpline->FindInputKeyClosestToWorldLocation(GetActorLocation());

		const int32 index0 = floor(closestKey);
		const int32 index1 = floor(closestKey + 1.0f);

		const float dist0 = m_CenterOfTrackSpline->GetDistanceAlongSplineAtSplinePoint(index0);
		const float dist1 = m_CenterOfTrackSpline->GetDistanceAlongSplineAtSplinePoint(index1);

		res = FMath::Lerp(dist0, dist1, closestKey - static_cast<float> (index0));
	}
	return res;
}

float ADeepDriveAgent::getDistanceToCenterOfTrack() const
{
	float res = 0.0f;
	if (m_CenterOfTrackSpline)
	{
		FVector curLoc = GetActorLocation();
		const float closestKey = m_CenterOfTrackSpline->FindInputKeyClosestToWorldLocation(curLoc);
		res = (m_CenterOfTrackSpline->GetLocationAtSplineInputKey(closestKey, ESplineCoordinateSpace::World) - curLoc).Size();
	}
	return res;
}

float ADeepDriveAgent::getDistanceToObstacleAhead(float maxDistance)
{
	float distance = -1.0f;

	return distance;
}


void ADeepDriveAgent::OnCheckpointReached()
{
	ADeepDriveAgentControllerBase *ctrl = Cast<ADeepDriveAgentControllerBase>(GetController());
	if (ctrl)
		ctrl->OnCheckpointReached();
}

void ADeepDriveAgent::OnDebugTrigger()
{
	ADeepDriveAgentControllerBase *ctrl = Cast<ADeepDriveAgentControllerBase>(GetController());
	if (ctrl)
		ctrl->OnDebugTrigger();
}

ADeepDriveAgentControllerBase *ADeepDriveAgent::getAgentController()
{
	return Cast<ADeepDriveAgentControllerBase>(GetController());
}
