#include "dither/i18n.hpp"
#include <algorithm>
#include <cstring>
namespace dither::i18n {
struct Entry {
    const char *key;
    const char *value[4];
};
#include "catalog.inc"
Locale fromValue(int value) noexcept {
    return value >= 1 && value <= 4 ? static_cast<Locale>(value) : Locale::English;
}
Locale fromCode(std::string_view c) noexcept {
    std::string_view normalized = c;
    auto lower = [](char ch) { return ch >= 'A' && ch <= 'Z' ? char(ch + ('a' - 'A')) : ch; };
    auto startsWith = [&](std::string_view prefix) {
        if (normalized.size() < prefix.size())
            return false;
        for (std::size_t i = 0; i < prefix.size(); ++i)
            if (lower(normalized[i]) != prefix[i])
                return false;
        return true;
    };
    if (startsWith("zh")) {
        auto remainder = normalized.substr(2);
        bool traditional = false;
        for (std::string_view marker : {"hant", "tw", "hk", "mo"}) {
            for (std::size_t i = 0; i + marker.size() <= remainder.size(); ++i) {
                bool match = true;
                for (std::size_t j = 0; j < marker.size(); ++j)
                    if (lower(remainder[i + j]) != marker[j]) {
                        match = false;
                        break;
                    }
                traditional |= match;
            }
        }
        return traditional ? Locale::TraditionalChinese : Locale::SimplifiedChinese;
    }
    if (startsWith("ja"))
        return Locale::Japanese;
    return Locale::English;
}
const char *code(Locale l) noexcept {
    constexpr const char *codes[] = {"en", "zh-Hans", "zh-Hant", "ja"};
    return codes[static_cast<int>(fromValue(static_cast<int>(l))) - 1];
}
std::string_view fallback(std::string_view translated, std::string_view english,
                          std::string_view key) noexcept {
    return !translated.empty() ? translated : !english.empty() ? english : key;
}
std::size_t keyCount() noexcept { return sizeof(entries) / sizeof(entries[0]); }
std::string_view keyAt(std::size_t i) noexcept {
    return i < keyCount() ? entries[i].key : std::string_view{};
}
std::string_view text(Locale l, std::string_view key) noexcept {
    for (const auto &e : entries)
        if (key == e.key)
            return fallback(e.value[static_cast<int>(fromValue(static_cast<int>(l))) - 1], e.value[0], key);
    return key;
}
void copyUtf8(char *dst, std::size_t cap, std::string_view v) noexcept {
    if (!cap)
        return;
    auto n = std::min(cap - 1, v.size());
    if (n < v.size())
        while (n > 0 && (static_cast<unsigned char>(v[n]) & 0xC0) == 0x80)
            --n;
    if (n)
        std::memcpy(dst, v.data(), n);
    dst[n] = 0;
}
} // namespace dither::i18n
