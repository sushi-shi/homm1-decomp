#include <PLATFORM/Platform.h>

#include <cctype>

namespace platform {

namespace {

bool IsBlank(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

bool IsSeparator(char c) {
#if defined(_WIN32)
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

void Trim(std::string& text) {
    while (!text.empty() && IsBlank(text.back()))
        text.pop_back();
    size_t first = 0;
    while (first < text.size() && IsBlank(text[first]))
        first++;
    text.erase(0, first);
}

}  // namespace

std::string ConfiguredDirectory(const std::string& value) {
    std::string text = value;
    Trim(text);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        text = text.substr(1, text.size() - 2);
        Trim(text);
    }
    // A root keeps its separator: "/" here, "C:\" on Windows.
    size_t keep = 1;
#if defined(_WIN32)
    if (text.size() >= 3 && text[1] == ':' && IsSeparator(text[2]))
        keep = 3;
#endif
    while (text.size() > keep && IsSeparator(text.back()))
        text.pop_back();
    return text;
}

std::string EnvironmentDirectory(const char* name) {
    return ConfiguredDirectory(Environment(name));
}

}  // namespace platform
