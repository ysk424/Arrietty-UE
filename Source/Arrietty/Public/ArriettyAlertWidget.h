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
    virtual void NativeConstruct() override;
    void SetAlert(const FString& Message);

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> AlertText;
};
