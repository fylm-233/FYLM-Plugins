#include "dither/i18n.hpp"
#include "dither/core.hpp"
#include <cstring>
#include <future>
#include <iostream>
#include <stdexcept>
namespace loc = dither::i18n;
void check(bool b) {
    if (!b)
        throw std::runtime_error("i18n invariant failed");
}
bool utf8(std::string_view s) {
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = s[i++];
        int n = c < 128                ? 0
                : c >= 194 && c <= 223 ? 1
                : c >= 224 && c <= 239 ? 2
                : c >= 240 && c <= 244 ? 3
                                       : -1;
        if (n < 0 || i + n > s.size())
            return false;
        while (n--)
            if ((static_cast<unsigned char>(s[i++]) & 192) != 128)
                return false;
    }
    return true;
}
int main() {
    try {
        check(loc::fromCode("zh-CN") == loc::Locale::SimplifiedChinese);
        check(loc::fromCode("zh_CN") == loc::Locale::SimplifiedChinese);
        check(loc::fromCode("zh-HK") == loc::Locale::TraditionalChinese);
        check(loc::fromCode("zh_TW") == loc::Locale::TraditionalChinese);
        check(loc::fromCode("zh-Hant-TW") == loc::Locale::TraditionalChinese);
        check(loc::fromCode("ja_JP") == loc::Locale::Japanese);
        check(loc::fromCode("en_US") == loc::Locale::English);
        check(loc::fromCode("ja-JP") == loc::Locale::Japanese);
        check(loc::fromCode("fr") == loc::Locale::English && loc::fromValue(0) == loc::Locale::English);
        check(loc::text(loc::Locale::SimplifiedChinese, "param.algorithm") == "风格 / 算法");
        check(loc::text(loc::Locale::TraditionalChinese, "param.algorithm") == "風格 / 演算法");
        check(loc::fallback("", "English", "key") == "English");
        check(loc::fallback("", "", "key") == "key");
        check(loc::text(loc::Locale::Japanese, "missing.test") == "missing.test");
        for (std::size_t k = 0; k < loc::keyCount(); ++k)
            for (int l = 1; l <= 4; ++l) {
                auto key = loc::keyAt(k), value = loc::text(loc::fromValue(l), key);
                if(value.empty()||value==key||!utf8(value))throw std::runtime_error("translation: "+std::string(key)+" locale "+std::to_string(l));
                if (key.substr(0, 6) == "param." && key.find(".items")==std::string_view::npos)
                    if(value.size()>31)throw std::runtime_error("label too long: "+std::string(key)+" locale "+std::to_string(l));
            }
        char small[8];
        loc::copyUtf8(small, sizeof(small), "演算法演算法");
        check(std::string(small) == "演算" && utf8(small));
        char tiny[1] = {'x'};
        loc::copyUtf8(tiny, 1, "abc");
        check(tiny[0] == 0);
        loc::copyUtf8(nullptr, 0, "abc");
        std::future<void> jobs[4];
        for (int l = 1; l <= 4; ++l)
            jobs[l - 1] = std::async(std::launch::async, [l] {
                auto locale = loc::fromValue(l);
                auto expected = loc::text(locale, "error.internal");
                for (int n = 0; n < 10000; ++n)
                    check(loc::text(locale, "error.internal") == expected);
            });
        for (auto &job : jobs)
            job.get();
        auto r = dither::render({nullptr, 1, 1, 4, dither::PixelFormat::RGBA8},
                                {nullptr, 1, 1, 4, dither::PixelFormat::RGBA8}, {});
        check(!r.ok && r.code == dither::ErrorCode::InvalidImage);
        check(std::string_view(dither::errorKey(r.code)) == "error.invalid_image");
        std::cout << loc::keyCount()
                  << " keys x 4 locales, independent Chinese resources, fallback, UTF-8, concurrent lookup "
                     "and error codes passed.\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
