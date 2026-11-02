#pragma once
#include <cstddef>
#include <string_view>
namespace dither::i18n {
// Values are persisted in AE projects. Do not reorder.
enum class Locale { English = 1, SimplifiedChinese = 2, TraditionalChinese = 3, Japanese = 4 };
// Accepts AE/BCP-47 tags such as zh_CN, zh-Hant-TW, ja_JP, and en_US.
Locale fromCode(std::string_view code) noexcept;
Locale fromValue(int value) noexcept;
const char *code(Locale locale) noexcept;
std::string_view fallback(std::string_view translated, std::string_view english,
                          std::string_view key) noexcept;
std::string_view text(Locale locale, std::string_view key) noexcept;
std::size_t keyCount() noexcept;
std::string_view keyAt(std::size_t index) noexcept;
// Truncates only at a UTF-8 codepoint boundary. Always terminates when capacity > 0.
void copyUtf8(char *dst, std::size_t capacity, std::string_view value) noexcept;
} // namespace dither::i18n
