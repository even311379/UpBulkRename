// Copyright 2024 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SUPDialog.h"

class FToolBarBuilder;
class FMenuBuilder;

class FUPBulkRenameModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:	
	TSharedRef<FExtender> PathMenuExtender(const TArray<FString>& SelectedPaths);
	TSharedRef<FExtender> AssetMenuExtender(const TArray<FAssetData>& SelectedAssets);
	TSharedRef<FExtender> LevelEditorMenuExtender(const TSharedRef<FUICommandList> UICommandList,
		const TArray<AActor*> SelectedActors);

};

