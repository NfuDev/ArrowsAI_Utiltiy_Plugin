// Still In Early Development Stage , Developed By NightFall16 @ArrowsInteractive - Unreal Engine 4.26.2

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Delegates/Delegate.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "ArrowsPerception.generated.h"


UENUM(BlueprintType)
enum class  EPerceptionState : uint8
{
	Active,
	Disabled
};

UENUM(BlueprintType)
enum class  EAgentBehaviour : uint8
{
	Guarding,
	PathPatrolling,
	RandomPoint
};

USTRUCT(BlueprintType)
struct FPoint
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRotator Rotation;


	FPoint()
	{

	}
	void SetHidingPoint(FVector _location, FRotator _Rotation)
	{
		Location = _location;
		Rotation = _Rotation;
	}
};

USTRUCT(BlueprintType)
struct FDetectedObstacles
{
    GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ObstacleObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTransform> HidingPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPoint> HidingLocations;

	FDetectedObstacles()
	{
		ObstacleObject = nullptr;
    }

	AActor* GetObstacle() { return ObstacleObject; }
};



//Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerceptionDetects, const AActor*, DetectedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnCertainedDetection, const FVector&, LocationToCheck);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentForget, const TArray<FVector>&, SearchingLocations);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARROWSAIUTILITY_API UArrowsPerception : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UArrowsPerception();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*the distance that targets needs to be in so they can be registerd by the perception*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		float SensingRadius;

	/*if the agent should report his perception detection to near by enemies*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
	    bool ReportDetection;

	/*max distance the report can reach , only agents in this distance range can hear the report and act upon it's information*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		float AcceptedReportDistance;

	/*the trace channel used for registering targets*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
	    TEnumAsByte<ETraceTypeQuery> SensingChannel;

	/*Array Of sound to say when the agent spots the player*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundBase*> SpottedSound;

	/*Array of sounds to say when the agent regain the line of sight before forgetting the player */
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundBase*> RegainSightSounds;

	/*Array of sounds to say after the agent forget about the player*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundBase*> ForgotSounds;

	/*Array Of sounds to say when the agent is not ceratin about seeing the player , it will play when the agent nearly spotted the player*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundBase*> UnCertainedSounds;

	/*Array of sounds to say when we try to tell other near by agents about detection if they reported lost , made for the case like some agent lost the player and played the forgot sounds then this agent hear that the other has lost 
	the player and this agent at the same time knows about the player then he will say "he still here" so that the agent who lost the player will know about it again*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundBase*> ReportingDetection;

	/*Array of sounds to replay to the report, like when the other agent tell us where the player is after we lose him , we say like "thanks i got it" or like "yeah i see him now" */
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundBase*> ReplyingToDetectionReport;

	/*Called When the perception detects the player and returns the player referece as [Actor] so you can cast to your custom class and do your own custom logics*/
	 UPROPERTY(BlueprintAssignable, Category = "Perception Settings")
		FOnPerceptionDetects OnPerceptionDetects;

	/*Called When the agent feels like seen the player , you can change how quicly the agent detect the player with the [awareness delay] value , when this delegate is called it is better to stop your agent movement logics
	so the agent reaction is realistic , stop it for second or two and then resume , or if you are using arrows patrol behaviour then no need for extra logics here, or in the behaviour tree , just make your combat logics*/
	 UPROPERTY(BlueprintAssignable, Category = "Perception Settings")
	    FUnCertainedDetection UnCertainedDetection;

	/*Called when the agent forgets the player and gives you list of points to search on*/
	UPROPERTY(BlueprintAssignable, Category = "Perception Settings")
	    FOnAgentForget OnAgentForget;

	/*the tag used to check wheather this is a target or just something else that can block the trace channel, and by default this component only works for detecting one target at a time with this tag , make sure to put the tag used in the player here*/
	UPROPERTY(VisibleAnywhere, Category = "Perception Settings")
		FName SensingTag;
	/*draw the sensing sphere ?*/
	UPROPERTY(EditAnywhere, Category = "Perception Debug Settings")
		bool SensingDebug;

	/*should draw the line of sight or not*/
	UPROPERTY(EditAnywhere, Category = "Perception Debug Settings")
		bool VisionDebug;

	/*Draw Hiding Point Location and direction after creating them*/
	UPROPERTY(EditAnywhere, Category = "Perception Debug Settings")
		bool PointGenerationDebug;

	/*Should draw debug shape on the last seen location?*/
	UPROPERTY(EditAnywhere, Category = "Perception Debug Settings")
		bool DebugLastSeen;

	UPROPERTY(EditAnywhere, Category = "Perception Debug Settings")
		bool OnScreenDebugs;

	/*The duration of the last seen debug shape in the world before it get removed*/
	UPROPERTY(EditAnywhere, Category = "Perception Debug Settings", meta = (EditCondition = "DebugLastSeen", EditConditionHides))
		float LastSeenDebugDrawTime;

	/*how far the cone can see player*/
	UPROPERTY(EditAnywhere, Category = "Vision Settings")
		float VisionConeLength;

	/*the angle of the cone*/
	UPROPERTY(EditAnywhere, Category = "Vision Settings")
		float VisionConeAngle;

	/*the time that the player gets when he is in the line of sight of the agent before the agent can detect so the player get some time to hide*/
	UPROPERTY(EditAnywhere, Category = "Awareness Settings")
		float AwarenessDelay;

	/*the percentage from the awareness delay if passed and never reached the full awareness it will call the uncertained detection event */
	UPROPERTY(EditAnywhere, Category = "Awareness Settings")
		float UncertainedPercent;

	/*this is the distance that if the player was closer to the agent than , then we should not have any doubt that we've seen the player , cuz it was so close we directly detect*/
	UPROPERTY(EditAnywhere, Category = "Awareness Settings")
		float ForceFullAwarenessDistance;

	/*the time needed for the agent to forget the player , it start to count this time when he loses sight of the player , and rest the value on line of sight regained so it will recount from the start*/
	UPROPERTY(EditAnywhere, Category = "Awareness Settings")
		float MaxMemory;

	UPROPERTY(EditAnywhere, Category = "Awareness Settings")
		float ReactionDelay;

	//** ABOVE  WAS ALL USER SETTINGS NOW WE GET UNDER THE HOOD VARIABLES FOR CORE SYSTEMS ** // 

	/*the value of awareness , this indicate the percentage between the time while the agent started to look at the player and the time remaining before the spot , used for cosmatics only , like a small widget to warn the player*/
	UPROPERTY(BlueprintReadOnly, Category = "Awareness Settings")
		float AgentAwareness;

	/*to decide if to use the partolling system i made for you or you can leave this false and use your own logic*/
	UPROPERTY(EditAnywhere, Category = "Arrows Patrolling Settings")
		bool UseArrowsPatrol;

	/*what the logics to use when the agent is not aware of the player, the garding logics will use the spawn location as a garding point so it will return to stand there after losing sight*/
	UPROPERTY(EditAnywhere, Category = "Arrows Patrolling Settings", meta = (EditCondition = "UseArrowsPatrol", EditConditionHides))
		EAgentBehaviour AgentBehaviour; 

	/*the distance to use to generate the random point */
	UPROPERTY(EditAnywhere, Category = "Arrows Patrolling Settings", meta = (EditCondition = "UseArrowsPatrol && AgentBehaviour == EAgentBehaviour::RandomPoint", EditConditionHides))
		float PointDistance;

	/*Spline Actor to define the patrolling path , if the agent is not aware of the player or recently lost sight and forgot about him*/
	UPROPERTY(EditAnywhere, Category = "Arrows Patrolling Settings" , meta = (EditCondition = "AgentBehaviour == EAgentBehaviour::PathPatrolling && UseArrowsPatrol", EditConditionHides))
	    class AArrowsPathAI* PatrolingPath;


	//Properties NO EXPOSE :
	UPROPERTY()
	bool bPlayerWasSpotted;

	UPROPERTY()
	bool bHasLineOfSight;

	UPROPERTY()
	bool bInVisionCone;

	UPROPERTY()
	bool bRecentlyForgot;

	UPROPERTY()
	bool bNearAgentDetects;

	UPROPERTY()
	bool bCalledByReport;//now it is just used to define where the function is being called , if it was the calculation or forced results from reports near by

	UPROPERTY()
	bool bBusyWithLines;

	UPROPERTY()
	float ReportLineOfSightCounter;

	/*if a near by agent recenlt regained sight and played the sound and i now regained so i dont need to say the sounds and make it feel doublcate*/
	UPROPERTY()
    bool RecenlyOtherAgentRegained;

	UPROPERTY()
	FTransform EscapeTransform;

	UPROPERTY()
	FTimerHandle AwarenessTimeHandler;

	UPROPERTY()
	FTimerHandle ForgetTimeHandler;

	UPROPERTY()
	FTimerHandle RecentForgotHandler;

	UPROPERTY()
	FTimerHandle DelayedReactionHandler;

	UPROPERTY()
	ACharacter* DivinePlayerRef;

	UPROPERTY(BlueprintReadWrite)
	TArray<FDetectedObstacles> PointsData;


	//Functions NO EXPOSE : 
	UFUNCTION()
	void DebugPerception();

	UFUNCTION()
	void PerceptionUpdate();//will be replaced with the new refactored preception update function

	// new logics function
	UFUNCTION()
	void NewPerceptionUpdate();

	UFUNCTION()
	bool TargetInVision();

	UFUNCTION()
	void OnAgentPerceptionUpdate();

	UFUNCTION()
	bool HasLineOfSight();

	// end of new logics
	UFUNCTION()
	void AwarenessTimer();

	UFUNCTION()
	void ForgettingTimer();

	UFUNCTION()
	void ResetRecentlyForgot();

	UFUNCTION()
	void PrintDebugs(FString DebugMes, float duration = 0.0f);

	UFUNCTION()
	void ClearAndInvalidateTimer(FTimerHandle& Timer);

	UFUNCTION()
	void AgentPlaySound(TArray<USoundBase*> Sounds);

	UFUNCTION()
	void RadialCheck();

	UFUNCTION()
	void DrawLastSeen();

	/*to draw calculated hiding poinst */
	UFUNCTION()
	void DrawGeneratedPoints(FVector Location, FVector Normals, FColor _Color);

	/*called when uncertained detection happens so the agent will react only for it or the forget state, and the certained detection should be handled by the plugin user in his own behaviour tree logic*/
	UFUNCTION()
	void ReactToDetection(bool bCalledOnForgot);

	UFUNCTION()
	void DelayedUncertainedSearch();

	UFUNCTION()
	void DelayedReport();

	/*Finds if the currenly hit obstacle is already hit before so we check to see if we should add a point behind it or not*/
	UFUNCTION()
	bool FindStructArrayElementByMember(AActor* StructMemeber, TArray<FDetectedObstacles> _PointsData, int32& FoundIndex);

	/*Loops through all hit results for each ray trace and see possible hiding points*/
	UFUNCTION(BlueprintCallable)
	void LoopThroughRayHitResults(TArray<FHitResult> RayHitResults);

	/*this addes new entries if we hit new obstacle or we edit the number of hiding points behind previously hit obstacle*/
	UFUNCTION()
	void SetMemberInPoints(AActor* Obstacle, FVector Location, FVector Normals, float AcceptedDistance);

	/*Checks if Two points behind same obstacle are having the same wall direction so we ignore one of them if it was too close and add it if it has different wall direction*/
	UFUNCTION()
	bool PointNormalsCheck(TArray<FPoint> _Points, FRotator RotationToCheck, FVector PointLocation, float& Distance);

	UFUNCTION()
	TArray<FVector> CombineHidingPoints();

	/*this is called when we lose the player or when we detect the player , we tell the near by enemies about this information so they dont have to follow the preception logics for detection they get the info directly from the agent
	that managed to detect the player - change the call form the play sound delegate so that it wait for the previous sentance before saying the report so that we dont get overlapped voices*/
	UFUNCTION()
	void ReportNearAgentsWithDetection(bool bIsLost);

	/*if the agent was near by and got the report from the other agent, then this agent will reply to him as if he knows where the player is and the report was about losing the player then we force detection on the 
	agent that send the report and also play sounds like "he is here" and if the agent report was about finding the palyer then we force detectino in the agent that got the report and dont say the spot sound since the first agent 
	did it*/
	UFUNCTION()
	void RespondToNearAgentReport(UArrowsPerception* Instegator, bool bIsLost);


	/*this should be called when ever some agent says something , the near by enemies should not directly say things after him or say in the same time, there should be like 2 seconds delay*/
	UFUNCTION()
	void ReportedAwareness();

	UFUNCTION()
	void ReportRegainSight();

	UFUNCTION()
	void ResetRecentlyRegained();


	//AI Nutural Behaviours , these are going to be used if the user checked the "Use Arrows Patrol"

	//this will be called everytime the agent forget the player and returns to the nutural state, and then this function will select what behaviour to use 
	UFUNCTION()
	void AISelectBehaviour();

	UPROPERTY()
	FTransform GuardingLocation;

	/*used to test after movement completed to the last seen location if we are searching meaning we are testing the on movement compeleted after uncertained detection reaction or after forgetting and should start searching after reaching the last seen location*/
	UPROPERTY()
	bool ReactIsSearch;

	/*to prevent recursive calls for move to when we reach the guarding location cus the [on move to compelete] calls the [Agent move to ] to the guarding location and this crashes the engine due to infinite loop*/
	UPROPERTY()
	bool MovementRequest;

	/*points figured out by the radial check and tested to be vaild hiding points so now we make the ai move to each one of them and invistgate , these poinst are send with the [Forget Delegate] so the user 
	can do other stuff with them*/
	UPROPERTY()
	TArray<FVector> InvestigationPoints;

	UPROPERTY(BlueprintReadOnly)
	AAIController* AgentController;

	UFUNCTION()
	void GaurdingBehaviour();

	UFUNCTION(BlueprintCallable)
	void AgentMoveTo(FVector MoveToLocation);

	

	/*now i knew it XDD never use a UFUNCTION with a function with struct paramerters that are not decorated with USTRUCT macro or this will make your life so hard*/
	void OnAgentMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	//logics needed in my game it is not related to anyother system, but you can use this function and logics to set enemies to split into two groups , allowed to engage and wating to engage
	// my approace was to make an array with references for those who first saw the player so they fight first depending on a int variable that defines how many per combat are allowed to fight , so if it is set to one 
	//then only the first one will fight and others will wait and so on , there is a distance based turn switcher so closer enemies while the fights can take places so i need a function to swap the waiters and the allowed
	// i made it with wildcard types so i can use it for other types

	/*takes two arrays and one element to take from this one to that one and replaces the swapped one in the first array*/
	/*UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "Doshka Project")
	void SwapArrayElements(TArray<FProperty*>& Array1, TArray<FProperty*>& Array2, FProperty* Item);

	NAAAAH i think i'll leave this idea and just use a simple swapping logic i dont need this 
	
	*/ 
    
};
