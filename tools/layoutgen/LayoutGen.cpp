// layoutgen: compiles a .layout source file into a C++ table (BijoyLayout.inc).
//
//   layoutgen <input.layout> <output.inc> [--strict]
//
// Validation failures print "<file>(<line>): error: ..." and exit with code 1, so the
// build stops on any malformed or ambiguous row. --strict (release) also rejects rows
// whose source is "memory" (unconfirmed).

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct Row {
    int line = 0;
    std::uint8_t scan = 0;
    std::string layer;
    std::string kind;
    std::string source;
    std::vector<std::uint32_t> codepoints;
};

// PC/AT set-1 scan codes for the unshifted US key labels.
std::optional<std::uint8_t> scanForKey(std::string_view key) {
    static const std::map<std::string_view, int> kScan = {
        {"`", 0x29}, {"1", 0x02}, {"2", 0x03}, {"3", 0x04}, {"4", 0x05}, {"5", 0x06},
        {"6", 0x07}, {"7", 0x08}, {"8", 0x09}, {"9", 0x0A}, {"0", 0x0B}, {"-", 0x0C},
        {"=", 0x0D}, {"q", 0x10}, {"w", 0x11}, {"e", 0x12}, {"r", 0x13}, {"t", 0x14},
        {"y", 0x15}, {"u", 0x16}, {"i", 0x17}, {"o", 0x18}, {"p", 0x19}, {"[", 0x1A},
        {"]", 0x1B}, {"a", 0x1E}, {"s", 0x1F}, {"d", 0x20}, {"f", 0x21}, {"g", 0x22},
        {"h", 0x23}, {"j", 0x24}, {"k", 0x25}, {"l", 0x26}, {";", 0x27}, {"'", 0x28},
        {"\\", 0x2B}, {"z", 0x2C}, {"x", 0x2D}, {"c", 0x2E}, {"v", 0x2F}, {"b", 0x30},
        {"n", 0x31}, {"m", 0x32}, {",", 0x33}, {".", 0x34}, {"/", 0x35},
    };
    const auto it = kScan.find(key);
    if (it == kScan.end()) return std::nullopt;
    return static_cast<std::uint8_t>(it->second);
}

std::optional<std::string_view> mapName(std::string_view value,
                                        const std::map<std::string_view, std::string_view>& names) {
    const auto it = names.find(value);
    if (it == names.end()) return std::nullopt;
    return it->second;
}

const std::map<std::string_view, std::string_view> kLayers = {
    {"normal", "KeyLayer::Normal"}, {"shift", "KeyLayer::Shift"},
    {"link", "KeyLayer::Link"},     {"link-shift", "KeyLayer::LinkShift"},
};

const std::map<std::string_view, std::string_view> kKinds = {
    {"consonant", "TokenKind::Consonant"},
    {"final", "TokenKind::Final"},
    {"link", "TokenKind::Link"},
    {"vowel-sign", "TokenKind::VowelSign"},
    {"vowel-sign-pre", "TokenKind::VowelSignPre"},
    {"independent-vowel", "TokenKind::IndependentVowel"},
    {"modifier", "TokenKind::Modifier"},
    {"phala", "TokenKind::Phala"},
    {"reph", "TokenKind::Reph"},
    {"digit", "TokenKind::Digit"},
    {"punct", "TokenKind::Punct"},
};

const std::map<std::string_view, std::string_view> kSources = {
    {"chart", "LayoutSource::Chart"},
    {"user-list", "LayoutSource::UserList"},
    {"memory", "LayoutSource::Memory"},
};

std::optional<std::uint32_t> parseCodepoint(std::string_view text) {
    if (text.size() < 6 || text.size() > 8 || text.substr(0, 2) != "U+") return std::nullopt;
    std::uint32_t value = 0;
    for (char c : text.substr(2)) {
        value <<= 4;
        if (c >= '0' && c <= '9') value |= static_cast<std::uint32_t>(c - '0');
        else if (c >= 'A' && c <= 'F') value |= static_cast<std::uint32_t>(c - 'A' + 10);
        else return std::nullopt;
    }
    // BMP only, no surrogates, no C0 controls.
    if (value < 0x20 || value > 0xFFFF || (value >= 0xD800 && value <= 0xDFFF)) return std::nullopt;
    return value;
}

class Parser {
public:
    explicit Parser(std::string path) : path_(std::move(path)) {}

    bool error(int line, const std::string& message) {
        std::fprintf(stderr, "%s(%d): error: %s\n", path_.c_str(), line, message.c_str());
        ++errors_;
        return false;
    }

    bool parse(std::istream& in, bool strict) {
        std::string text;
        int lineNo = 0;
        std::map<std::pair<std::uint8_t, std::string>, int> seen;
        while (std::getline(in, text)) {
            ++lineNo;
            if (!text.empty() && text.back() == '\r') text.pop_back();
            // '#' starts a comment except when it is the key field itself (not a valid key
            // anyway, so a leading '#' is always a comment).
            if (const auto hash = text.find('#'); hash != std::string::npos) text.erase(hash);

            std::istringstream fields(text);
            std::vector<std::string> f;
            for (std::string field; fields >> field;) f.push_back(field);
            if (f.empty()) continue;
            if (f.size() < 5) {
                error(lineNo, "expected: <key> <layer> <kind> <source> <codepoint>...");
                continue;
            }
            if (f.size() > 7) {
                error(lineNo, "at most 3 codepoints per row");
                continue;
            }

            Row row;
            row.line = lineNo;
            const auto scan = scanForKey(f[0]);
            if (!scan) { error(lineNo, "unknown key '" + f[0] + "'"); continue; }
            row.scan = *scan;
            if (!mapName(f[1], kLayers)) { error(lineNo, "unknown layer '" + f[1] + "'"); continue; }
            if (!mapName(f[2], kKinds)) { error(lineNo, "unknown kind '" + f[2] + "'"); continue; }
            if (!mapName(f[3], kSources)) { error(lineNo, "unknown source '" + f[3] + "'"); continue; }
            row.layer = f[1];
            row.kind = f[2];
            row.source = f[3];

            bool ok = true;
            for (std::size_t i = 4; i < f.size(); ++i) {
                const auto cp = parseCodepoint(f[i]);
                if (!cp) { ok = error(lineNo, "invalid codepoint '" + f[i] + "' (need U+XXXX, BMP)"); break; }
                row.codepoints.push_back(*cp);
            }
            if (!ok) continue;

            if (strict && row.source == "memory") {
                error(lineNo, "unconfirmed row (source=memory) rejected in --strict mode");
                continue;
            }
            const auto key = std::make_pair(row.scan, row.layer);
            if (const auto it = seen.find(key); it != seen.end()) {
                error(lineNo, "duplicate key '" + f[0] + "' in layer '" + row.layer +
                                  "' (first defined on line " + std::to_string(it->second) + ")");
                continue;
            }
            seen.emplace(key, lineNo);
            rows_.push_back(std::move(row));
        }
        if (rows_.empty() && errors_ == 0) error(lineNo, "layout has no rows");
        return errors_ == 0;
    }

    const std::vector<Row>& rows() const { return rows_; }

private:
    std::string path_;
    std::vector<Row> rows_;
    int errors_ = 0;
};

std::string render(const std::vector<Row>& rows, const std::string& inputPath) {
    std::ostringstream out;
    // File name only: keeps the output identical across machines (reproducible builds).
    const auto slash = inputPath.find_last_of("/\\");
    const std::string fileName = slash == std::string::npos ? inputPath : inputPath.substr(slash + 1);
    out << "// GENERATED by layoutgen from " << fileName << ". Do not edit.\n"
        << "inline constexpr LayoutEntry kBijoyEntries[] = {\n";
    char buf[32];
    for (const Row& r : rows) {
        std::snprintf(buf, sizeof buf, "0x%02X", r.scan);
        out << "    {" << buf << ", " << *mapName(r.layer, kLayers) << ", "
            << *mapName(r.source, kSources) << ", {" << *mapName(r.kind, kKinds) << ", "
            << r.codepoints.size() << ", {";
        for (std::size_t i = 0; i < r.codepoints.size(); ++i) {
            std::snprintf(buf, sizeof buf, "%su'\\x%04X'", i ? ", " : "", r.codepoints[i]);
            out << buf;
        }
        out << "}}},  // line " << r.line << "\n";
    }
    out << "};\n";
    return out.str();
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4 || (argc == 4 && std::string_view(argv[3]) != "--strict")) {
        std::fprintf(stderr, "usage: layoutgen <input.layout> <output.inc> [--strict]\n");
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
    Parser parser(inputPath);
    if (!parser.parse(in, strict)) return 1;

    const std::string generated = render(parser.rows(), inputPath);

    // Rewrite only when the content changes, so unchanged layouts do not trigger rebuilds.
    std::string existing;
    if (std::ifstream old(outputPath, std::ios::binary); old) {
        existing.assign(std::istreambuf_iterator<char>(old), std::istreambuf_iterator<char>());
    }
    if (existing != generated) {
        std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
        out << generated;
        if (!out) {
            std::fprintf(stderr, "%s: error: cannot write output\n", outputPath.c_str());
            return 1;
        }
    }
    std::printf("layoutgen: %zu rows -> %s\n", parser.rows().size(), outputPath.c_str());
    return 0;
}
