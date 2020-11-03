// Mans Isaksson 2020

#pragma once
#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_GenerateThumbnail.generated.h"

UCLASS()
class THUMBNAILGENERATORNODES_API UK2Node_GenerateThumbnail : public UK2Node
{
	GENERATED_BODY()
private:
	FNodeTextCache CachedNodeTitle;

public:
	UK2Node_GenerateThumbnail();

	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void PostPlacedNewNode() override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface
	virtual bool IsNodePure() const override { return false; }
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual FText GetMenuCategory() const override;
	virtual FName GetCornerIcon() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool NodeCausesStructuralBlueprintChange() const { return true; }
	//~ End UK2Node Interface

	/** Create new pins to show properties on archetype */
	void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins = nullptr);

	/** Get the class that we are going to spawn, if it's defined as default value */
	UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch = nullptr) const;

	UEdGraphPin* GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch = nullptr) const;
	UEdGraphPin* GetThumbnailSettingsPin() const;
	UEdGraphPin* GetCacheMethodPin() const;
	UEdGraphPin* GetThenPin() const;
	UEdGraphPin* GetPreCaptureActorOutputPin() const;

private:
	void SetPinToolTip(class UEdGraphPin& MutatablePin, const FText& PinDescription) const;

	/** See if this is a spawn variable pin, or a 'default' pin */
	bool IsSpawnVarPin(UEdGraphPin* Pin) const;

	/** Refresh pins when class was changed */
	void OnClassPinChanged();

};
