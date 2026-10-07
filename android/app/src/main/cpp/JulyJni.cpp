// JNI bridge between the Kotlin keyboard (NativeEngine.kt) and the shared C++ engine.
//
// A composer is owned by Kotlin through an opaque handle. Each edit returns one packed
// string so a key costs a single JNI call and a single Java allocation:
//     '1' or '0' (eaten), commit text, U+0000, composition text
// Engine text never contains U+0000, so the separator is unambiguous.

#include <jni.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>

#include "july/engine/Composer.h"
#include "july/engine/Layout.h"

namespace {

july::Composer* composerFrom(jlong handle) noexcept {
    return reinterpret_cast<july::Composer*>(static_cast<std::intptr_t>(handle));
}

jstring pack(JNIEnv* env, const july::EditResult& result) noexcept {
    std::array<jchar, 2 + 2 * july::kTextCapacity> buffer{};
    std::size_t n = 0;
    buffer[n++] = result.eaten ? u'1' : u'0';
    for (char16_t c : result.commit.view()) buffer[n++] = c;
    buffer[n++] = u'\0';
    for (char16_t c : result.composition.view()) buffer[n++] = c;
    return env->NewString(buffer.data(), static_cast<jsize>(n));
}

// Combining signs are shown on a dotted circle (U+25CC), as on printed Bijoy charts.
bool needsDottedCircle(july::TokenKind kind) noexcept {
    switch (kind) {
    case july::TokenKind::VowelSign:
    case july::TokenKind::VowelSignPre:
    case july::TokenKind::Modifier:
    case july::TokenKind::Phala:
    case july::TokenKind::Link:
        return true;
    default:
        return false;
    }
}

} // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_org_julybangla_keyboard_NativeEngine_create(JNIEnv*, jclass, jboolean classic) {
    july::ComposerOptions options;
    options.encoding = classic ? july::OutputEncoding::Classic : july::OutputEncoding::Unicode;
    auto* composer = new (std::nothrow) july::Composer(options);
    return static_cast<jlong>(reinterpret_cast<std::intptr_t>(composer));
}

JNIEXPORT void JNICALL
Java_org_julybangla_keyboard_NativeEngine_destroy(JNIEnv*, jclass, jlong handle) {
    delete composerFrom(handle);
}

JNIEXPORT jstring JNICALL
Java_org_julybangla_keyboard_NativeEngine_pressKey(JNIEnv* env, jclass, jlong handle,
                                                   jint scan, jboolean shift) {
    if (scan < 0 || scan >= static_cast<jint>(july::kScanCodeCount)) return pack(env, {});
    return pack(env, composerFrom(handle)->pressKey(static_cast<std::uint16_t>(scan), shift));
}

JNIEXPORT jstring JNICALL
Java_org_julybangla_keyboard_NativeEngine_backspace(JNIEnv* env, jclass, jlong handle) {
    return pack(env, composerFrom(handle)->backspace());
}

JNIEXPORT void JNICALL
Java_org_julybangla_keyboard_NativeEngine_reset(JNIEnv*, jclass, jlong handle) {
    composerFrom(handle)->reset();
}

// Key-cap label for a Bijoy key, or "" when the key is not mapped in that layer.
JNIEXPORT jstring JNICALL
Java_org_julybangla_keyboard_NativeEngine_keyLabel(JNIEnv* env, jclass, jint scan,
                                                   jboolean shift) {
    std::array<jchar, 4> buffer{};
    std::size_t n = 0;
    if (scan >= 0 && scan < static_cast<jint>(july::kScanCodeCount)) {
        const july::Token& token = july::lookupBijoyKey(
            static_cast<std::uint16_t>(scan), shift ? july::KeyLayer::Shift : july::KeyLayer::Normal);
        if (token.kind != july::TokenKind::None) {
            if (needsDottedCircle(token.kind)) buffer[n++] = 0x25CC;
            for (char16_t c : token.text()) buffer[n++] = c;
        }
    }
    return env->NewString(buffer.data(), static_cast<jsize>(n));
}

} // extern "C"
