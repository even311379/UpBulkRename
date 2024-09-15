// Copyright 2024 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"


namespace ENewNameValidStatus
{
	enum Type
	{
		Valid,
		NoChange,
		InValid,
		Duplicated
	};
}

class FRenameActionData : public TSharedFromThis<FRenameActionData>
{
public:
	FRenameActionData(AActor* InActor)
		:TargetActor(InActor)
	{
		TempFinalPath = InActor->GetActorLabel();
	}
	FRenameActionData(const FString& Original, bool* InIsFolder, bool* InShouldShowPath)
		: OriginalFullPath(Original), TempFinalPath(Original), IsFolder(InIsFolder), ShouldShowPath(InShouldShowPath)
	{}
	FString OriginalFullPath;
	FString TempFinalPath;
	FString RenamingPath;
	bool* IsFolder = nullptr;
	bool* ShouldShowPath = nullptr;
	bool IsNewPathDuplicated = false;
	void CheckNewPathDuplicated();
	FString GetFinalPath();
	void ResetFinalFullPath();
	ENewNameValidStatus::Type GetNewNameStatus() const;
	const FSlateBrush* GetStatusIcon() const;
	
	AActor* TargetActor = nullptr;
};

DECLARE_MULTICAST_DELEGATE(FOnEditRenameOperations)

class SRenameRow : public SMultiColumnTableRow<TSharedPtr<FRenameActionData>>
{
public:
	SLATE_BEGIN_ARGS(SRenameRow) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, TSharedPtr<FRenameActionData> InData,
		const TSharedRef<STableViewBase>& InOwnerTableView, class SUPDialog* InParentDialog);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
private:
	TSharedPtr<FRenameActionData> MyData;
	TSharedPtr<class SWidgetSwitcher> NewPathSwitcher;
	class SUPDialog* ParentDialog = nullptr;
	TArray<TSharedRef<class ITextDecorator>> MyDecorator;
};

class UPBULKRENAME_API SUPDialog : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SUPDialog)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const TArray<FString>& InSelectedPaths, const bool InIsFolder);
	void Construct(const FArguments& InArgs, const TArray<AActor*>& InSelectedActors);
	void CreateDialogContent();

	static void Open(const TArray<FString>& InSelectedPaths, bool InIsFolder = false);
	static void Open(const TArray<AActor*> InSelectedActors);
	
	// UI params
	bool IsActor = false;
	bool IsFolder = false;
	bool ShouldEditPath = false;
	FOnEditRenameOperations StartEdit;
	FOnEditRenameOperations EndEdit;
private:
	// UI events
	void ResetOperations();
	void ResetAll();
	void ApplyOperations();
	void ExecuteRename();
	void ActorsRename();
	void FoldersRename();
	void AssetsRename();
	bool CanExecuteRename() const;
	void UpdateOperationEditPreview();

	TSharedPtr<STreeView<TSharedPtr<FRenameActionData>>> ListWidget;
	TArray<TSharedPtr<FRenameActionData>> RenameData;
	
	// UI params
	bool bApplyPerforceFix = false;
	FName CurrentSourceControlProvider;
	int NumCharactersToRemoveAtBegin = 0;
	int NumCharactersToRemoveAtEnd = 0;
	FText Prefix;
	FText Suffix;
	FText SearchText;
	FText ReplaceText;
	bool bIgnoreCase = false;
	bool bUseRegex = false;
	bool bStartOperationEdit = false;

};
