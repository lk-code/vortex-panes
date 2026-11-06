#include "core/notices_text.h"

namespace vp {

namespace {

bool startsWith(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

// Removes **bold** and `code` markers and rewrites [text](url) links.
std::string formatInline(std::string_view line) {
    std::string out;
    out.reserve(line.size());
    for (size_t i = 0; i < line.size(); ++i) {
        if (line.compare(i, 2, "**") == 0) {
            ++i;
            continue;
        }
        if (line[i] == '`') {
            continue;
        }
        if (line[i] == '[') {
            const size_t close = line.find("](", i);
            const size_t end = close == std::string_view::npos ? close : line.find(')', close + 2);
            if (end != std::string_view::npos) {
                const std::string_view text = line.substr(i + 1, close - i - 1);
                const std::string_view url = line.substr(close + 2, end - close - 2);
                out.append(text);
                if (startsWith(url, "http://") || startsWith(url, "https://")) {
                    out.append(" (").append(url).append(")");
                }
                i = end;
                continue;
            }
        }
        out.push_back(line[i]);
    }
    return out;
}

} // namespace

std::string noticesToPlainText(std::string_view markdown) {
    std::string result;
    result.reserve(markdown.size());
    bool inFence = false;
    size_t start = 0;
    while (start <= markdown.size()) {
        size_t end = markdown.find('\n', start);
        if (end == std::string_view::npos) {
            end = markdown.size();
        }
        std::string_view line = markdown.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        start = end + 1;

        if (startsWith(line, "```")) {
            inFence = !inFence;
            continue;
        }
        if (inFence) {
            result.append(line).push_back('\n');
            continue;
        }
        size_t level = 0;
        while (level < line.size() && line[level] == '#') {
            ++level;
        }
        if (level > 0 && level < line.size() && line[level] == ' ') {
            line.remove_prefix(level + 1);
        }
        result.append(formatInline(line)).push_back('\n');
    }
    // Drop trailing empty lines.
    while (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

} // namespace vp
