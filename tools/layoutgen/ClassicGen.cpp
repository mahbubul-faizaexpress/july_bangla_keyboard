// classicgen: compiles a .classic glyph table into a sorted C++ table (ClassicTable.inc).
//
//   classicgen <input.classic> <output.inc> [--strict]
//
// Validation failures print "<file>(<line>): error: ..." and exit 1. --strict (release)
// rejects rows whose source is "converter" (not yet verified in the SutonnyMJ font).

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t kMaxKey = 6;
constexpr std::size_t kMaxGlyph = 4;

struct Row {
    int line = 0;
    std::u16string key;
    std::u16string glyph;  // cp1252-decoded
    std::string source;
};

// Windows-1252 decoding; undefined bytes (81 8D 8F 90 9D) and C0 controls are rejected.
std::optional<char16_t> cp1252ToUnicode(unsigned byte) {
    static constexpr char16_t k80[32] = {
        0x20AC, 0,      0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
        0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0,      0x017D, 0,
        0,      0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0,      0x017E, 0x0178,
    };
    if (byte < 0x20 || byte > 0xFF || byte == 0x7F) return std::nullopt;
    if (byte >= 0x80 && byte < 0xA0) {
        const char16_t c = k80[byte - 0x80];
        if (c == 0) return std::nullopt;
        return c;
    }
    return static_cast<char16_t>(byte);
}

std::optional<char16_t> parseCodepoint(std::string_view text) {
    if (text.size() != 6 || text.substr(0, 2) != "U+") return std::nullopt;
    unsigned value = 0;
    for (char c : text.substr(2)) {
        value <<= 4;
        if (c >= '0' && c <= '9') value |= static_cast<unsigned>(c - '0');
        else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned>(c - 'A' + 10);
        else return std::nullopt;
    }
    if (value < 0x20 || (value >= 0xD800 && value <= 0xDFFF)) return std::nullopt;
    return static_cast<char16_t>(value);
}

std::optional<unsigned> parseByte(std::string_view text) {
    if (text.size() != 2) return std::nullopt;
    unsigned value = 0;
    for (char c : text) {
        value <<= 4;
        if (c >= '0' && c <= '9') value |= static_cast<unsigned>(c - '0');
        else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned>(c - 'A' + 10);
        else return std::nullopt;
    }
    return value;
}

std::string hex(std::u16string_view s) {
    std::string out;
    char buf[16];
    for (char16_t c : s) {
        std::snprintf(buf, sizeof buf, "%su'\\x%04X'", out.empty() ? "" : ", ", static_cast<unsigned>(c));
        out += buf;
    }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4 || (argc == 4 && std::string_view(argv[3]) != "--strict")) {
        std::fprintf(stderr, "usage: classicgen <input.classic> <output.inc> [--strict]\n");
        return 2;
    }
    const std::string inputPath = argv[1];
    const std::string outputPath = argv[2];
    const bool strict = argc == 4;

    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "%s: error: cannot open input\n", inputPath.c_str());
        return 1;
    }

    int errors = 0;
    auto error = [&](int line, const std::string& message) {
        std::fprintf(stderr, "%s(%d): error: %s\n", inputPath.c_str(), line, message.c_str());
        ++errors;
    };

    std::vector<Row> rows;
    std::map<std::u16string, int> seen;
    std::string text;
    int lineNo = 0;
    while (std::getline(in, text)) {
        ++lineNo;
        if (!text.empty() && text.back() == '\r') text.pop_back();
        if (const auto hash = text.find('#'); hash != std::string::npos) text.erase(hash);
        std::istringstream fields(text);
        std::vector<std::string> f;
        for (std::string field; fields >> field;) f.push_back(field);
        if (f.empty()) continue;

        const auto arrow = std::find(f.begin(), f.end(), "->");
        if (arrow == f.end() || arrow == f.begin() || std::distance(arrow, f.end()) < 3) {
            error(lineNo, "expected: <code point>... -> <byte>... <source>");
            continue;
        }
        Row row;
        row.line = lineNo;
        row.source = f.back();
        if (row.source != "verified" && row.source != "converter") {
            error(lineNo, "unknown source '" + row.source + "'");
            continue;
        }
        bool ok = true;
        for (auto it = f.begin(); it != arrow && ok; ++it) {
            const auto cp = parseCodepoint(*it);
            if (!cp) { error(lineNo, "invalid code point '" + *it + "'"); ok = false; }
            else row.key.push_back(*cp);
        }
        for (auto it = std::next(arrow); it != std::prev(f.end()) && ok; ++it) {
            const auto byte = parseByte(*it);
            const auto unit = byte ? cp1252ToUnicode(*byte) : std::nullopt;
            if (!unit) { error(lineNo, "invalid or undefined cp1252 byte '" + *it + "'"); ok = false; }
            else row.glyph.push_back(*unit);
        }
        if (!ok) continue;
        if (row.key.size() > kMaxKey) { error(lineNo, "key longer than 6 code points"); continue; }
        if (row.glyph.empty() || row.glyph.size() > kMaxGlyph) { error(lineNo, "need 1-4 glyph bytes"); continue; }
        if (strict && row.source == "converter") {
            error(lineNo, "unverified row (source=converter) rejected in --strict mode");
            continue;
        }
        if (const auto it = seen.find(row.key); it != seen.end()) {
            error(lineNo, "duplicate key (first defined on line " + std::to_string(it->second) + ")");
            continue;
        }
        seen.emplace(row.key, lineNo);
        rows.push_back(std::move(row));
    }
    if (rows.empty() && errors == 0) error(lineNo, "table has no rows");
    if (errors != 0) return 1;

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.key < b.key; });

    const auto slash = inputPath.find_last_of("/\\");
    std::ostringstream out;
    out << "// GENERATED by classicgen from "
        << (slash == std::string::npos ? inputPath : inputPath.substr(slash + 1)) << ". Do not edit.\n"
        << "// Sorted by key for binary search.\n"
        << "inline constexpr ClassicEntry kSutonnyEntries[] = {\n";
    for (const Row& r : rows) {
        out << "    {{" << hex(r.key) << "}, " << r.key.size() << ", {" << hex(r.glyph) << "}, " << r.glyph.size()
            << ", " << (r.source == "verified" ? "true" : "false") << "},  // line " << r.line << "\n";
    }
    out << "};\n";
    const std::string generated = out.str();

    std::string existing;
    if (std::ifstream old(outputPath, std::ios::binary); old)
        existing.assign(std::istreambuf_iterator<char>(old), std::istreambuf_iterator<char>());
    if (existing != generated) {
        std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
        file << generated;
        if (!file) {
            std::fprintf(stderr, "%s: error: cannot write output\n", outputPath.c_str());
            return 1;
        }
    }
    std::printf("classicgen: %zu rows -> %s\n", rows.size(), outputPath.c_str());
    return 0;
}
