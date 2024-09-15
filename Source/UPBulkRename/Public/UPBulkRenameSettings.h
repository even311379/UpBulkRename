// Copyright 2024 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UPBulkRenameSettings.generated.h"

/**
 * 
 */
UCLASS(config=EditorSettings)
class UPBULKRENAME_API UUPBulkRenameSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Config, Category="Perforce")
	bool bAllowPerforceFix;

	UPROPERTY(EditAnywhere, Config, Category="Perforce|Connection", meta=(EditCondition="bAllowPerforceFix", EditConditionHides))
	FString Port;
	UPROPERTY(EditAnywhere, Config, Category="Perforce|Connection", meta=(EditCondition="bAllowPerforceFix", EditConditionHides))
	FString User;
	UPROPERTY(EditAnywhere, Config, Category="Perforce|Connection", meta=(EditCondition="bAllowPerforceFix", PasswordField=true, EditConditionHides))
	FString Password;
	UPROPERTY(EditAnywhere, Config, Category="Perforce|Connection", meta=(EditCondition="bAllowPerforceFix", EditConditionHides))
	FString Workspace;
};
