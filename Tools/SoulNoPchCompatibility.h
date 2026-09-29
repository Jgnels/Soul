#pragma once
// Build-only forced includes for the admitted machine's -NoPCH workflow.
// These vendor modules ordinarily obtain both declarations through Engine PCH.
// Keep donor/plugin sources read-only; this introduces no definitions/behavior.
#if __has_include("GameFramework/Actor.h")
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#endif
#if __has_include("Framework/Commands/InputChord.h")
#include "Framework/Commands/InputChord.h"
#endif
#if __has_include("Layout/Visibility.h")
#include "Layout/Visibility.h"
#endif
