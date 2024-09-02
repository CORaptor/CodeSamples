// By Kamil Krawczyk

// A building class for an Age-of-Empires-inspired real-time strategy

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HealthComponent.h"
#include "Selectable.h"
#include "Engine/EngineTypes.h"
#include "Building.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResearchStarted, class UTechnology*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FResearchStopped, class UTechnology*, Item, bool, bFinished);

UCLASS(Abstract, Blueprintable)
class ABuilding : public APawn, public ISelectable
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABuilding();

	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent;

	void Select();

	void Unselect();

	const FSelectionData GetSelectionData() { return SelectionData; }

	UFUNCTION(BlueprintCallable, Category = "Technologies")
	void ResearchTechnology(TSubclassOf<class UTechnology> TechnologyClass);

	UFUNCTION(Category = "Technologies")
	void OnResearchCompleted(UTechnology* TechnologyClass);

	UFUNCTION(BlueprintCallable, Category = "Technologies")
	void CancelResearch(TSubclassOf<class UTechnology> TechnologyClass);

	UFUNCTION(Category = "Units")
	void OnUnitRecruited(TSubclassOf<AUnit> UnitClass);
	
	UFUNCTION(BlueprintCallable, Category = "Units")
	void RecruitUnit(TSubclassOf<class AUnit> UnitClass);

	UFUNCTION(BlueprintCallable, Category = "Units")
	void CancelRecruiting(TSubclassOf<class AUnit> UnitClass);

	UFUNCTION(BlueprintCallable)
	void AddToActionsQueue(class UCommand* Action, TSubclassOf<UObject> ProcessedObjectClass);

	void RemoveCurrentActionFromQueue();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable)
	FResearchStarted OnResearchStarted;
	
	UPROPERTY(BlueprintAssignable)
	FResearchStopped OnResearchStopped;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<class AUnit>> RecruitedUnits;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<class UTechnology>> AvailableTechnologies;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<class UArrowComponent> SpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Selection")
	TObjectPtr<class UDecalComponent> DecalComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSelectionData SelectionData;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UObject> CurrentlyResearched;

	UPROPERTY(BlueprintReadOnly)
	TArray<class UCommand*> ActionsQueue;

	// Keeps the handle to currently research technology's timer
	FTimerHandle ResearchTimerHandle;
};
