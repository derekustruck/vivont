// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"

// Declare log categories
DECLARE_LOG_CATEGORY_EXTERN(LogVivont, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVivontAudio, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVivontAPI, Log, All);

// Define log macros for clean logging
#define VIVONT_LOG(Verbosity, Format, ...) \
    UE_LOG(LogVivont, Verbosity, TEXT("%s: " Format), *FString(__FUNCTION__), ##__VA_ARGS__)

#define VIVONT_AUDIO_LOG(Verbosity, Format, ...) \
    UE_LOG(LogVivontAudio, Verbosity, TEXT("%s: " Format), *FString(__FUNCTION__), ##__VA_ARGS__)

#define VIVONT_API_LOG(Verbosity, Format, ...) \
    UE_LOG(LogVivontAPI, Verbosity, TEXT("%s: " Format), *FString(__FUNCTION__), ##__VA_ARGS__)

// Error log macros
#define VIVONT_ERROR(Context, Format, ...) \
    UE_LOG(LogVivont, Error, TEXT("[%s] " Format), TEXT(Context), ##__VA_ARGS__)

#define VIVONT_AUDIO_ERROR(Context, Format, ...) \
    UE_LOG(LogVivontAudio, Error, TEXT("[%s] " Format), TEXT(Context), ##__VA_ARGS__)

#define VIVONT_API_ERROR(Context, Format, ...) \
    UE_LOG(LogVivontAPI, Error, TEXT("[%s] " Format), TEXT(Context), ##__VA_ARGS__)

// Warning log macros
#define VIVONT_WARNING(Context, Format, ...) \
    UE_LOG(LogVivont, Warning, TEXT("[%s] " Format), TEXT(Context), ##__VA_ARGS__)

#define VIVONT_AUDIO_WARNING(Context, Format, ...) \
    UE_LOG(LogVivontAudio, Warning, TEXT("[%s] " Format), TEXT(Context), ##__VA_ARGS__)

#define VIVONT_API_WARNING(Context, Format, ...) \
    UE_LOG(LogVivontAPI, Warning, TEXT("[%s] " Format), TEXT(Context), ##__VA_ARGS__)

// Special log for missing animation data
#define VIVONT_MISSING_ANIM_DATA(AudioId, Format, ...) \
    UE_LOG(LogVivont, Warning, TEXT("[MISSING_ANIMATION_DATA] Audio ID: %u - " Format), AudioId, ##__VA_ARGS__)

// API tracking macros
#define VIVONT_API_REQUEST(Context, AudioSize) \
    UE_LOG(LogVivontAPI, Log, TEXT("[%s] Sending API request with %d bytes of audio data"), TEXT(Context), AudioSize)

#define VIVONT_API_RESPONSE(Context, ResponseCode, ContentSize) \
    UE_LOG(LogVivontAPI, Log, TEXT("[%s] Received API response: HTTP %d (%d bytes)"), TEXT(Context), ResponseCode, ContentSize)

// Audio tracking macros
#define VIVONT_AUDIO_RECEIVED(Context, AudioSize) \
    UE_LOG(LogVivontAudio, Log, TEXT("[%s] Received %d bytes of audio data"), TEXT(Context), AudioSize)

#define VIVONT_AUDIO_SENT(Context, AudioSize) \
    UE_LOG(LogVivontAudio, Log, TEXT("[%s] Sent %d bytes of audio data"), TEXT(Context), AudioSize)
