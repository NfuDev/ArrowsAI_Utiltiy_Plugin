// Still In Early Development Stage , Developed By NightFall16 @ArrowsInteractive - Unreal Engine 4.26.2


#include "ArrowsPerception.h"
#include "ArrowsPathAI.h"
#include <Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h>
#include "Kismet/KismetMathLibrary.h"
#include <Engine/Classes/GameFramework/Character.h>
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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
	AcceptedReportDistance = 2000.0f;

	VisionConeAngle = 45.0f;
	VisionConeLength = 900.0f;

	AwarenessDelay = 2.0f;
	UncertainedPercent = 0.1f;
	ForceFullAwarenessDistance = 500.0f;
	MaxMemory = 5.0f;
	ReactionDelay = 2.0f;

	LastSeenDebugDrawTime = 10.0f;

	

	// ...
}


// Called when the game starts
void UArrowsPerception::BeginPlay()
{
	Super::BeginPlay();

	//this will be used to figure if the player is in range so we dont need to to the trace logics and this is better performance , this refernce is for calculation only and not used for define detection
	DivinePlayerRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	AgentController = UAIBlueprintHelperLibrary::GetAIController(GetOwner());// i used the calss constructor last time to get the reference and this is wrong i forgot that possission happens only after begin play

	AgentController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &UArrowsPerception::OnAgentMoveCompleted);

	GuardingLocation = GetOwner()->GetActorTransform();//save the spawn position to return to after doing the search after forget or the check out the uncertained last seen locaion
	
	// ...
	
}


// Called every frame
void UArrowsPerception::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DebugPerception();
	NewPerceptionUpdate();//refactor this function later use the new idea of recenlt gained sight , recently lost sight , is currently having sight , is currently not having sight , the currently have and currently not have for tick update since they are called all the time as long as their condition is  correct
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
				bCalledByReport = false;
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
					//FVector ArrowsEnd = LastSeen + (EscapeTransform.GetRotation().Vector() * 100.0f);
					DrawLastSeen();
					UnCertainedDetection.Broadcast(LastSeen);
					ReactToDetection(false);
					AgentPlaySound(UnCertainedSounds);

				}
				else
				{
					ClearAndInvalidateTimer(RecentForgotHandler);
					bBusyWithLines = true;
					AgentPlaySound(RegainSightSounds);
					ClearAndInvalidateTimer(AwarenessTimeHandler);
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
					if (!bRecentlyForgot)
					{
						GetWorld()->GetTimerManager().SetTimer(AwarenessTimeHandler, this, &UArrowsPerception::AwarenessTimer, AwarenessDelay, false);
					}
					else
					{
						ClearAndInvalidateTimer(RecentForgotHandler);
						bBusyWithLines = true;
						AgentPlaySound(RegainSightSounds);
						AwarenessTimer();
						bRecentlyForgot = false;
					}

				}

			/*force full detection if we have line of sight , and the player is too close , then awareness will raise so quick (in this case instant awareness)*/
			  else if((GetOwner()->GetActorLocation() - DivinePlayerRef->GetActorLocation()).Size() <= ForceFullAwarenessDistance)
			  {
					ClearAndInvalidateTimer(AwarenessTimeHandler);

					if (!bRecentlyForgot)
					{
						PrintDebugs("Forced Detecion ", 10.0f);
						AwarenessTimer();
					}
					else
					{
						PrintDebugs("detected after forgtting and we forced detection ", 10.0f);
						ClearAndInvalidateTimer(RecentForgotHandler);
						bBusyWithLines = true;
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
					bBusyWithLines = true;
					AgentPlaySound(RegainSightSounds);
					ClearAndInvalidateTimer(AwarenessTimeHandler);
					PrintDebugs("fast sight regain and awareness clearance ", 10.0f);
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
			//FVector ArrowsEnd = LastSeen + (EscapeTransform.GetRotation().Vector() * 100.0f); wasnt needed cuz i deleted the draw arrow from here and implemented the draw last seen which has it's own logics for drawing the arrow
			DrawLastSeen();
			UnCertainedDetection.Broadcast(LastSeen);
			ReactToDetection(false);
			AgentPlaySound(UnCertainedSounds);
		}
		else
		{
			ClearAndInvalidateTimer(RecentForgotHandler);
			bBusyWithLines = true;
			AgentPlaySound(RegainSightSounds);
			AwarenessTimer();
			bRecentlyForgot = false;
		}
	}

		ClearAndInvalidateTimer(AwarenessTimeHandler);

		//start forgetting timer
		if (!GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler) && bPlayerWasSpotted)
		{
			bCalledByReport = false; // here the forgot is calculated so we need to specify that the call was not from the report
			PrintDebugs("Called Forget from the out of vision cone check");
			GetWorld()->GetTimerManager().SetTimer(ForgetTimeHandler, this, &UArrowsPerception::ForgettingTimer, MaxMemory, false);
		}

	}
	
	
}

void UArrowsPerception::ReportNearAgentsWithDetection(bool bIsLost)
{
	PrintDebugs("Detection Reported", 5.0f);
	TArray<FHitResult> TraceResults;
	TArray<AActor*> ActorsToIgnore;
	FVector Start = GetOwner()->GetActorLocation(); // start and end are the same since we are checking around in a radius and not actually tracing direction
	TEnumAsByte<ETraceTypeQuery> TraceChannel;
	TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	bool hit = UKismetSystemLibrary::SphereTraceMulti(GetWorld(), Start, Start, AcceptedReportDistance, TraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, TraceResults, true, FColor::Red, FColor::Green, 10.0f);
	if (hit)
	{

		for (auto& SingleHit : TraceResults)
		{
			APawn* AgentPawn = Cast<APawn>(SingleHit.GetActor());
			if (AgentPawn)
			{
				UArrowsPerception* OtherPerceptionComponenet = Cast< UArrowsPerception>(SingleHit.GetActor()->GetComponentByClass(UArrowsPerception::StaticClass()));

				if (OtherPerceptionComponenet)
				{
					PrintDebugs("Valid Near Enemy", 5.0f);
					OtherPerceptionComponenet->RespondToNearAgentReport(this, bIsLost);
				}
			}

			else
			{
				PrintDebugs("No Valid Pawn", 5.0f);
			}

		}
	}
}


void UArrowsPerception::RespondToNearAgentReport(UArrowsPerception* Instegator, bool bIsLost)
{
	//it is a bad naming if you are reading this, b is lost is used as b i found the player
	if (bIsLost)//if the agent sent the report and he was detecting the player then we know that the near agent was deteting so we force detection 
	{
		bNearAgentDetects = true;
		bCalledByReport = true;
		ReportedAwareness();//forcing awareness here too when we hear about the player being spotted by another near by agent
	}
	else
	{
		float ReportDistance = (this->GetOwner()->GetActorLocation() - Instegator->GetOwner()->GetActorLocation()).Size();

		if (bPlayerWasSpotted)// if the agent reported the loss of the player we send a report about we know where he is and force awareness
		{
			if (HasLineOfSight() && ReportLineOfSightCounter > 0.5)//only tell the reporter who lost the player that i know where he is if i have a line of sight , so they wont keep telling each other and never lose reference or forget
			{
				ReportLineOfSightCounter = 0.0f;

				Instegator->bNearAgentDetects = true;
				bCalledByReport = true;
				Instegator->ReportedAwareness();//telling the other agent if he reported that he lost the player , and we have spotted him so we tell him the location of the player by forcing the detection on him

				if (ReportDistance >= 600.0f)// if the reporter was too close it is more realistc to follow the one who didnt lose the player yet but if he is too far then the one who did not lose it yet can call him like he is here
				{
					AgentPlaySound(ReplyingToDetectionReport);//here is saying the player is here
				}
			}
		
			else
			{
				ReportLineOfSightCounter += 0.05;
			}
		}

		else
		{
			bNearAgentDetects = false;
			bCalledByReport = true; // so we can check it in the forget state so if some agent forgot near by another agent that is nearly to forget they wont repeat the lines
		}
	}
}

void UArrowsPerception::DelayedReport()
{
	if (!bBusyWithLines)
	{
		ReportNearAgentsWithDetection(bPlayerWasSpotted);
	}
	else
	{
		//if the agent got the report was saying anything we wait 2 seconds taking in mind that the sound playing will be done in this period , i should implement this logic using on sound finished delegate , but for now i'll go with this 
		FTimerHandle waitForLinesHandle;
		bBusyWithLines = false;
		GetWorld()->GetTimerManager().SetTimer(waitForLinesHandle, this, &UArrowsPerception::DelayedReport, 2.0f, false);

	}
}// not used anymore

void UArrowsPerception::ReportRegainSight()
{
	TArray<FHitResult> TraceResults;
	TArray<AActor*> ActorsToIgnore;
	FVector Start = GetOwner()->GetActorLocation(); // start and end are the same since we are checking around in a radius and not actually tracing direction
	TEnumAsByte<ETraceTypeQuery> TraceChannel;
	TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	bool hit = UKismetSystemLibrary::SphereTraceMulti(GetWorld(), Start, Start, AcceptedReportDistance, TraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, TraceResults, true, FColor::Red, FColor::Green, 10.0f);
	if (hit)
	{

		for (auto& SingleHit : TraceResults)
		{
			APawn* AgentPawn = Cast<APawn>(SingleHit.GetActor());
			if (AgentPawn)
			{
				UArrowsPerception* OtherPerceptionComponenet = Cast< UArrowsPerception>(SingleHit.GetActor()->GetComponentByClass(UArrowsPerception::StaticClass()));

				if (OtherPerceptionComponenet)
				{
					OtherPerceptionComponenet->RecenlyOtherAgentRegained = true;
					FTimerHandle ResetDelay;
					GetWorld()->GetTimerManager().SetTimer(ResetDelay, OtherPerceptionComponenet, &UArrowsPerception::ResetRecentlyRegained, 2.0f, false);//call the function on the other components to reset their recent regained value
				}
			}

		}
	}
}

void UArrowsPerception::ResetRecentlyRegained()
{
	RecenlyOtherAgentRegained = false;
}

#pragma endregion Perception Update

#pragma region NewPerceptionUpdate

void UArrowsPerception::NewPerceptionUpdate()
{
	if (DivinePlayerRef)
	{

		//update awareness value for UI Only
		if (GetWorld()->GetTimerManager().IsTimerActive(AwarenessTimeHandler))
		{
			AgentAwareness = GetWorld()->GetTimerManager().GetTimerElapsed(AwarenessTimeHandler);

		}

		float DistanceToPlayer = (GetOwner()->GetActorLocation() - DivinePlayerRef->GetActorLocation()).Size();

		// for debug only
		if (bNearAgentDetects)
		{
			PrintDebugs("Am Checking Cuz i got report", 0.0f);
		}

		if (DistanceToPlayer <= SensingRadius || !bPlayerWasSpotted)
		{
			PrintDebugs("Am Thinking", 0.0f);

			if (TargetInVision())
			{
				if (HasLineOfSight() && !bHasLineOfSight)
				{
					bHasLineOfSight = true;//the variable is to prevent the spam of this part of code
					OnAgentPerceptionUpdate();
				}
				else if (!HasLineOfSight() && bHasLineOfSight)
				{
					bHasLineOfSight = false;
					OnAgentPerceptionUpdate();
				}

				if (HasLineOfSight())
					EscapeTransform = DivinePlayerRef->GetActorTransform();
			}

		}

		else if (bPlayerWasSpotted)
		{
			bHasLineOfSight = HasLineOfSight();
			OnAgentPerceptionUpdate();
		}
	}

}

bool UArrowsPerception::TargetInVision()
{
	FRotator DirectionTowardsPlayer = (DivinePlayerRef->GetActorLocation() - GetOwner()->GetActorLocation()).Rotation();
	FRotator SideV = FRotator(0.0f, -VisionConeAngle, 0.0f);
	FRotator ConeSideVector = SideV.RotateVector(GetOwner()->GetActorRotation().Vector()).Rotation();

	FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(DirectionTowardsPlayer, ConeSideVector);

	return  (DeltaRotation.Yaw >= 0 && DeltaRotation.Yaw <= (2 * VisionConeAngle));
}

bool UArrowsPerception::HasLineOfSight()
{
	FHitResult TraceResults;
	TArray<AActor*> ActorsToIgnore;
	FVector Start = GetOwner()->GetActorLocation();
	FVector End = DivinePlayerRef->GetActorLocation();
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjetcs;
	TEnumAsByte<EDrawDebugTrace::Type> DegubLinetrace = VisionDebug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
	
	TraceObjetcs.Add(EObjectTypeQuery::ObjectTypeQuery1);//static objects query channel

	bool hit = UKismetSystemLibrary::LineTraceSingleForObjects(this, Start, End, TraceObjetcs, false, ActorsToIgnore, DegubLinetrace, TraceResults, true, FColor::Red, FColor::Green, 0.0f);

	return !hit;//inversing the hit value cuz if no hit means yes line of sight
}

void UArrowsPerception::OnAgentPerceptionUpdate()
{
	float DistanceToPlayer = (GetOwner()->GetActorLocation() - DivinePlayerRef->GetActorLocation()).Size();

	if (bHasLineOfSight) // 3 cases , we seen for first time so we start the awareness , we seen him after we lost sight but not for too much to forget
	{

		float ElapsedForgetTime = GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler)? GetWorld()->GetTimerManager().GetTimerElapsed(ForgetTimeHandler) : 0.0f;

		ClearAndInvalidateTimer(ForgetTimeHandler);

		if (ElapsedForgetTime == 0.0f && !bRecentlyForgot)//fist time seen
		{

			if (DistanceToPlayer <= ForceFullAwarenessDistance) // seen first time and too close
			{
				ClearAndInvalidateTimer(AwarenessTimeHandler);
				AwarenessTimer();
			}

			if (!GetWorld()->GetTimerManager().IsTimerActive(AwarenessTimeHandler) && !bPlayerWasSpotted)//so if we had forced detection we dont need to call this logic
			{

				GetWorld()->GetTimerManager().SetTimer(AwarenessTimeHandler, this, &UArrowsPerception::AwarenessTimer, AwarenessDelay, false);
			}
		}
		
		else if((ElapsedForgetTime/ MaxMemory > 0.7f) || bRecentlyForgot)//seen after lost sight and before forgetting we force detection 
		{
			if (!RecenlyOtherAgentRegained)//the variable value is corrosponding with the other near by agent and should be set by a report too
			{
				AgentPlaySound(RegainSightSounds);
				ReportRegainSight();
			}
			
			ReportedAwareness(); // used it cuz it do the same as awareness timer but without the sound so it wont overlap with the regained sound playing 
		}

	}

	else // has no line of sight , 2 cases , lost sight after detection so we start forget timer, and lost sight before detection we call the uncertained detection
	{
		if ((AgentAwareness / AwarenessDelay >= UncertainedPercent) && !bPlayerWasSpotted && !GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler))//if lose line of sight near to full awareness then call the uncertained detection
		{
			AgentAwareness = 0.0f;
			FVector LastSeen = EscapeTransform.GetLocation();
			DrawLastSeen();

			UnCertainedDetection.Broadcast(LastSeen);
			AgentPlaySound(UnCertainedSounds);

			ClearAndInvalidateTimer(AwarenessTimeHandler);
		}

		else if(bPlayerWasSpotted && !GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler))
		{
			bCalledByReport = false;
			GetWorld()->GetTimerManager().SetTimer(ForgetTimeHandler, this, &UArrowsPerception::ForgettingTimer, MaxMemory, false);
		}

		else if (!((AgentAwareness / AwarenessDelay >= UncertainedPercent) && !bPlayerWasSpotted && !GetWorld()->GetTimerManager().IsTimerActive(ForgetTimeHandler)))// the oppiste of the first case
		{
			AgentAwareness = 0.0f;
			ClearAndInvalidateTimer(AwarenessTimeHandler);
		}

	}
}

#pragma endregion

#pragma region Debugging

void UArrowsPerception::PrintDebugs(FString DebugMes, float duration)
{
	if (OnScreenDebugs)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, duration, FColor::Green, DebugMes);
		}
	}
}

void UArrowsPerception::DrawLastSeen()
{
	if (DebugLastSeen)
	{
		//selecting the right direction to draw the arrow , if we are moving then we care about the direction of the movement and if not the the rotation of the actor which is registed in the escape transform
		FVector PlayerVelocity = DivinePlayerRef->GetVelocity();
		PlayerVelocity.Z = 0; // removing the speed of falling so we dont get wrong directin if the last moment before seeing the player the player was falling we will get a downwards arrow and this will give invalid searching locations 

		FVector DrawDirection = PlayerVelocity.Size() > 0.1 ? DivinePlayerRef->GetVelocity().Rotation().Vector() : EscapeTransform.GetRotation().Vector();

		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), EscapeTransform.GetLocation(), 50.0f, 4, FColor::Red, LastSeenDebugDrawTime, 1.0f);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), EscapeTransform.GetLocation(), EscapeTransform.GetLocation()+(DrawDirection*100.0f) , 2.0f, FColor::Red, LastSeenDebugDrawTime, 2.0f);
	}
}

void UArrowsPerception::DrawGeneratedPoints(FVector Location, FVector Normals, FColor _Color)
{
	if (PointGenerationDebug)
	{
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Location, 50.0f, 4, _Color, 10.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), Location, Location + (Normals * 100.0f), 2.0f, _Color, 10.0f, 2.0f);
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
void UArrowsPerception::AgentPlaySound(TArray<USoundBase*> Sounds)
{
	// play sound here
	if (Sounds.Num() > 0)
	{

		USoundBase* Sound = Sounds[UKismetMathLibrary::RandomIntegerInRange(0, Sounds.Num() - 1)];
		
		
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
	ReportNearAgentsWithDetection(true);
	AgentPlaySound(SpottedSound);
	//if (!(bRecentlyForgot || bNearAgentDetects) && !bCalledByReport)//making sure that if there are two agents one of them have to scream the lines but the other one do not need to just fire the delegate
	//{
	//	AgentPlaySound(SpottedSound);
	//	bCalledByReport = false;
	//}
	//
	//if (ReportDetection && !bNearAgentDetects)
	//{
	//	FTimerHandle ReportHandle;
	//	GetWorld()->GetTimerManager().SetTimer(ReportHandle, this, &UArrowsPerception::DelayedReport, 2.0f, false);
	//	bNearAgentDetects = false;// very sus code i dont know why, but this is caused due to the change from using bCalledByReport
	//}
}

/*splitting the awareness logics so we dont need all the variables like bwascalled by report and other complex logics and if statements */
void UArrowsPerception::ReportedAwareness()
{
	ClearAndInvalidateTimer(AwarenessTimeHandler);
	bPlayerWasSpotted = true;
	OnPerceptionDetects.Broadcast(DivinePlayerRef);
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
	if(!bCalledByReport)
	{
		AgentPlaySound(ForgotSounds);
	}
	

	if (ReportDetection)
	{
		FTimerHandle ReportHandle;
		GetWorld()->GetTimerManager().SetTimer(ReportHandle, this, &UArrowsPerception::DelayedReport, 3.0f, false);
		ReportNearAgentsWithDetection(false);
	}
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
				DrawGeneratedPoints(Location, Normals, FColor::Green);
		    }

			//too close and same wall direction
			else
			{
				PrintDebugs("Filtered cuz faild the distance test", 5.0f);
				DrawGeneratedPoints(Location, Normals, FColor::Red);
			}
		}

		//not same wall direction
		else
		{

			PrintDebugs("Added cuz not same direction", 5.0f);
			PointsData[Index].HidingLocations.Add(NewPoint);
			DrawGeneratedPoints(Location, Normals, FColor::Green);
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
		DrawGeneratedPoints(Location, Normals, FColor::Green);
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

	TSubclassOf<UNavigationQueryFilter> NavigationFilter;
	if (AgentController)
	{
		AgentController->MoveToLocation(Location, 5.0f, true, true, true, true, NavigationFilter, true);
	}
	else
	{
		PrintDebugs("AI controller is not valid", 5.0f);
	}
	
}

void UArrowsPerception::OnAgentMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	// seek nex point 
	if (!ReactIsSearch && MovementRequest)
	{
		MovementRequest = false;
		AgentMoveTo(GuardingLocation.GetLocation());// returning to guarding location just for testing
	}
}

void UArrowsPerception::ReactToDetection(bool bCalledOnForgot)
{
	if (bCalledOnForgot)
	{

	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(DelayedReactionHandler, this, &UArrowsPerception::DelayedUncertainedSearch, ReactionDelay, false);
	}
		
}

void UArrowsPerception::DelayedUncertainedSearch()
{
	ReactIsSearch = false;
	MovementRequest = true;
	AgentMoveTo(EscapeTransform.GetLocation());
}
#pragma endregion
