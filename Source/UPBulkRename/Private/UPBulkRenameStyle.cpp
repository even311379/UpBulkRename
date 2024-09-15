// Copyright 2024 PufStudio. All Rights Reserved.

#include "UPBulkRenameStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FUPBulkRenameStyle::StyleInstance = nullptr;

void FUPBulkRenameStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FUPBulkRenameStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FUPBulkRenameStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("UPBulkRenameStyle"));
	return StyleSetName;
}

const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FUPBulkRenameStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("UPBulkRenameStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("UpBulkRename")->GetBaseDir() / TEXT("Resources"));

	FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(DEFAULT_FONT("Regular", 11))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black);

	Style->Set("Default", FTextBlockStyle(NormalText));
	Style->Set("a", FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FLinearColor(0.05, 0.05, 1.0)));
	Style->Set("r", FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FLinearColor(1.0, 0.05, 0.05))
		.SetStrikeBrush(*FAppStyle::Get().GetBrush("DefaultTextUnderline"))
		);
	
	Style->Set("SmallIcon", new IMAGE_BRUSH(TEXT("Icon64"), Icon20x20));
	return Style;
}

void FUPBulkRenameStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FUPBulkRenameStyle::Get()
{
	return *StyleInstance;
}
