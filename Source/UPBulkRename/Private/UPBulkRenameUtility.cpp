// Copyright 2024 PufStudio. All Rights Reserved.

#include "UPBulkRenameUtility.h"

#include "Developer/SourceControl/Private/SourceControlModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include <filesystem>

#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetSystemLibrary.h"

void UUPBulkRenameUtility::StopSourceControl()
{
	FSourceControlModule& SourceControlModule = FSourceControlModule::Get();
	SourceControlModule.SetProvider("None");
}

void UUPBulkRenameUtility::StartSourceControl_Perforce()
{
	FSourceControlModule& SourceControlModule = FSourceControlModule::Get();
	SourceControlModule.SetProvider("Perforce");
}

void UUPBulkRenameUtility::RunScript(FString ScriptName, FString Parameters, bool bWait)
{
	FString ScriptPath = IPluginManager::Get().FindPlugin(TEXT("UPBulkRename"))->GetBaseDir() + "/Scripts/" + ScriptName;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *(ScriptPath))
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
				*ScriptPath, *Parameters, false, false, false, nullptr, 0, nullptr, nullptr);
	if (ProcHandle.IsValid())
	{
        if (bWait)
            FPlatformProcess::WaitForProc(ProcHandle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("system proc Not working"))
	}
}

void UUPBulkRenameUtility::SystemRename(const FString& OldName, const FString& NewName)
{
	if (!std::filesystem::exists(TCHAR_TO_ANSI(*OldName)))
	{
		NotifyError(FText::FromString("Source file not exist"));
		return;
	}
	std::filesystem::path TargetPath = std::filesystem::path(TCHAR_TO_ANSI(*NewName)).parent_path();
	std::filesystem::create_directories(TargetPath);
	std::filesystem::rename(TCHAR_TO_ANSI(*OldName), TCHAR_TO_ANSI(*NewName));
}

FString UUPBulkRenameUtility::MakeSysPath(const FString& Path, bool IsFolder, bool IsPkgName)
{
	FString Out = UKismetSystemLibrary::GetProjectDirectory();
	FString Temp = Path.Mid(1, Path.Len());
	FString Left, Right;
	Temp.Split(TEXT("/"), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (Left == TEXT("Game"))
	{
		Out += "Content/" + Right;
	}
	else
	{
		Out += "Plugins/" + Left + "/Content/" + Right;
	}
	if (IsPkgName)
	{
		Out += TEXT(".uasset");
		return Out;
	}
	if (!IsFolder)
	{
		Out.Split(".", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		Out = Left + TEXT(".uasset");
	}
	return Out;
}

void UUPBulkRenameUtility::NotifySuccess(FText Message, FString HyperLinkURL,
                                         FText HyperLinkText)
{
	Notify(FName("Success"), Message, HyperLinkURL, HyperLinkText);
}

void UUPBulkRenameUtility::NotifyWarning(FText Message, FString HyperLinkURL, FText HyperLinkText)
{
	Notify(FName("Warning"), Message, HyperLinkURL, HyperLinkText);
}

void UUPBulkRenameUtility::NotifyError(FText Message, FString HyperLinkURL, FText HyperLinkText)
{
	Notify(FName("Error"), Message, HyperLinkURL, HyperLinkText);
}

void UUPBulkRenameUtility::Notify(const FName NotifyCategory, const FText& Message, const FString& HyperLinkURL,
                                      const FText& HyperLinkText)
{
	FNotificationInfo Info(Message);
	Info.ExpireDuration = 3.0f;
	
	if (NotifyCategory == FName("Success"))
	{
		Info.Image = FCoreStyle::Get().GetBrush("Icons.SuccessWithColor");
	}
	else if (NotifyCategory == FName("Warning"))
	{
		Info.Image = FCoreStyle::Get().GetBrush("Icons.WarningWithColor");
	}
	else if (NotifyCategory == FName("Error"))
	{
		Info.Image = FCoreStyle::Get().GetBrush("Icons.ErrorWithColor");
	}
	else
	{
		Info.Image = FCoreStyle::Get().GetBrush("Icons.InfoWithColor");
	}
	if (!HyperLinkURL.IsEmpty())
	{
		Info.HyperlinkText = HyperLinkText;
		Info.Hyperlink = FSimpleDelegate::CreateLambda([HyperLinkURL]()
		{
			FPlatformProcess::LaunchURL(*HyperLinkURL, nullptr, nullptr);
		});
	}
	
	FSlateNotificationManager::Get().AddNotification(Info);
}
