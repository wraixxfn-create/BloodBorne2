#include "Core/LocalizationManager.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Internationalization/Internationalization.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogRuntimeLocalization, Log, All);

void ULocalizationManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!LoadDictionary(TEXT("en"), EnglishFallbackDictionary))
    {
        UE_LOG(LogRuntimeLocalization, Error, TEXT("English fallback dictionary could not be loaded."));
    }

    const FString Culture = FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName();
    if (!SetLanguage(Culture))
    {
        SetLanguage(TEXT("en"));
    }
}

void ULocalizationManager::Deinitialize()
{
    ActiveDictionary.Empty();
    EnglishFallbackDictionary.Empty();
    Super::Deinitialize();
}

bool ULocalizationManager::SetLanguage(const FString& LanguageCode)
{
    const FString NormalizedCode = NormalizeLanguageCode(LanguageCode);
    if (!GetSupportedLanguages().Contains(NormalizedCode))
    {
        UE_LOG(LogRuntimeLocalization, Warning, TEXT("Unsupported language '%s'."), *LanguageCode);
        return false;
    }

    TMap<FName, FText> LoadedDictionary;
    if (!LoadDictionary(NormalizedCode, LoadedDictionary))
    {
        return false; // Keep the currently active, known-good dictionary.
    }

    ActiveDictionary = MoveTemp(LoadedDictionary);
    CurrentLanguage = NormalizedCode;
    OnLanguageChanged.Broadcast(CurrentLanguage);
    return true;
}

FText ULocalizationManager::GetLocalizedText(const FName Key) const
{
    if (const FText* Value = ActiveDictionary.Find(Key))
    {
        return *Value;
    }
    if (const FText* Fallback = EnglishFallbackDictionary.Find(Key))
    {
        UE_LOG(LogRuntimeLocalization, Verbose, TEXT("Missing '%s' in '%s'; using English."),
            *Key.ToString(), *CurrentLanguage);
        return *Fallback;
    }

    UE_LOG(LogRuntimeLocalization, Warning, TEXT("Missing localization key '%s'."), *Key.ToString());
    return FText::FromName(Key);
}

TArray<FString> ULocalizationManager::GetSupportedLanguages()
{
    return { TEXT("en"), TEXT("it") };
}

bool ULocalizationManager::LoadDictionary(const FString& LanguageCode,
    TMap<FName, FText>& OutDictionary) const
{
    const FString FilePath = GetLocalizationFilePath(LanguageCode);
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogRuntimeLocalization, Error, TEXT("Unable to read localization file: %s"), *FilePath);
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogRuntimeLocalization, Error, TEXT("Invalid localization JSON: %s"), *FilePath);
        return false;
    }

    TMap<FName, FText> ParsedDictionary;
    FlattenJsonObject(RootObject, FString(), ParsedDictionary);
    if (ParsedDictionary.IsEmpty())
    {
        UE_LOG(LogRuntimeLocalization, Error, TEXT("Localization file is empty: %s"), *FilePath);
        return false;
    }

    OutDictionary = MoveTemp(ParsedDictionary);
    UE_LOG(LogRuntimeLocalization, Log, TEXT("Loaded %d '%s' strings."), OutDictionary.Num(), *LanguageCode);
    return true;
}

void ULocalizationManager::FlattenJsonObject(const TSharedPtr<FJsonObject>& Object,
    const FString& Prefix, TMap<FName, FText>& OutDictionary)
{
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
    {
        const FString Key = Prefix.IsEmpty() ? Pair.Key : Prefix + TEXT(".") + Pair.Key;
        if (Pair.Value->Type == EJson::Object)
        {
            FlattenJsonObject(Pair.Value->AsObject(), Key, OutDictionary);
        }
        else if (Pair.Value->Type == EJson::String)
        {
            OutDictionary.Add(FName(*Key), FText::FromString(Pair.Value->AsString()));
        }
        else
        {
            UE_LOG(LogRuntimeLocalization, Warning, TEXT("Ignoring non-string localization value '%s'."), *Key);
        }
    }
}

FString ULocalizationManager::NormalizeLanguageCode(const FString& LanguageCode)
{
    FString Result = LanguageCode.ToLower();
    Result.ReplaceInline(TEXT("_"), TEXT("-"));
    int32 SeparatorIndex = INDEX_NONE;
    if (Result.FindChar(TEXT('-'), SeparatorIndex))
    {
        Result.LeftInline(SeparatorIndex);
    }
    return Result;
}

FString ULocalizationManager::GetLocalizationFilePath(const FString& LanguageCode)
{
    return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Localization"), LanguageCode + TEXT(".json"));
}
