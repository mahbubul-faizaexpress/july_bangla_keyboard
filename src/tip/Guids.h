#pragma once

#include <windows.h>

// Identity of the July Bangla Keyboard text service. These GUIDs are permanent: changing
// one breaks existing registrations and user settings.

namespace july::tip {

// COM class of the text service (InprocServer32).
inline constexpr GUID kClsidTextService = {
    0xAF6ABBB0, 0xA4E4, 0x4624, {0xB2, 0xF2, 0x08, 0x85, 0xAA, 0xAB, 0x8A, 0x7F}};

// TSF language profile under bn-BD.
inline constexpr GUID kGuidProfile = {
    0xCDFAB3B7, 0x9E90, 0x40DD, {0xB0, 0x4A, 0x19, 0x72, 0x23, 0xA2, 0x65, 0x28}};

// Global (desktop-wide, cross-process) compartment holding the InputMode as VT_I4.
inline constexpr GUID kGuidModeCompartment = {
    0x540A2534, 0xDA60, 0x41E0, {0xA9, 0x2C, 0xF5, 0x2B, 0x10, 0x13, 0x9F, 0x44}};

// Preserved key Ctrl+Alt+B (cycle English -> Unicode -> Classic).
inline constexpr GUID kGuidPreservedKeyCycle = {
    0x2B265375, 0x9ACB, 0x4D50, {0x88, 0x0B, 0xCC, 0xF6, 0xEB, 0x96, 0xFC, 0x44}};

inline constexpr LANGID kLangIdBanglaBangladesh = 0x0845;  // bn-BD
inline constexpr wchar_t kDisplayName[] = L"July Bangla Keyboard";

} // namespace july::tip
