// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyAlertWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UArriettyAlertWidget::RebuildWidget()
{
    Initialize();
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return Super::RebuildWidget();
    }

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor(0.08f, 0.0f, 0.0f, 1.0f));
    Background->SetPadding(FMargin(18.0f, 10.0f));
    WidgetTree->RootWidget = Background;

    AlertText = WidgetTree->ConstructWidget<UTextBlock>();
    AlertText->SetText(FText::FromString(TEXT("ARRIETTY WARNING")));
    AlertText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.18f, 0.04f, 1.0f)));
    AlertText->SetJustification(ETextJustify::Center);
    AlertText->SetAutoWrapText(true);
    AlertText->SetShadowOffset(FVector2D(2.0f, 2.0f));
    AlertText->SetShadowColorAndOpacity(FLinearColor::Black);
    FSlateFontInfo Font = AlertText->GetFont();
    Font.Size = 42;
    Font.OutlineSettings.OutlineSize = 2;
    Font.OutlineSettings.OutlineColor = FLinearColor::Black;
    AlertText->SetFont(Font);
    Background->SetContent(AlertText);

    return Super::RebuildWidget();
}

void UArriettyAlertWidget::SetAlert(const FString& Message)
{
    if (AlertText)
    {
        AlertText->SetText(FText::FromString(Message));
    }
}
