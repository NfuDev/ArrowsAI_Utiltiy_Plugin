// Still In Early Development Stage , Developed By NightFall16 @ArrowsInteractive - Unreal Engine 4.26.2


#include "ArrowsPerception.h"
#include "ArrowsPathAI.h"
#include <Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h>
#include "Kismet/KismetMathLibrary.h"
#include <Engine/Classes/GameFramework/Character.h>
#include "Kismet/GameplayStatics.h"


#include "Blueprint/AIBlueprintHelperLibrary.h"


// Sets default values for this component's properties
UArrowsPerception::UArrowsPerception()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//setting defaults 
	SensingRadius = 1500.0f;
	SensingTag = FName("Player");

	VisionConeAngle = 45.0f;
	VisionConeLength = 900.0f;

	AwarenessDelay = 2.0f;
	UncertainedPercent = 0.1f;
	ForceFullAwarenessDistance = 500.0f;
	MaxMemory = 5.0f;

	LastSeenDebugDrawTime = 10.0f;

	AgentController = UAIBlueprintHelperLibrary::GetAIController(GetOwner());
	//AgentController->
	
	
	// ...
}


// Called when the game starts
void UArrowsPerception::BeginPlay()
{
	Super::BeginPlay();

	//this will be used to figure if the player is in range so we dont need to to the trace logics and this is better performance , this refernce is for calculation only and not used for define detection
	DivinePlayerRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	
	// ...
	
}


// Called every frame
void UArrowsPerception::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DebugPerception();
	PerceptionUpdate();
	// ...
}

void UArrowsPerception::DebugPerception()
{
	// debug vision cone 
	if (VisionDebug)
	{
		//drawing the cone 
		FColor ConeColor = bHasLineOfSight ? FColor::Red : FColor::Green;
		float Angle = FMath::DegreesToRadians(VisionConeAngle);
		UKismetSystemLibrary::DrawDebugCone(this, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation().Vector() , VisionConeLength, Angle, Angle, 12, ConeColor, 0.0f, 1.0f);

		//line to player
		if(bHasLineOfSight)
		UKismetSystemLibrary::DrawDebugLine(this, GetOwner()->GetActorLocation(), DivinePlayerRef->GetActorLocation(), FColor::Red, 0.0f, 1.0f);
	}
}

#pragma region Perception Update

//this function checks for the player if he is in the vision cone so we detect , we ditched the last logic of using sphere trace we are now faking it using actual distance check and angle check and if those passed then the player is for
//sure percevable 
void UArrowsPerception::PerceptionUpdate()
{
	//update awareness value for UI Only
	if (GetWorld()->GetTimerManager().IsTimerActive(AwarenessTimeHandler))
	{
		AgentAwareness = GetWorld()->GetTimerManager().GetTimerElapsed(AwarenessTimeHandler);
		
	}

	//this checks if the player is in the vision cone
	if (DivinePlayerRef && ( GetOwner()->GetActorLocation() - DivinePlayerRef->GetActorLocation() ).Size() <= SensingRadius)
	{
		FRotator DirectionTowardsPlayer = (DivinePlayerRef->GetActorLocation() - GetOwner()->GetActorLocation()).Rotation();
		FRotator SideV = FRotator(0.0f, -VisionConeAngle, 0.0f);//get the rotation needed to rotate the forward vector of the owner to the side so we calculate in one direction and not left and right this will make math really easy
		FRotator ConeSideVector = SideV.RotateVector(GetOwner()->GetActorRotation().Vector()).Rotation();

		FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(DirectionTowardsPlayer, ConeSideVector);

		bInVisionCone = (DeltaRotation.Yaw >= 0 && DeltaRotation.Yaw <= (2*VisionConeAngle));
		PrintDebugs(FString::FromInt(DeltaRotation.Yaw));
	}

	/*here we try to see if there is any obstacles that blocks the vision*/
	if (bInVisionCone)
	{
		FHitResult TraceResults;
		TArray<AActor*> ActorsToIgnore;
		FVector Start = GetOwner()->GetActorLocation();
		FVector End = DivinePlayerRef->GetActorLocation();
		TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjetcs;

		TraceObjetcs.Add(EObjectTypeQuery::ObjectTypeQuery1);

		//UKismetSystemLibrary::LineTraceSingle(this, Start, End, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, TraceResults, true, FColor::Red, FColor::Green, 0.0f);

		bool hit = UKismetSystemLibrary::LineTraceSingleForObjects(this, Start, End, TraceObjetcs, false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, TraceResults, true, FColor::Red, FColor::Green, 0.0f);
		
		if (hit)//if the hit is true then the player is behind something cuz we are checking for hits with static objects like walls and stuff
		{

			bHasLineOfSight = false;
			if (bPlayerWasSpotted && !GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler))
			{
				PrintDebugs("Player was spotted and i lost sight so i forget after the max memory value", 5.0f);

				GetWorld()->GetTimerManager().SetTimer(ForgetTimeHandler, this, &UArrowsPerception::ForgettingTimer, MaxMemory, false);
			}

			// the awareness is high enough to doubt seeing the player but we did not see the player yet , the check for bPlayerWasSpotted is to prevent the agent from calling the uncertained seeing if the values was rigght
			//but the player was spotted and that was causing a bug and this is the fix
			else if ((AgentAwareness/ AwarenessDelay >= UncertainedPercent) && !bPlayerWasSpotted && !GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler))
			{
				if (!bRecentlyForgot)
				{

					AgentAwareness = 0.0f;
					FVector LastSeen = EscapeTransform.GetLocation();
					FVector ArrowsEnd = LastSeen + (EscapeTransform.GetRotation().Vector() * 100.0f);
					DrawLastSeen();
					UnCertainedDetection.Broadcast(LastSeen);
					AgentPlaySound(UnCertainedSounds);

				}
				else
				{
					ClearAndInvalidateTimer(RecentForgotHandler);
					AgentPlaySound(RegainSightSounds);
					AwarenessTimer();
					bRecentlyForgot = false;
				}
			}

			ClearAndInvalidateTimer(AwarenessTimeHandler);
		}


		else if (!hit)
		{
			bHasLineOfSight = true;
			EscapeTransform = DivinePlayerRef->GetActorTransform();

			if (!bPlayerWasSpotted)//only call the awareness if we had not spot the player yet , but if so then scream "here you are"
			{
				
				if (!GetWorld()->GetTimerManager().IsTimerActive(AwarenessTimeHandler) && (GetOwner()->GetActorLocation() - DivinePlayerRef->GetActorLocation()).Size() > ForceFullAwarenessDistance)
				{

					GetWorld()->GetTimerManager().SetTimer(AwarenessTimeHandler, this, &UArrowsPerception::AwarenessTimer, AwarenessDelay, false);

				}

			/*force full detection if we have line of sight , and the player is too close , then awareness will raise so quick (in this case instant awareness)*/
			else if((GetOwner()->GetActorLocation() - DivinePlayerRef->GetActorLocation()).Size() <= ForceFullAwarenessDistance)
				{
					if (!bRecentlyForgot)
					{
						AwarenessTimer();
					}
					else
					{
						ClearAndInvalidateTimer(RecentForgotHandler);
						AgentPlaySound(RegainSightSounds);
						AwarenessTimer();
						bRecentlyForgot = false;
						
					}
				}
			}
			
			else if (bPlayerWasSpotted && GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler))//if the player was spoted and we lost sight but we regaied it before forgetting him we call this part of the code
			{
				if (GetWorld()->GetTimerManager().GetTimerElapsed(ForgetTimeHandler) / MaxMemory > 0.7f)
				{

					AgentPlaySound(RegainSightSounds);
				}

				ClearAndInvalidateTimer(ForgetTimeHandler);
			}
		}
	}


	//make sure after the player is out of the vision cone and since this will stop the trace we need to manually clear the line of sight
	else if (!bInVisionCone)
	{
		bHasLineOfSight = false;

    if ((AgentAwareness / AwarenessDelay >= UncertainedPercent) && !bPlayerWasSpotted && !GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler))
	{

		if (!bRecentlyForgot)
		{


			AgentAwareness = 0.0f;
			FVector LastSeen = EscapeTransform.GetLocation();
			FVector ArrowsEnd = LastSeen + (EscapeTransform.GetRotation().Vector() * 100.0f);
			DrawLastSeen();
			UnCertainedDetection.Broadcast(LastSeen);
			AgentPlaySound(UnCertainedSounds);
		}
		else
		{
			ClearAndInvalidateTimer(RecentForgotHandler);
			AgentPlaySound(RegainSightSounds);
			AwarenessTimer();
			bRecentlyForgot = false;
		}
	}

		ClearAndInvalidateTimer(AwarenessTimeHandler);

		//start forgetting timer
		if (!GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler) && bPlayerWasSpotted)
		{
			PrintDebugs("Called Forget from the out of vision cone check");
			GetWorld()->GetTimerManager().SetTimer(ForgetTimeHandler, this, &UArrowsPerception::ForgettingTimer, MaxMemory, false);
		}

	}
	
	
}

#pragma endregion Perception Update

#pragma region Debugging

void UArrowsPerception::PrintDebugs(FString DebugMes, float duration)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, duration, FColor::Green, DebugMes);
	}
}

void UArrowsPerception::DrawLastSeen()
{
	if (DebugLastSeen)
	{
		//selecting the right direction to draw the arrow , if we are moving then we care about the direction of the movement and if not the the rotation of the actor which is registed in the escape transform
		FVector DrawDirection = DivinePlayerRef->GetVelocity().Size() > 0.1 ? DivinePlayerRef->GetVelocity().Rotation().Vector() : EscapeTransform.GetRotation().Vector();

		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), EscapeTransform.GetLocation(), 50.0f, 4, FColor::Red, LastSeenDebugDrawTime, 1.0f);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), EscapeTransform.GetLocation(), EscapeTransform.GetLocation()+(DrawDirection*100.0f) , 2.0f, FColor::Red, LastSeenDebugDrawTime, 2.0f);
	}
}
#pragma endregion


//to save time so i dont need to clear and invalidate each time so i added a function like the one in the blueprints to do both things in one call
void UArrowsPerception::ClearAndInvalidateTimer(FTimerHandle& Timer)
{
	GetWorld()->GetTimerManager().ClearTimer(Timer);
	Timer.Invalidate();
}

//plays random sound when certain delegate is called 
void UArrowsPerception::AgentPlaySound(TArray<USoundWave*> Sounds)
{
	// play sound here
	if (Sounds.Num() > 0)
	{

		USoundWave* Sound = Sounds[UKismetMathLibrary::RandomIntegerInRange(0, Sounds.Num() - 1)];

		if (Sound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
		}
		
		else
	    {
			PrintDebugs("Sound Reference is not vaild", 3.0f);
		}
	}
	
}

#pragma region Timers_Implementation

void UArrowsPerception::AwarenessTimer()
{
	PrintDebugs("the awareness funciuon is being called", 3.0f);
	bPlayerWasSpotted = true;
	OnPerceptionDetects.Broadcast(DivinePlayerRef);
	if (!bRecentlyForgot)
	{
		AgentPlaySound(SpottedSound);
	}
	
}

void UArrowsPerception::ForgettingTimer()
{
	InvestigationPoints.Empty();

	AgentAwareness = 0.0f;
	bPlayerWasSpotted = false;
	bRecentlyForgot = true;
	// so if we forgot the player reference and directly after that we had and uncertained detection, we want to say that we actually saw the player again since we recently forgot him and we were in a chase so any doubt is certained
	GetWorld()->GetTimerManager().SetTimer(RecentForgotHandler, this, &UArrowsPerception::ResetRecentlyForgot, 3.0f, false);

	DrawLastSeen();
	RadialCheck();

	InvestigationPoints = CombineHidingPoints();
	OnAgentForget.Broadcast(InvestigationPoints);

	AgentPlaySound(ForgotSounds);
}

void UArrowsPerception::ResetRecentlyForgot()
{
	bRecentlyForgot = false;
}
#pragma endregion 

#pragma region PointsCheck

void UArrowsPerception::RadialCheck()
{
	float Start = -90.0f;
	float End = 90.0f;
	float Step = 10.0f;
	PointsData.Empty();

branch:
	if (Start <= End)
	{
		FRotator DirectionRotator = FRotator(0.0f, Start, 0.0f);
		FVector EditedFVector = DirectionRotator.RotateVector(EscapeTransform.GetRotation().Vector());
		FVector StartCheck = (EditedFVector * 1000.0f) + EscapeTransform.GetLocation();
		FVector EndCheck = EscapeTransform.GetLocation();

		TArray<FHitResult> TraceResults;
		TArray<AActor*> ActorsToIgnore;
		TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjetcs;
		TraceObjetcs.Add(EObjectTypeQuery::ObjectTypeQuery1); 

		bool hit = UKismetSystemLibrary::LineTraceMultiForObjects(GetWorld(), StartCheck, EndCheck, TraceObjetcs, false, ActorsToIgnore, EDrawDebugTrace::None, TraceResults, true, FColor::Red, FColor::Green, 15.0f);

		if (hit)
		{
			LoopThroughRayHitResults(TraceResults);
		}

		Start += Step;
		goto branch;
	}

	else
	{
		//call the point combine function and also maybe the draw debug code // changed my mind call the combine after forgetting about the player and use the return value to send a local variable
		PrintDebugs("Radial Check Is Done", 10.0f);
	}
}

//to find if the current point is found behind some object that we already have points behind it , so we can test the distance between those two points and see if we should add this point of ignore it 
bool UArrowsPerception::FindStructArrayElementByMember(AActor* StructMemeber, TArray<FDetectedObstacles> _PointsData,  int32& FoundIndex)
{
	int32 itiratorIndex = 0;
	for (FDetectedObstacles Itirator : _PointsData)
	{

		if (Itirator.ObstacleObject == StructMemeber)
		{
			FoundIndex = itiratorIndex;
			return true;
		}

		else
		{
			FoundIndex = 0;
		}

		itiratorIndex++;
	}
	
	return false;

}

//loop through the hit results for each ray and check the points 
void UArrowsPerception::LoopThroughRayHitResults(TArray<FHitResult> RayHitResults)
{
	bool bPointsHasInitOverlap;
	int32 Index;
	bool found;
	for (FHitResult Hit : RayHitResults)
	{
		bPointsHasInitOverlap = Hit.bStartPenetrating;
		found = FindStructArrayElementByMember(Hit.GetActor(), PointsData, Index);
		if (!bPointsHasInitOverlap)
		{
			//add new memeber if the point is behind new obstacle cuz we did the check using the [find struct array element by member]
			if (!found)
			{
				//PrintDebugs("Adding New Memeber", 5.0f);
				SetMemberInPoints(Hit.GetActor(), Hit.Location, Hit.Normal, 300.0f);
			}

			else
			{
				//PrintDebugs("Setting At Index : " + FString::FromInt(Index), 5.0f);
				SetMemberInPoints(Hit.GetActor(), Hit.Location, Hit.Normal, 300.0f);
				//PrintDebugs("Found Similar Entry, Edited The Found Element AT Index : " + FString::FromInt(Index) + " Number of previous points : " + FString::FromInt(PointsData[Index].HidingLocations.Num()), 10.0f);
			}
		}

		else
		{
			PrintDebugs("should ignore cuz there was initial overlap ", 5.0f);
		}
	}
	
}


void UArrowsPerception::SetMemberInPoints(AActor* Obstacle, FVector Location, FVector Normals, float AcceptedDistance)
{
	int32 Index;
	float SharedNormalsPointDistance;

	bool found = FindStructArrayElementByMember(Obstacle, PointsData, Index);
	FPoint NewPoint;
	NewPoint.SetHidingPoint(Location, Normals.Rotation());
	if (found)
	{
		if (PointNormalsCheck(PointsData[Index].HidingLocations, Normals.Rotation(), Location, SharedNormalsPointDistance))
		{
			PrintDebugs("Found Same Obstacle with same normals and distance is : " + FString::FromInt(SharedNormalsPointDistance), 5.0f);
			if (SharedNormalsPointDistance >= AcceptedDistance)
			{
				PrintDebugs("Addeed Cuz Distance Was Bigger Than Accpeted Distance", 5.0f);
				PointsData[Index].HidingLocations.Add(NewPoint);

				if (PointGenerationDebug)
				{
					UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Location, 50.0f, 4, FColor::Green, 10.0f, 1.0f);
					UKismetSystemLibrary::DrawDebugArrow(GetWorld(), Location, Location + (Normals * 100.0f), 2.0f, FColor::Green, 10.0f, 2.0f);
				}
		    }

			//too close and same wall direction
			else
			{
				PrintDebugs("Filtered cuz faild the distance test", 5.0f);
				if (PointGenerationDebug) 
				{
					UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Location, 50.0f, 4, FColor::Red, 10.0f, 1.0f);
					UKismetSystemLibrary::DrawDebugArrow(GetWorld(), Location, Location + (Normals * 100.0f), 2.0f, FColor::Red, 10.0f, 2.0f); 
				}
			}
		}

		//not same wall direction
		else
		{

			PrintDebugs("Added cuz not same direction", 5.0f);
			PointsData[Index].HidingLocations.Add(NewPoint);
			if (PointGenerationDebug)
			{
				UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Location, 50.0f, 4, FColor::Green, 10.0f, 1.0f);
				UKismetSystemLibrary::DrawDebugArrow(GetWorld(), Location, Location + (Normals * 100.0f), 2.0f, FColor::Green, 10.0f, 2.0f);
			}
		}
		
	}
	
	// not found se we make new entry
	else
	{
		PrintDebugs("Added cuz new obstacle", 5.0f);
		FDetectedObstacles NewEntryPoint;
		NewEntryPoint.HidingLocations.Add(NewPoint);
		NewEntryPoint.ObstacleObject = Obstacle;
		PointsData.Add(NewEntryPoint);

		if (PointGenerationDebug)
		{
			UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Location, 50.0f, 4, FColor::Green, 10.0f, 1.0f);
			UKismetSystemLibrary::DrawDebugArrow(GetWorld(), Location, Location + (Normals * 100.0f), 2.0f, FColor::Green, 10.0f, 2.0f);
		}
	}
}


bool UArrowsPerception::PointNormalsCheck(TArray<FPoint> _Points, FRotator RotationToCheck, FVector PointLocation, float& Distance)
{
	for (int32 i = _Points.Num() - 1; i >= 0; i--)
	{
		//PrintDebugs("Angle : " +FString::FromInt(UKismetMathLibrary::NormalizedDeltaRotator(_Points[i].Rotation, RotationToCheck).Yaw), 10.0f);
		/*PrintDebugs("Angle : " + FString::FromInt((_Points[i].Rotation.Vector() - RotationToCheck.Vector()).Size()), 10.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), PointLocation, 50.0f, 4, FColor::Yellow, 10.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), _Points[i].Location, 50.0f, 4, FColor::Red, 10.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), _Points[i].Location, _Points[i].Location + (_Points[i].Rotation.Vector() * 100.0f), 2.0f, FColor::Green, 10.0f, 2.0f);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), PointLocation, PointLocation + (RotationToCheck.Vector() * 100.0f), 2.0f, FColor::Green, 10.0f, 2.0f);*/

		if (_Points[i].Rotation.Vector() == RotationToCheck.Vector())//(UKismetMathLibrary::NormalizedDeltaRotator(_Points[i].Rotation, RotationToCheck).Yaw == 0.0f)
		{
			Distance = (_Points[i].Location - PointLocation).Size();
			
			return true;
		}
	}

	Distance = 0.0f;
	return false;
}

//combine all [Fpoints] struct locations into one vector array to be send via forget delegate to the user
TArray<FVector> UArrowsPerception::CombineHidingPoints()
{
	TArray<FVector> Results;
	for (auto& itirator : PointsData)
	{
		for (FPoint _point : itirator.HidingLocations)
		{
			FVector NewLocation = _point.Location + (_point.Rotation.Vector() * 50.0f);
			Results.Add(NewLocation);
		}
	}

	return Results;
}

#pragma endregion


#pragma region Behaviour Implementations:

void UArrowsPerception::AISelectBehaviour()
{
	switch (AgentBehaviour)
	{
	    case EAgentBehaviour::Guarding:
	    {
			GaurdingBehaviour();
	    }

		case EAgentBehaviour::PathPatrolling:
		{

		}

		case EAgentBehaviour::RandomPoint :
		{

		}

	}
}

void UArrowsPerception::GaurdingBehaviour()
{
	
}

void UArrowsPerception::AgentMoveTo(FVector Location)
{

//	TSubclassOf<UNavigationQueryFilter> NavigationFilter;
  //  AgentController->MoveToLocation(Location, 5.0f, true, true, true, true, NavigationFilter, true);
	
	
}

//void UArrowsPerception::OnAgentMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
//{
//	// seek nex point 
//}

#pragma endregion