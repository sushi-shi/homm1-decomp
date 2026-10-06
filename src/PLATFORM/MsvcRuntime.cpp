#include <PLATFORM/MsvcRuntime.h>

#if !defined(_MSC_VER) && !defined(_WIN32)

#include <ctype.h>
#include <string.h>

// The Microsoft functions fold ASCII letters to lower case and compare the
// folded bytes as unsigned values.

int strnicmp(const char* left, const char* right, size_t count) {
    for (size_t i = 0; i < count; i++) {
        int a = tolower(static_cast<unsigned char>(left[i]));
        int b = tolower(static_cast<unsigned char>(right[i]));
        if (a != b)
            return a - b;
        if (a == 0)
            return 0;
    }
    return 0;
}

int stricmp(const char* left, const char* right) {
    return strnicmp(left, right, static_cast<size_t>(-1));
}

int strcmpi(const char* left, const char* right) {
    return stricmp(left, right);
}

char* strrev(char* text) {
    size_t length = strlen(text);
    for (size_t i = 0; i < length / 2; i++) {
        char swapped = text[i];
        text[i] = text[length - 1 - i];
        text[length - 1 - i] = swapped;
    }
    return text;
}

#endif
