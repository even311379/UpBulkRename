// Copyright 2024 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UPBulkRenameUtility.generated.h"

/**
 * TODO: make this c++ utility only! at last
 * current some template bp assets still depend on this file
 */
UCLASS()
class UPBULKRENAME_API UUPBulkRenameUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="PerforceRename")
	static void StopSourceControl();
	
	UFUNCTION(BlueprintCallable, Category="PerforceRename")
	static void StartSourceControl_Perforce();
	
	UFUNCTION(BlueprintCallable, Category="PerforceRename")
	static void RunScript(FString ScriptName, FString Parameters, bool bWait = false);
	
	UFUNCTION(BlueprintCallable, Category="PerforceRename")
	static void SystemRename(const FString& OldName, const FString& NewName);

	static FString MakeSysPath(const FString& Path, bool IsFolder = false, bool IsPkgName = false);
	
	UFUNCTION(BlueprintCallable, Category="PerforceRename|Notifications")
	static void NotifySuccess(FText Message, FString HyperLinkURL = "", FText HyperLinkText = FText::GetEmpty());
	
	UFUNCTION(BlueprintCallable, Category="PerforceRename|Notifications")
	static void NotifyWarning(FText Message, FString HyperLinkURL = "", FText HyperLinkText = FText::GetEmpty());
	
	UFUNCTION(BlueprintCallable, Category="PerforceRename|Notifications")
	static void NotifyError(FText Message, FString HyperLinkURL = "", FText HyperLinkText = FText::GetEmpty());
	
private:
	static void Notify(const FName NotifyCategory, const FText& Message, const FString& HyperLinkURL, const FText& HyperLinkText);
	
};
