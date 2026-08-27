// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArriettyAlertWidget.generated.h"

class UTextBlock;

UCLASS()
class ARRIETTY_API UArriettyAlertWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetAlert(const FString& Message);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> AlertText;
};
