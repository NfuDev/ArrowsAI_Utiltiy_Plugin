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

	/*the trace channel used for registering targets*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
	    TEnumAsByte<ETraceTypeQuery> SensingChannel;

	/*Array Of sound to say when the agent spots the player*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundWave*> SpottedSound;

	/*Array of sounds to say when the agent regain the line of sight before forgetting the player */
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundWave*> RegainSightSounds;

	/*Array of sounds to say after the agent forget about the player*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundWave*> ForgotSounds;

	/*Array Of sounds to say when the agent is not ceratin about seeing the player , it will play when the agent nearly spotted the player*/
	UPROPERTY(EditAnywhere, Category = "Perception Settings")
		TArray<USoundWave*> UnCertainedSounds;

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
	FTransform EscapeTransform;

	UPROPERTY()
	FTimerHandle AwarenessTimeHandler;

	UPROPERTY()
	FTimerHandle ForgetTimeHandler;

	UPROPERTY()
	FTimerHandle RecentForgotHandler;

	UPROPERTY()
	ACharacter* DivinePlayerRef;

	UPROPERTY(BlueprintReadWrite)
	TArray<FDetectedObstacles> PointsData;

	//Functions NO EXPOSE : 
	UFUNCTION()
	void DebugPerception();

	UFUNCTION()
	void PerceptionUpdate();

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
	void AgentPlaySound(TArray<USoundWave*> Sounds);

	UFUNCTION()
	void RadialCheck();

	UFUNCTION()
	void DrawLastSeen();

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

	//AI Nutural Behaviours , these are going to be used if the user checked the "Use Arrows Patrol"

	//this will be called everytime the agent forget the player and returns to the nutural state, and then this function will select what behaviour to use 
	UFUNCTION()
	void AISelectBehaviour();

	UPROPERTY()
	FTransform GuardingLocation;

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

	

 /*   UFUNCTION()
	void OnAgentMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);*/
};
