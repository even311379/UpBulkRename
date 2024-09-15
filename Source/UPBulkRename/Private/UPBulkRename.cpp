// Copyright 2024 PufStudio. All Rights Reserved.

#include "UPBulkRename.h"
#include "ContentBrowserModule.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#include "UPBulkRenameSettings.h"
#include "UPBulkRenameStyle.h"


#define LOCTEXT_NAMESPACE "FUPBulkRenameModule"

void FUPBulkRenameModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FUPBulkRenameStyle::Initialize();
	FUPBulkRenameStyle::ReloadTextures();

	// content browser
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedPaths>& ContentBrowserPathMenuExtenders =
		ContentBrowserModule.GetAllPathViewContextMenuExtenders();
	ContentBrowserPathMenuExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this,
		&FUPBulkRenameModule::PathMenuExtender));

	TArray<FContentBrowserMenuExtender_SelectedAssets>& ContentBrowserAssetMenuExtenders =
		ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	ContentBrowserAssetMenuExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this,
		&FUPBulkRenameModule::AssetMenuExtender));

	// level editor
	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TArray <FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors> & LevelEditorMenuExtenders =
	LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	LevelEditorMenuExtenders.Add(FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::
		CreateRaw(this, &FUPBulkRenameModule::LevelEditorMenuExtender));

	// Register plugin setting
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
		    "Editor",
		    "Plugins",
		    "UP Bulk Rename",
		    LOCTEXT("PluginSettingsLabel", "UP Bulk Rename"),
		    LOCTEXT("PluginSettingsDescription", "Configure UP Bulk Rename plugin"),
		    GetMutableDefault<UUPBulkRenameSettings>()
		);
	}
}

void FUPBulkRenameModule::ShutdownModule()
{
}

TSharedRef<FExtender> FUPBulkRenameModule::PathMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> Extender(new FExtender()); 
	if (SelectedPaths.IsEmpty()) return Extender;
	Extender->AddMenuExtension(
		FName("Delete"),
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda([=, this](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.AddMenuEntry
			(
				LOCTEXT("MenuTitle", "UP Bulk Rename"), 
				LOCTEXT("MenuTooltip", "Bulk rename folders with perforce fix"),
				FSlateIcon(FUPBulkRenameStyle::GetStyleSetName(), "SmallIcon"),
				FExecuteAction::CreateLambda([=, this]()
				{
					SUPDialog::Open(SelectedPaths, true);
				})
			);
		})
		);
	return Extender;
}

TSharedRef<FExtender> FUPBulkRenameModule::AssetMenuExtender(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender(new FExtender()); 
	if (SelectedAssets.IsEmpty()) return Extender;
	Extender->AddMenuExtension(
		FName("Delete"),
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda([=, this](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.AddMenuEntry
			(
				LOCTEXT("MenuTitle", "UP Bulk Rename"), 
				LOCTEXT("MenuTooltip", "Bulk rename assets with perforce fix"),
				FSlateIcon(FUPBulkRenameStyle::GetStyleSetName(), "SmallIcon"),
				FExecuteAction::CreateLambda([=, this]()
				{
					TArray<FString> SelectedAssetPaths;
					for (auto Asset : SelectedAssets)
					{
						SelectedAssetPaths.Add(Asset.GetObjectPathString());
					}
					SUPDialog::Open(SelectedAssetPaths);
				})
			);
		})
	);
	return Extender;
}


TSharedRef<FExtender> FUPBulkRenameModule::LevelEditorMenuExtender(
	const TSharedRef<FUICommandList> UICommandList, const TArray<AActor*> SelectedActors)
{
	
	TSharedRef<FExtender> MenuExtender = MakeShareable(new FExtender());

	if (SelectedActors.Num()>0)
	{
		MenuExtender->AddMenuExtension(
			FName("ActorOptions"),
			EExtensionHook::Before,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([=, this](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("ActorActionTitle", "UP Bulk Rename"),
					LOCTEXT("ActorActionTooltip", "Bulk rename actors"),
					FSlateIcon(FUPBulkRenameStyle::GetStyleSetName(), "SmallIcon"),
					FExecuteAction::CreateLambda([=, this]()
					{
						SUPDialog::Open(SelectedActors);
					})
				);
			})
		);
	}
	return MenuExtender;
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUPBulkRenameModule, UPBulkRename)