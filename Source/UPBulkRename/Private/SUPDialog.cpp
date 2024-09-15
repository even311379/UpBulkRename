// Copyright 2024 PufStudio. All Rights Reserved.

#include "SUPDialog.h"

#include "EditorAssetLibrary.h"
#include "SlateOptMacros.h"
#include "UPBulkRenameSettings.h"
#include "UPBulkRenameUtility.h"
#include "UPPerforceConnection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "UPBulkRenameStyle.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/SRichTextBlock.h"
#include <filesystem>

#include "Developer/SourceControl/Private/SourceControlModule.h"

#define LOCTEXT_NAMESPACE "UPDialog"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


void FRenameActionData::CheckNewPathDuplicated()
{
	std::filesystem::path SysPath = std::filesystem::path(TCHAR_TO_ANSI(*UUPBulkRenameUtility::MakeSysPath(GetFinalPath())));
	IsNewPathDuplicated = std::filesystem::exists(SysPath);
}

FString FRenameActionData::GetFinalPath()
{
	if (*IsFolder)
	{
		return TempFinalPath;
	}
	FString Left;
	if (*ShouldShowPath)
	{
		return TempFinalPath ;
	}
	// UE_LOG(LogTemp, Warning, TEXT("origin: %s, temp path: %s"), *OriginalFullPath, *TempFinalPath)
	OriginalFullPath.Split(TEXT("/"), &Left, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	return Left + "/" + TempFinalPath + "." + TempFinalPath;
}

void FRenameActionData::ResetFinalFullPath()
{
	if (TargetActor)
	{
		TempFinalPath = TargetActor->GetActorLabel();
		return;
	}
	if (*IsFolder)
	{
		TempFinalPath = OriginalFullPath;
		return;
	}
	FString OutRight, OutLeft;
	OriginalFullPath.Split(TEXT("."), &OutLeft, &OutRight, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (*ShouldShowPath)
	{
		TempFinalPath = OutLeft;
	}
	else
	{
		TempFinalPath = OutRight;
	}
	IsNewPathDuplicated = false;
}

ENewNameValidStatus::Type FRenameActionData::GetNewNameStatus() const
{
	if (TargetActor)
	{
		return TargetActor->GetActorLabel() == TempFinalPath?
			ENewNameValidStatus::NoChange : ENewNameValidStatus::Valid;
	}
	if (TempFinalPath.IsEmpty()) return ENewNameValidStatus::InValid;	
	if (IsNewPathDuplicated) return ENewNameValidStatus::Duplicated;
	FString Left, Right;
	FString InvalidChars = *IsFolder || *ShouldShowPath? "!@#$%^&*()=\\|]}[{'\";:?><`~.," : "!@#$%^&*()=\\|]}[{'\";:/?><`~.,";

	if (*IsFolder)
	{
		if (OriginalFullPath == TempFinalPath) return ENewNameValidStatus::NoChange;
	}
	else
	{
		OriginalFullPath.Split(".", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (Right == TempFinalPath) return ENewNameValidStatus::NoChange;
	}
	for (int i = 0; i < InvalidChars.Len(); i++)
	{
		if (TempFinalPath.Contains(InvalidChars.Mid(i, 1)))
			return ENewNameValidStatus::InValid;
	}
	return ENewNameValidStatus::Valid;
}

const FSlateBrush* FRenameActionData::GetStatusIcon() const
{
	switch (GetNewNameStatus()) {
	case ENewNameValidStatus::Valid:
		return FAppStyle::GetBrush("Icons.SuccessWithColor");
	case ENewNameValidStatus::NoChange:
		return FAppStyle::GetBrush("Icons.WarningWithColor");
	default: ;
		return FAppStyle::GetBrush("Icons.ErrorWithColor");
	}
}

void SRenameRow::Construct(const FArguments& InArgs, TSharedPtr<FRenameActionData> InData,
                           const TSharedRef<STableViewBase>& InOwnerTableView, SUPDialog* InParentDialog)
{
	MyData = InData;
	ParentDialog = InParentDialog;
	ParentDialog->StartEdit.AddLambda([this]()
	{
		NewPathSwitcher->SetActiveWidgetIndex(1);
	});
	ParentDialog->EndEdit.AddLambda([this]()
	{
		NewPathSwitcher->SetActiveWidgetIndex(0);
	});
	
	SMultiColumnTableRow<TSharedPtr<FRenameActionData>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SRenameRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	FString OutRight, OutLeft;
	MyData->OriginalFullPath.Split(TEXT("."), &OutLeft, &OutRight, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FText OriginAsset = FText::FromString(OutRight);
	FText OriginPath = FText::FromString(OutLeft);
	MyData->ResetFinalFullPath();
	
	TSharedPtr<SWidget> RowWidget = SNullWidget::NullWidget;
	if (InColumnName == FName("Old"))
	{
		RowWidget = SNew(SBox)
		.VAlign(VAlign_Center)
		.Padding(FMargin(10, 1, 0, 1))
		[
			SNew(STextBlock)
			.Text_Lambda([=, this]()
			{
				if (MyData->TargetActor)
					return FText::FromString(MyData->TargetActor->GetActorLabel());
				if (ParentDialog->IsFolder)
					return FText::FromString(MyData->OriginalFullPath);
				return ParentDialog->ShouldEditPath? OriginPath : OriginAsset;
			})
		];
	}
	else if (InColumnName == FName("New"))
	{
		RowWidget = SNew(SBox)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.Padding(FMargin(10, 1, 0, 1))
		[
			SAssignNew(NewPathSwitcher, SWidgetSwitcher)
			+SWidgetSwitcher::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this]()
					{
						return FText::FromString(MyData->TempFinalPath);
					})
					.OnTextChanged_Lambda([this](const FText& T)
					{
						MyData->TempFinalPath = T.ToString();
						if (!MyData->TargetActor)
							MyData->CheckNewPathDuplicated();
					})
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding(3.f, 0.f)
				[
					SNew(SImage)
					.ToolTipText_Lambda([this]()
					{
						switch (MyData->GetNewNameStatus())
						{
						case ENewNameValidStatus::Valid:
							return FText::FromString("Ok");
						case ENewNameValidStatus::NoChange:
							return FText::FromString("New name is the same. Will ignore.");
						case ENewNameValidStatus::InValid:
							return FText::FromString("Error! New name contains invalid character or it's and empty name");
						case ENewNameValidStatus::Duplicated:
							return FText::FromString("Error! Asset the same already exists!");
						}
						return FText::GetEmpty();
					})
					.Image_Lambda([this]()
					{
						return MyData->GetStatusIcon();
					})
					.DesiredSizeOverride(FVector2D(16,16))
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SRichTextBlock)
				.Text_Lambda([this] { return FText::FromString(MyData->RenamingPath);})
				.DecoratorStyleSet(&FUPBulkRenameStyle::Get())
				.TextStyle(FUPBulkRenameStyle::Get(), "Default")
			]
		];
	}
	return RowWidget.ToSharedRef();
}

void SUPDialog::Construct(const FArguments& InArgs, const TArray<FString>& InSelectedPaths, const bool InIsFolder)
{
	
	FSourceControlModule& SourceControlModule = FSourceControlModule::Get();
	CurrentSourceControlProvider = SourceControlModule.GetProvider().GetName();
	bApplyPerforceFix = GetMutableDefault<UUPBulkRenameSettings>()->bAllowPerforceFix &&
		CurrentSourceControlProvider == FName("Perforce");
	IsFolder = InIsFolder;
	RenameData.Empty();
	for (const FString & Path : InSelectedPaths)
	{
		RenameData.Add(MakeShareable(new FRenameActionData(Path, &IsFolder, &ShouldEditPath)));
	}

	CreateDialogContent();

}

void SUPDialog::Construct(const FArguments& InArgs, const TArray<AActor*>& InSelectedActors)
{
	RenameData.Empty();
	for (AActor* Actor : InSelectedActors)
	{
		RenameData.Add(MakeShareable(new FRenameActionData(Actor)));
	}
	IsActor = true;
	CreateDialogContent();
}

void SUPDialog::CreateDialogContent()
{
	SAssignNew(ListWidget, STreeView<TSharedPtr<FRenameActionData>>)
	.TreeItemsSource(&RenameData)
	.SelectionMode(ESelectionMode::Type::None)
	.HeaderRow(
		SNew(SHeaderRow)
		// Old
		+ SHeaderRow::Column(FName("Old"))
		.HeaderContentPadding(FMargin(10, 1, 0, 1))
		.DefaultLabel(LOCTEXT("RowName1", "Old"))
		.FillWidth(1.0)

		// New
		+ SHeaderRow::Column(FName("New"))
		.HeaderContentPadding(FMargin(10, 1, 0, 1))
		.DefaultLabel(LOCTEXT("RowName2", "New"))
		.FillWidth(1.0)

	)
	.OnGenerateRow_Lambda([this](TSharedPtr<FRenameActionData> InData, const TSharedRef<STableViewBase>& OutTable)
	{
		return SNew(SRenameRow, InData, OutTable, this);
	})
	.OnGetChildren_Lambda([](TSharedPtr<FRenameActionData> InItem, TArray<TSharedPtr<FRenameActionData>>& OutChildren)
	{
	})
	;

	TSharedPtr<SHorizontalBox> OperationButtonRow = SNew(SHorizontalBox);
	if (!IsFolder && !IsActor)
	{
		OperationButtonRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0, 0, 4, 0))
		[
			SNew(STextBlock)
			.Text(FText::FromString("Edit Path?"))
		];
		OperationButtonRow->AddSlot()
		.Padding(0.f, 0.f, 20.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]()
			{
				return ShouldEditPath? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
			{
				ShouldEditPath = NewState == ECheckBoxState::Checked;
				ResetAll();
			})
		];
	}
	OperationButtonRow->AddSlot()
	.AutoWidth()
	.Padding(0.f, 0.f, 20.f, 0.f)
	[
		SNew(SButton)
		.ToolTipText(FText::FromString("Reset operation values"))
		.OnPressed(this, &SUPDialog::ResetOperations)
		.Content()
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Icons.Refresh"))
		]
	];
	OperationButtonRow->AddSlot()
	.AutoWidth()
	[
		SNew(SButton)
		.ToolTipText(FText::FromString("Apply operation values to new name"))
		.Text(FText::FromString("Apply"))
		.OnPressed(this, &SUPDialog::ApplyOperations)
	];
	
	TSharedPtr<SVerticalBox> DialogContent = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SExpandableArea)
		.BorderImage(FAppStyle::Get().GetBrush("Brushes.Header"))
		.BodyBorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
		.HeaderPadding(FMargin(4.0f, 2.0f))
		.AreaTitle(FText::FromString("Operations"))
		.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
		.Padding(FMargin(20.f))
		.AllowAnimatedTransition(false)
		.BodyContent()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Remove"))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Begin"))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding(10.f, 0.f)
					[
						SNew(SSpinBox<int>)
						.MinValue(0)
						.MaxValue(32)
						.EnableWheel(true)
						.Justification(ETextJustify::Type::Center)
						.Value_Lambda([this](){ return NumCharactersToRemoveAtBegin; })
						.OnValueChanged_Lambda([this](int NewValue)
						{
							NumCharactersToRemoveAtBegin = FMath::Clamp(NewValue, 0, 32);
							UpdateOperationEditPreview();
						})
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("End"))
						.Font(FAppStyle::GetFontStyle("TinyText"))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding(10.f, 0.f, 0.f, 0.f)
					[
						SNew(SSpinBox<int>)
						.MinValue(0)
						.MaxValue(32)
						.EnableWheel(true)
						.Justification(ETextJustify::Type::Center)
						.Value_Lambda([this](){ return NumCharactersToRemoveAtEnd; })
						.OnValueChanged_Lambda([this](int NewValue)
						{
							NumCharactersToRemoveAtEnd = FMath::Clamp(NewValue, 0, 32);
							UpdateOperationEditPreview();
						})
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 5.f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Add"))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Begin"))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding(10.f, 0.f)
					[
						SNew(SEditableTextBox)
						.HintText(FText::FromString("prefix"))
						.Justification(ETextJustify::Type::Center)
						.Text_Lambda([this](){ return Prefix; })
						.OnTextChanged_Lambda([this](const FText& NewText)
						{
							Prefix = NewText;
							UpdateOperationEditPreview();
						})
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("End"))
						.Font(FAppStyle::GetFontStyle("TinyText"))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding(10.f, 0.f, 0.f, 0.f)
					[
						SNew(SEditableTextBox)
						.HintText(FText::FromString("suffix"))
						.Justification(ETextJustify::Type::Center)
						.Text_Lambda([this](){ return Suffix; })
						.OnTextChanged_Lambda([this](const FText& NewText)
						{
							Suffix = NewText;
							UpdateOperationEditPreview();
						})
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Search & replace"))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SEditableTextBox)
						.HintText(FText::FromString("search"))
						.Justification(ETextJustify::Type::Center)
						.Text_Lambda([this](){ return SearchText; })
						.OnTextChanged_Lambda([this](const FText& NewText)
						{
							SearchText = NewText;
							UpdateOperationEditPreview();
						})
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(8.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(" > "))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SEditableTextBox)
						.HintText(FText::FromString("replace"))
						.Justification(ETextJustify::Type::Center)
						.Text_Lambda([this](){ return ReplaceText; })
						.OnTextChanged_Lambda([this](const FText& NewText)
						{
							ReplaceText = NewText;
							UpdateOperationEditPreview();
						})
					]
					+SHorizontalBox::Slot()
					.Padding(10.f, 0.f, 2.f, 0.f)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Cc"))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.ToolTipText(FText::FromString("Ignore case?"))
						.IsChecked_Lambda([this](){ return bIgnoreCase ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](const ECheckBoxState NewState)
						{
							bIgnoreCase = NewState == ECheckBoxState::Checked;
							UpdateOperationEditPreview();
						})
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.f, 0.f, 2.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(".*"))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.ToolTipText(FText::FromString("Regex?"))
						.IsChecked_Lambda([this](){ return bUseRegex ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](const ECheckBoxState NewState)
						{
							bUseRegex = NewState == ECheckBoxState::Checked;
							UpdateOperationEditPreview();
						})
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 10.f, 0.f, 0.f)
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Center)
			[
				OperationButtonRow.ToSharedRef()
			]
		]
	]
	+ SVerticalBox::Slot()
	.Padding(0.f, 20.f, 0.f, 0.f)
	.FillHeight(1.f)
	[
		SNew(SBox)
		.MaxDesiredHeight(640.f)
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
			[
				ListWidget.ToSharedRef()
			]
		]
	];
	
	// window construct
	FString DialogTitle;
	if (IsActor)
	{
		DialogTitle = "UP Bulk Rename on " + FString::FromInt(RenameData.Num()) + "actors" ;
	} else if (IsFolder)
	{
		int N_Assets = 0;
		for (auto Data : RenameData)
		{
			N_Assets += UEditorAssetLibrary::ListAssets(Data->OriginalFullPath).Num();
		}
		DialogTitle = "UP Bulk Rename on folders (" + FString::FromInt(N_Assets) + " assets)";
	}
	else
	{
		DialogTitle = "UP Bulk Rename on " + FString::FromInt(RenameData.Num()) + "assets" ;
	}
	
	// SetSizingRule(ESizingRule::Autosized);
	// // TitleBar
	// bHasMaximizeButton = false;
	// bHasMaximizeButton = false;
	// bHasCloseButton = true;

	SWindow::Construct(SWindow::FArguments()
	.Title(FText::FromString(DialogTitle))
	.SizingRule(ESizingRule::Autosized)
	.SupportsMaximize(false)
	.SupportsMinimize(false)
	.HasCloseButton(true)
	[
		SNew( SBorder )
		.Padding( 4.f )
		.BorderImage( FAppStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SBox)
				.Padding(8)
				.MinDesiredWidth(640)
				[
					DialogContent.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Enable Perforce Fix?"))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(3, 0, 0, 0)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.ToolTipText(FText::FromString(R"(
Can not enable perforce fix?
Set source control provider as perforce and Enable allow perforce fix!
(editor preference->plugins->UPBulkRename)")
					)
					.IsEnabled_Lambda([this]()
					{
						
						return GetMutableDefault<UUPBulkRenameSettings>()->bAllowPerforceFix && CurrentSourceControlProvider == FName("Perforce");
					})
					.IsChecked_Lambda([this]()
					{
						return bApplyPerforceFix?
							ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						bApplyPerforceFix = NewState == ECheckBoxState::Checked;
					})
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString("Reset"))
					.ToolTipText(FText::FromString("Reset new name to old name"))
					.OnPressed(this, &SUPDialog::ResetAll)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(20.f, 0, 0, 0)
				[
					SNew(SButton)
					.Text(FText::FromString("Execute"))
					.OnPressed(this, &SUPDialog::ExecuteRename)
					.IsEnabled(this, &SUPDialog::CanExecuteRename)
				]
			]
		]
	]);
}

void SUPDialog::Open(const TArray<FString>& InSelectedPaths, bool InIsFolder)
{
	FSlateApplication::Get().AddWindow(
		SNew(SUPDialog, InSelectedPaths, InIsFolder)
	);
}

void SUPDialog::Open(const TArray<AActor*> InSelectedActors)
{
	FSlateApplication::Get().AddWindow(
		SNew(SUPDialog, InSelectedActors)
	);
}

void SUPDialog::ResetOperations()
{
	NumCharactersToRemoveAtBegin = 0;
	NumCharactersToRemoveAtEnd = 0;
	Prefix = FText::GetEmpty();
	Suffix = FText::GetEmpty();
	SearchText = FText::GetEmpty();
	ReplaceText = FText::GetEmpty();
	bIgnoreCase = false;
	bUseRegex = false;
	EndEdit.Broadcast();
	bStartOperationEdit = false;
}

void SUPDialog::ResetAll()
{
	for (auto Data : RenameData)
	{
		Data->ResetFinalFullPath();
	}
	ResetOperations();
	bStartOperationEdit = false;
}

void SUPDialog::ApplyOperations()
{
	for (auto& Data : RenameData)
	{
		if (NumCharactersToRemoveAtBegin)
		{
			int N_Remove = FMath::Clamp(NumCharactersToRemoveAtBegin, 0, Data->TempFinalPath.Len() - 1);
			Data->TempFinalPath.RemoveAt(0, N_Remove);
		}
		if (NumCharactersToRemoveAtEnd)
		{
			int N_Remove = FMath::Clamp(NumCharactersToRemoveAtEnd, 0, Data->TempFinalPath.Len() - 1);
			Data->TempFinalPath.RemoveAt(Data->TempFinalPath.Len() - N_Remove ,
				N_Remove);
		}
		if (NumCharactersToRemoveAtBegin && NumCharactersToRemoveAtEnd &&
			NumCharactersToRemoveAtBegin + NumCharactersToRemoveAtEnd >= Data->TempFinalPath.Len())
		{
			Data->TempFinalPath = "";
			continue;
		}
		if (!Prefix.IsEmpty())
		{
			Data->TempFinalPath = Prefix.ToString() + Data->TempFinalPath;
		}
		if (!Suffix.IsEmpty())
		{
			Data->TempFinalPath = Data->TempFinalPath + Suffix.ToString();
		}
		if (!SearchText.IsEmpty())
		{
			if (!bUseRegex)
			{
				ESearchCase::Type SearchCase = bIgnoreCase? ESearchCase::IgnoreCase : ESearchCase::CaseSensitive;
				Data->TempFinalPath.ReplaceInline(*SearchText.ToString(), *ReplaceText.ToString(),
					SearchCase);
			}
			else
			{
				FRegexMatcher Matcher(FRegexPattern(SearchText.ToString()), Data->TempFinalPath);
				while (Matcher.FindNext())
				{
					Data->TempFinalPath.ReplaceInline(*Matcher.GetCaptureGroup(0),
						*ReplaceText.ToString());
				}
			}
		}
		if (!IsActor)
			Data->CheckNewPathDuplicated();
	}
	ResetOperations();
}


void SUPDialog::ExecuteRename()
{
	if (IsActor)
	{
		ActorsRename();
		return;
	}
	RenameData.RemoveAll([](TSharedPtr<FRenameActionData> Data)
	{
		return Data->OriginalFullPath == Data->GetFinalPath();
	});
	
	if (IsFolder)
	{
		FoldersRename();
	}
	else
	{
		AssetsRename();
	}
}

void SUPDialog::ActorsRename()
{
	for (auto Data : RenameData)
	{
		Data->TargetActor->SetActorLabel(Data->TempFinalPath);
	}
	RequestDestroyWindow();
}

void SUPDialog::FoldersRename()
{
	if (!bApplyPerforceFix)
	{
		// rename folder
		for (auto Data : RenameData)
		{
			UEditorAssetLibrary::RenameDirectory(Data->OriginalFullPath, Data->GetFinalPath());
		}
		RequestDestroyWindow();
		return;
	}
	
	// start p4 connection
	ClientApi P4Api;
	FP4RecordSet Records;
	FP4ResultInfo ResultInfo;
	FUPPerforceConnection Connection = FUPPerforceConnection(P4Api, Records, ResultInfo);
	if (!Connection.Init())
	{
		UUPBulkRenameUtility::NotifyError(FText::FromString("Login perforce failed"));
		RequestDestroyWindow();
		return;
	}

	// prepare for edit
	TArray<FString> OriginalAssets; // standard obj path
	UUPBulkRenameUtility::StopSourceControl();
	for (auto Data : RenameData)
	{
		OriginalAssets.Append(UEditorAssetLibrary::ListAssets(Data->OriginalFullPath));
	}

	// collect references and dependencies and then use external script to batch checkout files
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FName> Referencers;
	TArray<FAssetIdentifier> AssetDependencies;
	TArray<FString> FilesToEdit;
	for (auto Asset : OriginalAssets)
	{
		FString Left;
		Asset.Split(TEXT("."), &Left, nullptr);
		FName PkgName = FName(Left);
		FilesToEdit.Add(UUPBulkRenameUtility::MakeSysPath(Asset));
		Referencers.Reset();
		AssetRegistry.GetReferencers(PkgName, Referencers);
		AssetDependencies.Reset();
		AssetRegistry.GetDependencies(FAssetIdentifier(PkgName), AssetDependencies);
		for (auto Referencer : Referencers)
		{
			FilesToEdit.Add(UUPBulkRenameUtility::MakeSysPath(Referencer.ToString(), false, true));
		}
		for (auto AssetDependency : AssetDependencies)
		{
			FilesToEdit.Add(UUPBulkRenameUtility::MakeSysPath(AssetDependency.ToString(), false, true));
		}
	}

	if (!Connection.RunCommand("edit", FilesToEdit))
	{
		UE_LOG(LogUPBulkRename, Error, TEXT("Checkout file failed..."))
		UUPBulkRenameUtility::NotifyError(FText::FromString("Can not checkout files..."));
		RequestDestroyWindow();
		return;
	}

	// rename folder
 	for (auto Data : RenameData)
 	{
 		UEditorAssetLibrary::RenameDirectory(Data->OriginalFullPath, Data->GetFinalPath());
 	}

	// run hack actions
	TArray<FString> RenamedAssets;
	for (auto Data : RenameData)
	{
		RenamedAssets.Append(UEditorAssetLibrary::ListAssets(Data->GetFinalPath()));
	}

	for (int i = 0; i < RenamedAssets.Num(); i++)
	{
		UUPBulkRenameUtility::SystemRename(UUPBulkRenameUtility::MakeSysPath(RenamedAssets[i]),
			UUPBulkRenameUtility::MakeSysPath(OriginalAssets[i]));
	}
	
	for (int i = 0; i < RenamedAssets.Num(); i++)
	{
		TArray<FString> MoveParams;
		MoveParams.Add(UUPBulkRenameUtility::MakeSysPath(OriginalAssets[i]));
		MoveParams.Add(UUPBulkRenameUtility::MakeSysPath(RenamedAssets[i]));
		if (!Connection.RunCommand(TEXT("move"), MoveParams))
		{
			FString Msg = FString::Printf(TEXT("Can not move file from %s to %s"), *MoveParams[0], *MoveParams[1]);
			UE_LOG(LogUPBulkRename, Error, TEXT("%s"), *Msg);
			UUPBulkRenameUtility::NotifyError(FText::FromString(Msg));
		}
	}
	UUPBulkRenameUtility::StartSourceControl_Perforce();
	// done?
	RequestDestroyWindow();
	UUPBulkRenameUtility::NotifySuccess(FText::FromString("UpBulk Rename on folder success!"));
}

void SUPDialog::AssetsRename()
{
	if (!bApplyPerforceFix)
	{
		for (auto Data : RenameData)
		{
			UEditorAssetLibrary::RenameAsset(Data->OriginalFullPath, Data->GetFinalPath());
		}
		RequestDestroyWindow();
		return;
	}

	ClientApi P4Api;
	FP4RecordSet Records;
	FP4ResultInfo ResultInfo;
	FUPPerforceConnection Connection = FUPPerforceConnection(P4Api, Records, ResultInfo);
	if (!Connection.Init())
	{
		UUPBulkRenameUtility::NotifyError(FText::FromString("Login perforce failed"));
		RequestDestroyWindow();
		return;
	}
	UUPBulkRenameUtility::StopSourceControl();
		
	// collect references and dependencies and then use external script to batch checkout files
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FName> Referencers;
	TArray<FAssetIdentifier> AssetDependencies;
	TArray<FString> FilesToEdit;
	for (auto Data : RenameData)
	{
		FString Left;
		Data->OriginalFullPath.Split(TEXT("."), &Left, nullptr);
		FName PkgName = FName(Left);
		FilesToEdit.Add(UUPBulkRenameUtility::MakeSysPath(Data->OriginalFullPath));
		Referencers.Reset();
		AssetRegistry.GetReferencers(PkgName, Referencers);
		AssetDependencies.Reset();
		AssetRegistry.GetDependencies(FAssetIdentifier(PkgName), AssetDependencies);
		for (auto Referencer : Referencers)
		{
			FilesToEdit.Add(UUPBulkRenameUtility::MakeSysPath(Referencer.ToString(), false, true));
		}
		for (auto AssetDependency : AssetDependencies)
		{
			FilesToEdit.Add(UUPBulkRenameUtility::MakeSysPath(AssetDependency.ToString(), false, true));
		}
	}
	if (!Connection.RunCommand(TEXT("edit"), FilesToEdit))
	{
		UE_LOG(LogUPBulkRename, Error, TEXT("Checkout file failed..."))
		UUPBulkRenameUtility::NotifyError(FText::FromString("Can not checkout files..."));
		RequestDestroyWindow();
		return;
	}

	
 	for (auto Data : RenameData)
 	{
 		UEditorAssetLibrary::RenameAsset(Data->OriginalFullPath, Data->GetFinalPath());
 	}
	
	for (auto Data : RenameData)
	{
		UUPBulkRenameUtility::SystemRename(UUPBulkRenameUtility::MakeSysPath(Data->GetFinalPath()),
			UUPBulkRenameUtility::MakeSysPath(Data->OriginalFullPath));
	}
	for (auto Data : RenameData)
	{
		TArray<FString> RenameParams = {
			UUPBulkRenameUtility::MakeSysPath(Data->OriginalFullPath),
			UUPBulkRenameUtility::MakeSysPath(Data->GetFinalPath())
		};
		
		if (!Connection.RunCommand(TEXT("move"), RenameParams))
		{
			FString Msg = FString::Printf(TEXT("Can not move file from %s to %s"), *RenameParams[0], *RenameParams[1]);
			UE_LOG(LogUPBulkRename, Error, TEXT("%s"), *Msg);
			UUPBulkRenameUtility::NotifyError(FText::FromString(Msg));
		}
	}

	UUPBulkRenameUtility::StartSourceControl_Perforce();
	UUPBulkRenameUtility::NotifySuccess(FText::FromString("UpBulk Rename on assets success!"));
	RequestDestroyWindow();
}

bool SUPDialog::CanExecuteRename() const
{
	if (bStartOperationEdit) return false;
	for (auto Data : RenameData)
	{
		if (Data->GetNewNameStatus() == ENewNameValidStatus::InValid ||
			Data->GetNewNameStatus() == ENewNameValidStatus::Duplicated)
			return false;
	}
	for (auto Data : RenameData)
	{
		if (Data->GetNewNameStatus() == ENewNameValidStatus::Valid)
			return true;
	}
	return false;
}

void SUPDialog::UpdateOperationEditPreview()
{
	if (!bStartOperationEdit)
	{
		StartEdit.Broadcast();
		bStartOperationEdit = true;
	}
	for (auto Data : RenameData)
	{
		Data->RenamingPath = Data->TempFinalPath;
		if (NumCharactersToRemoveAtBegin)
		{
			FString Temp = Data->RenamingPath;
			Data->RenamingPath = "<r>" + Temp.Mid(0, NumCharactersToRemoveAtBegin) + "</>" +
				Temp.Mid( NumCharactersToRemoveAtBegin);
		}
		if (NumCharactersToRemoveAtEnd)
		{
			FString Temp = Data->RenamingPath;
			Data->RenamingPath = Temp.Mid(0, Temp.Len() - NumCharactersToRemoveAtEnd) + "<r>" + 
				Temp.Mid( Temp.Len() - NumCharactersToRemoveAtEnd) + "</>";
		}
		if (NumCharactersToRemoveAtBegin && NumCharactersToRemoveAtEnd &&
			NumCharactersToRemoveAtBegin + NumCharactersToRemoveAtEnd >= Data->TempFinalPath.Len())
		{
			Data->RenamingPath = "<r>" + Data->TempFinalPath + "</>";
			continue;
		}
		if (!Prefix.IsEmpty())
		{
			Data->RenamingPath = "<a>" + Prefix.ToString() + "</>" + Data->RenamingPath;
		}
		if (!Suffix.IsEmpty())
		{
			Data->RenamingPath += "<a>" + Suffix.ToString() + "</>";
		}
		if (!SearchText.IsEmpty())
		{
			FString Temp = Data->RenamingPath;
			FString Left, Right;
			int TempN = 0;
			if (!bUseRegex)
			{
				ESearchCase::Type SearchCase = bIgnoreCase? ESearchCase::IgnoreCase : ESearchCase::CaseSensitive;
				while (Temp.Split(SearchText.ToString(), &Left, &Right, SearchCase))
				{
					if (TempN == 0)
						Data->RenamingPath = "";
					Data->RenamingPath += Left + "<r>" + SearchText.ToString() + "</>" + "<a>" + ReplaceText.ToString() + "</>";
					Temp = Right;
					TempN++;
				}
			}
			else
			{
				FRegexMatcher Matcher(FRegexPattern(SearchText.ToString()), Data->TempFinalPath);
				while (Matcher.FindNext())
				{
					if (TempN==0)
					{
						Data->RenamingPath = "";
					}
					TempN++;
					const int MatchStart = Matcher.GetMatchBeginning();
					const int MatchEnd   = Matcher.GetMatchEnding();
					FString Matched = Data->TempFinalPath.Mid(MatchStart, MatchEnd - MatchStart);
					Temp.Split(Matched, &Left, &Right, ESearchCase::CaseSensitive);
					Data->RenamingPath += Left + "<r>" + Matched + "</>" + "<a>" + ReplaceText.ToString() + "</>";
					Temp = Right;
				}
			}
			Data->RenamingPath += Right;
		}
	}
}




END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
