#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LocalizationManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLanguageChanged, const FString&, LanguageCode);

/**
 * Runtime JSON localization service owned by the GameInstance.
 * Keys are flattened with dots (for example "ui.examine") and values are FText
 * so callers never depend on the JSON representation.
 */
UCLASS()
class BLOODBORNE2_API ULocalizationManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Loads a supported language and broadcasts OnLanguageChanged on success. */
    UFUNCTION(BlueprintCallable, Category="Localization")
    bool SetLanguage(const FString& LanguageCode);

    /** Returns the translated value, falling back to English and then the key itself. */
    UFUNCTION(BlueprintPure, Category="Localization")
    FText GetLocalizedText(FName Key) const;

    UFUNCTION(BlueprintPure, Category="Localization")
    FString GetCurrentLanguage() const { return CurrentLanguage; }

    UFUNCTION(BlueprintPure, Category="Localization")
    static TArray<FString> GetSupportedLanguages();

    UPROPERTY(BlueprintAssignable, Category="Localization")
    FOnLanguageChanged OnLanguageChanged;

private:
    bool LoadDictionary(const FString& LanguageCode, TMap<FName, FText>& OutDictionary) const;
    static void FlattenJsonObject(const TSharedPtr<class FJsonObject>& Object,
        const FString& Prefix, TMap<FName, FText>& OutDictionary);
    static FString NormalizeLanguageCode(const FString& LanguageCode);
    static FString GetLocalizationFilePath(const FString& LanguageCode);

    UPROPERTY(Transient)
    FString CurrentLanguage = TEXT("en");

    TMap<FName, FText> ActiveDictionary;
    TMap<FName, FText> EnglishFallbackDictionary;
};
