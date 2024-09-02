// By Kamil Krawczyk

#include "Building.h"
#include "Components/ArrowComponent.h"
#include "Components/DecalComponent.h"
#include "Technology.h"
#include "Command.h"
#include "Unit.h"
#include "ResourcesOwnerComponent.h"
#include "StratyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"

// Sets default values
ABuilding::ABuilding()
{
 	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health Component"));
	
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Point"));
	SpawnPoint->SetupAttachment(RootComponent);

	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("Selection Decal"));
	DecalComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	Unselect();
}

void ABuilding::Select()
{
	if (DecalComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("No selection decal"));
		return;
	}

	DecalComponent->SetVisibility(true);
}

void ABuilding::Unselect()
{
	if (DecalComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("No selection decal"));
		return;
	}

	DecalComponent->SetVisibility(false);
}

void ABuilding::ResearchTechnology(TSubclassOf<UTechnology> TechnologyClass)
{
	// Check if no technology is currently researched
	if (CurrentlyResearched != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Yellow, TEXT("Another technology is being researched"));
		return;
	}
	
	TObjectPtr<UTechnology> ResearchedTech = Cast<UTechnology>(TechnologyClass->GetDefaultObject());
	if (ResearchedTech == nullptr) return;

	FTimerDelegate Delegate;
	Delegate.BindUFunction(this, "OnResearchCompleted", ResearchedTech);
	
	OnResearchStarted.Broadcast(ResearchedTech);
	GetWorld()->GetTimerManager().SetTimer(ResearchTimerHandle, Delegate, ResearchedTech->GetResearchDuration(), false);
	
	// Set the class variable
	CurrentlyResearched = ResearchedTech;
}

void ABuilding::CancelResearch(TSubclassOf<UTechnology> TechnologyClass)
{
	GetWorld()->GetTimerManager().ClearTimer(ResearchTimerHandle);
	
	if (TechnologyClass != NULL)
	{
		TObjectPtr<UTechnology> Technology = Cast<UTechnology>(TechnologyClass->GetDefaultObject());
		if (Technology != nullptr)
		{
			OnResearchStopped.Broadcast(Technology, false);
		}
	}

	RemoveCurrentActionFromQueue();
}

void ABuilding::OnUnitRecruited(TSubclassOf<AUnit> UnitClass)
{
	UWorld* World = GEngine->GetWorldFromContextObject(GetWorld(), EGetWorldErrorMode::LogAndReturnNull);
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.Owner = Owner;
	
	APawn* Pawn = World->SpawnActor<APawn>
		(UnitClass, SpawnPoint->GetComponentLocation(), FRotator(0, 0, 0), ActorSpawnParams);
	Pawn->SpawnDefaultController();

	RemoveCurrentActionFromQueue();
}

void ABuilding::RecruitUnit(TSubclassOf<class AUnit> UnitClass)
{
	AUnit* Unit = Cast<AUnit>(UnitClass->GetDefaultObject());
	if (Unit == nullptr) return;

	FTimerDelegate Delegate; // Delegate to bind function with parameters
	Delegate.BindUFunction(this, "OnUnitRecruited", UnitClass);

	//OnResearchStarted.Broadcast(ResearchedTech);
	GetWorld()->GetTimerManager().SetTimer(ResearchTimerHandle, Delegate, Unit->GetRecruitmentTime(), false);

	// Set the class variable
	CurrentlyResearched = UnitClass;
}

void ABuilding::OnResearchCompleted(UTechnology* TechnologyClass)
{
	if (TechnologyClass == nullptr) return;

	TArray<AActor*> AffectedActors;
	for (TSubclassOf<AActor> AffectedClass : TechnologyClass->GetAffectedClasses())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AffectedClass, AffectedActors);

		for (AActor* AffectedActor : AffectedActors)
		{
			TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = Cast<UAbilitySystemComponent>
				(AffectedActor->GetComponentByClass(UAbilitySystemComponent::StaticClass()));

			if (AbilitySystemComponent == nullptr) break;
			AbilitySystemComponent->ApplyGameplayEffectToSelf(TechnologyClass, 0, AbilitySystemComponent->MakeEffectContext());
		}
	}

	// TO CHANGE LATER ON TO SUIT RECRUIMENT AS WELL
	TObjectPtr<UTechnology> ResearchedTechnology = Cast<UTechnology>(CurrentlyResearched);

	RemoveCurrentActionFromQueue();
	OnResearchStopped.Broadcast(ResearchedTechnology, true);

	// Triggers event in players queue
	AStratyPlayerController* PlayerController = Cast<AStratyPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayerController->RemoveCurrentActionFromQueue();

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Technology researched"));
	CurrentlyResearched = nullptr;
}

void ABuilding::CancelRecruiting(TSubclassOf<class AUnit> UnitClass)
{
}

void ABuilding::AddToActionsQueue(UCommand* Action, TSubclassOf<UObject> ProcessedObjectClass)
{
	// Check if the player can afford the research
	AStratyPlayerController* PlayerController = Cast<AStratyPlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PlayerController == nullptr) return;

	AUnit* Unit = Cast<AUnit>(ProcessedObjectClass->GetDefaultObject());
	if (Unit != nullptr)
	{
		if (!PlayerController->GetResourcesOwnerComponent()->Purchase(Unit->GetCost())) return;
	}
	else
	{
		TObjectPtr<UTechnology> Technology = Cast<UTechnology>(ProcessedObjectClass->GetDefaultObject());
		if (Technology != nullptr)
		{
			if (!PlayerController->GetResourcesOwnerComponent()->Purchase(Technology->GetCost())) return;
		}
		else return;
	}
	
	// If there's no action pending in the queue, start the research automatically
	if (ActionsQueue.Num() == 0)
	{
		Action->StartResearch();
	}

	ActionsQueue.Add(Action);

	// Add action to the array of all researched actions by the player
	PlayerController->AddToActionsQueue(Action);
}

void ABuilding::RemoveCurrentActionFromQueue()
{
	if (ActionsQueue.Num() == 0) return;

	ActionsQueue.RemoveAt(0);
	CurrentlyResearched = nullptr;

	// If there is next action pending in the queue, start the research automatically
	if (ActionsQueue.Num() != 0)
	{
		TObjectPtr<UCommand> Action = ActionsQueue[0];
		Action->StartResearch();
	}
}