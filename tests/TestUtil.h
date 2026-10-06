#pragma once

// Shared helpers for engine tests: golden-file parsing and driving the composer from
// key labels.

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "july/engine/Composer.h"

namespace july::test {

// Scan codes for letter key labels, restated independently of layoutgen's table.
inline std::uint16_t scanForLetter(char lower) {
    static constexpr char kRows[3][11] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    static constexpr std::uint16_t kRowStart[3] = {0x10, 0x1E, 0x2C};
    for (int row = 0; row < 3; ++row) {
        const std::string_view keys = kRows[row];
        if (const auto pos = keys.find(lower); pos != std::string_view::npos)
            return static_cast<std::uint16_t>(kRowStart[row] + pos);
    }
    return 0;
}

inline std::u16string utf8ToUtf16(std::string_view s) {
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto b = static_cast<unsigned char>(s[i]);
        char32_t cp = 0;
        std::size_t len = 1;
        if (b < 0x80) cp = b;
        else if ((b >> 5) == 0x6) { cp = b & 0x1F; len = 2; }
        else if ((b >> 4) == 0xE) { cp = b & 0x0F; len = 3; }
        else return u"<invalid utf-8>";  // BMP-only test data
        if (i + len > s.size()) return u"<truncated utf-8>";
        for (std::size_t k = 1; k < len; ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        out.push_back(static_cast<char16_t>(cp));
        i += len;
    }
    return out;
}

// "U+0995 U+09CD ..." for readable failure messages.
inline std::string codepoints(std::u16string_view text) {
    std::string out;
    char buf[12];
    for (char16_t c : text) {
        std::snprintf(buf, sizeof buf, "%sU+%04X", out.empty() ? "" : " ", static_cast<unsigned>(c));
        out += buf;
    }
    return out;
}

struct GoldenCase {
    int line = 0;
    std::string keys;        // space-separated labels; uppercase = Shift
    std::u16string expected;
};

// Reads keys<TAB>expected<TAB>note rows; '#' lines and blank lines are skipped.
inline std::vector<GoldenCase> readGolden(const char* path) {
    std::vector<GoldenCase> cases;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "cannot open golden file %s\n", path);
        return cases;
    }
    int lineNo = 0;
    for (std::string line; std::getline(in, line);) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto tab1 = line.find('\t');
        const auto tab2 = line.find('\t', tab1 == std::string::npos ? tab1 : tab1 + 1);
        if (tab1 == std::string::npos || tab2 == std::string::npos) {
            std::fprintf(stderr, "%s(%d): malformed golden row\n", path, lineNo);
            cases.push_back({lineNo, "<malformed>", u"<malformed>"});
            continue;
        }
        cases.push_back({lineNo, line.substr(0, tab1),
                         utf8ToUtf16(std::string_view(line).substr(tab1 + 1, tab2 - tab1 - 1))});
    }
    return cases;
}

struct KeyPress {
    std::uint16_t scan = 0;
    bool shift = false;
};

// US key label -> scan code + Shift: letters (uppercase = Shift), digits, the shifted
// number-row symbols, and ` ~ ' " \ |.
inline KeyPress keyFor(char label) {
    if (label >= 'a' && label <= 'z') return {scanForLetter(label), false};
    if (label >= 'A' && label <= 'Z') return {scanForLetter(static_cast<char>(label - 'A' + 'a')), true};
    static constexpr std::string_view kDigits = "1234567890";
    static constexpr std::string_view kShiftedDigits = "!@#$%^&*()";
    if (const auto i = kDigits.find(label); i != std::string_view::npos) return {static_cast<std::uint16_t>(0x02 + i), false};
    if (const auto i = kShiftedDigits.find(label); i != std::string_view::npos) return {static_cast<std::uint16_t>(0x02 + i), true};
    switch (label) {
    case '`': return {0x29, false};
    case '~': return {0x29, true};
    case '\'': return {0x28, false};
    case '"': return {0x28, true};
    case '\\': return {0x2B, false};
    case '|': return {0x2B, true};
    default: return {0, false};
    }
}

// Types space-separated key labels into the composer, then commits; returns all text the
// host would have committed.
inline std::u16string typeKeys(Composer& composer, std::string_view keys) {
    std::u16string text;
    for (const char c : keys) {
        if (c == ' ') continue;
        const KeyPress k = keyFor(c);
        text += composer.pressKey(k.scan, k.shift).commit.view();
    }
    text += composer.commitAll().commit.view();
    return text;
}

} // namespace july::test
