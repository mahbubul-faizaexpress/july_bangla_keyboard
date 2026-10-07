#include "JulyTest.h"
#include "july/engine/InputMode.h"

using july::InputMode;

TEST_CASE("nextMode cycles English -> Unicode -> Classic -> English") {
    CHECK(july::nextMode(InputMode::English) == InputMode::Unicode);
    CHECK(july::nextMode(InputMode::Unicode) == InputMode::Classic);
    CHECK(july::nextMode(InputMode::Classic) == InputMode::English);
}

TEST_CASE("modeFromInt round-trips and rejects unknown values") {
    for (auto m : {InputMode::English, InputMode::Unicode, InputMode::Classic})
        CHECK(july::modeFromInt(static_cast<int>(m)) == m);
    CHECK(july::modeFromInt(-1) == InputMode::English);
    CHECK(july::modeFromInt(3) == InputMode::English);
}

TEST_CASE("modeLabel returns exact UTF-8 labels") {
    CHECK(july::modeLabel(InputMode::English) == u8"EN");
    // বাংলা = U+09AC U+09BE U+0982 U+09B2 U+09BE
    CHECK(july::modeLabel(InputMode::Unicode) == u8"বাংলা");
    // ক্লাসিক = U+0995 U+09CD U+09B2 U+09BE U+09B8 U+09BF U+0995
    CHECK(july::modeLabel(InputMode::Classic) == u8"ক্লাসিক");
}
