#ifndef CHENC_UCONV_HPP
#define CHENC_UCONV_HPP

#include <string>
#include <string_view>
#include <cstdint>

namespace chenc::uconv
{
    namespace utf8
    {
        // UTF-8 错误码定义
        enum class utf8_error : int64_t
        {
            ok_1_byte = 1,                         // 成功转换 1 字节
            ok_2_byte = 2,                         // 成功转换 2 字节
            ok_3_byte = 3,                         // 成功转换 3 字节
            ok_4_byte = 4,                         // 成功转换 4 字节
            ok_string = 5,                         // 输入字符串转换成功
            invalid_start_byte = 0,                // 非法的 UTF-8 起始字节
            null_utf8_ptr = -1,                    // UTF-8 输入指针为空
            null_utf32_ptr = -2,                   // UTF-32 输出指针为空
            zero_length = -3,                      // UTF-8 输入长度为 0
            too_short_for_2_bytes = -4,            // 2 字节序列长度不足
            invalid_2nd_byte_in_2_bytes = -5,      // 2 字节序列中第二字节非法
            overlong_2_bytes = -6,                 // 非法编码：应使用更短形式表示
            too_short_for_3_bytes = -7,            // 3 字节序列长度不足
            invalid_continuation_in_3_bytes = -8,  // 3 字节序列中后续字节非法
            overlong_3_bytes = -9,                 // 非法编码：应使用更短形式表示
            surrogate_in_3_bytes = -10,            // 非法编码：包含代理项 (U+D800~U+DFFF)
            non_character = -11,                   // 非法编码：包含非字符 (如 FDD0..FDEF 或 xxFFFE/xxFFFF)
            too_short_for_4_bytes = -12,           // 4 字节序列长度不足
            invalid_continuation_in_4_bytes = -13, // 4 字节序列中后续字节非法
            overlong_4_bytes = -14,                // 非法编码：应使用更短形式表示
            codepoint_out_of_range = -15           // 非法码点：超出 Unicode 最大范围
        };

        /**
         * @brief 将 UTF-8 单个字符转换为 UTF-32 字符
         *
         * @param utf8_char 输入的 UTF-8 字符串指针
         * @param utf8_len  可用的 UTF-8 字节长度
         * @param utf32_char 输出的 UTF-32 字符存储位置
         * @return utf8_error 错误码（或成功码）
         */
        inline utf8_error utf8_char_to_utf32(const char8_t *const utf8_char, const size_t utf8_len, char32_t *const utf32_char)
        {
            // 检查指针和长度有效性
            if (!utf8_char)
                return utf8_error::null_utf8_ptr;
            if (!utf32_char)
                return utf8_error::null_utf32_ptr;
            if (utf8_len == 0)
                return utf8_error::zero_length;

            const uint8_t *data = reinterpret_cast<const uint8_t *>(utf8_char);

            // 1 字节（ASCII 范围）
            if ((data[0] & 0b10000000) == 0b00000000)
            {
                *utf32_char = data[0];
                return utf8_error::ok_1_byte;
            }

            // 2 字节序列：U+0080 ~ U+07FF
            if ((data[0] & 0b11100000) == 0b11000000)
            {
                if (utf8_len < 2)
                    return utf8_error::too_short_for_2_bytes;
                if ((data[1] & 0b11000000) != 0b10000000)
                    return utf8_error::invalid_2nd_byte_in_2_bytes;

                *utf32_char = ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
                if (*utf32_char < 0x80)
                    return utf8_error::overlong_2_bytes;
                return utf8_error::ok_2_byte;
            }

            // 3 字节序列：U+0800 ~ U+FFFF
            if ((data[0] & 0b11110000) == 0b11100000)
            {
                if (utf8_len < 3)
                    return utf8_error::too_short_for_3_bytes;
                if ((data[1] & 0b11000000) != 0b10000000 || (data[2] & 0b11000000) != 0b10000000)
                    return utf8_error::invalid_continuation_in_3_bytes;

                *utf32_char = ((data[0] & 0x0F) << 12) |
                              ((data[1] & 0x3F) << 6) |
                              (data[2] & 0x3F);
                if (*utf32_char < 0x800)
                    return utf8_error::overlong_3_bytes;
                if (*utf32_char >= 0xD800 && *utf32_char <= 0xDFFF)
                    return utf8_error::surrogate_in_3_bytes;
                if ((*utf32_char >= 0xFDD0 && *utf32_char <= 0xFDEF) || (*utf32_char & 0xFFFE) == 0xFFFE)
                    return utf8_error::non_character;
                return utf8_error::ok_3_byte;
            }

            // 4 字节序列：U+10000 ~ U+10FFFF
            if ((data[0] & 0b11111000) == 0b11110000)
            {
                if (utf8_len < 4)
                    return utf8_error::too_short_for_4_bytes;
                if ((data[1] & 0b11000000) != 0b10000000 ||
                    (data[2] & 0b11000000) != 0b10000000 ||
                    (data[3] & 0b11000000) != 0b10000000)
                    return utf8_error::invalid_continuation_in_4_bytes;

                *utf32_char = ((data[0] & 0x07) << 18) |
                              ((data[1] & 0x3F) << 12) |
                              ((data[2] & 0x3F) << 6) |
                              (data[3] & 0x3F);
                if (*utf32_char < 0x10000)
                    return utf8_error::overlong_4_bytes;
                if (*utf32_char > 0x10FFFF)
                    return utf8_error::codepoint_out_of_range;
                if ((*utf32_char >= 0xFDD0 && *utf32_char <= 0xFDEF) || (*utf32_char & 0xFFFE) == 0xFFFE)
                    return utf8_error::non_character;
                return utf8_error::ok_4_byte;
            }

            // 无效起始字节
            return utf8_error::invalid_start_byte;
        }

        /**
         * @brief 将 UTF-8 字符串转换为 UTF-32 字符串
         * @param utf8_str 指向 UTF-8 字符串的指针
         * @param utf32_str 存储转换结果的 UTF-32 字符串
         * @return utf8_error 错误码（或成功码）
         */
        inline utf8_error utf8_str_to_utf32(const std::u8string_view &utf8_str, std::u32string &utf32_str)
        {
            utf32_str.clear();
            utf32_str.reserve(utf8_str.length()); // UTF-32 字符数 <= UTF-8 字节数
            const char8_t *current = utf8_str.data();
            const char8_t *const end = current + utf8_str.length();
            while (current < end)
            {
                char32_t c32;
                auto consumed = utf8_char_to_utf32(current, end - current, &c32);
                if (static_cast<int64_t>(consumed) <= 0)
                    return consumed;
                utf32_str.push_back(c32);
                current += static_cast<int64_t>(consumed);
            }
            return utf8_error::ok_string;
        }
    }

    namespace utf16
    {
        // UTF-16 错误码定义
        enum class utf16_error : int64_t
        {
            ok_1_word = 1,                   // 成功转换 1 个 UTF-16 单元
            ok_2_words = 2,                  // 成功转换代理对（2 个单元）
            ok_string = 3,                   // 输入字符串转换成功
            null_utf16_ptr = -1,             // UTF-16 输入指针为空
            null_utf32_ptr = -2,             // UTF-32 输出指针为空
            zero_length = -3,                // 输入长度为 0
            unexpected_low_surrogate = -4,   // 遇到未配对的低位代理项
            high_surrogate_without_low = -5, // 高位代理项缺失对应低位项
            invalid_bmp_code_point = -6,     // 非 BMP 区域的字符
            invalid_code_point = -7,         // 非法字符
        };

        /**
         * @brief 将 UTF-16 单个字符转换为 UTF-32 字符
         *
         * @param utf16_char 输入的 UTF-16 字符串指针
         * @param utf16_len  可用的 UTF-16 单元数量（不是字节）
         * @param utf32_char 输出的 UTF-32 字符存储位置
         * @return utf16_error 错误码（或成功码）
         */
        inline utf16_error utf16_char_to_utf32(const char16_t *utf16_char, size_t utf16_len, char32_t *utf32_char)
        {
            if (!utf16_char)
                return utf16_error::null_utf16_ptr;
            if (!utf32_char)
                return utf16_error::null_utf32_ptr;
            if (utf16_len == 0)
                return utf16_error::zero_length;

            char16_t first = utf16_char[0];

            // 非代理项，直接返回
            if (first < 0xD800 || first > 0xDFFF)
            {
                *utf32_char = first;

                // 对 BMP 码点进行非字符校验
                if ((*utf32_char >= 0xFDD0 and *utf32_char <= 0xFDEF) ||
                    (*utf32_char & 0xFFFE) == 0xFFFE)
                    return utf16_error::invalid_bmp_code_point;

                return utf16_error::ok_1_word;
            }

            // 遇到低位代理项，但没有前面的高位代理
            if (first >= 0xDC00 && first <= 0xDFFF)
                return utf16_error::unexpected_low_surrogate;

            // 高位代理项
            if (utf16_len < 2)
                return utf16_error::high_surrogate_without_low;

            char16_t second = utf16_char[1];
            if (second < 0xDC00 || second > 0xDFFF)
                return utf16_error::high_surrogate_without_low;

            *utf32_char = ((static_cast<char32_t>(first - 0xD800) << 10) |
                           (second - 0xDC00)) +
                          0x10000;

            // 对解码后的码点进行最终校验
            if ((*utf32_char & 0xFFFE) == 0xFFFE)
                return utf16_error::invalid_code_point;

            return utf16_error::ok_2_words;
        }

        /**
         * @brief 将 UTF-16 字符串转换为 UTF-32 字符串
         * @param utf16_str 指向 UTF-16 字符串的指针
         * @param utf32_str 存储转换结果的 UTF-32 字符串
         * @return utf16_error 错误码（或成功码）
         */
        inline utf16_error utf16_str_to_utf32(const std::u16string_view &utf16_str, std::u32string &utf32_str)
        {
            utf32_str.clear();
            utf32_str.reserve(utf16_str.length()); // UTF-32 字符数 <= UTF-16 单元数
            const char16_t *current = utf16_str.data();
            const char16_t *const end = current + utf16_str.length();
            while (current < end)
            {
                char32_t c32;
                auto consumed = utf16_char_to_utf32(current, end - current, &c32);
                if (static_cast<int64_t>(consumed) <= 0)
                    return consumed;
                utf32_str.push_back(c32);
                current += static_cast<int64_t>(consumed);
            }
            return utf16_error::ok_string;
        }

    }

    namespace utf32
    {
        // UTF-32 错误码定义
        enum class utf32_error : int64_t
        {
            ok_1 = 1,               // 成功转换为 1 utf8 字符 或 1 utf16 字符
            ok_2 = 2,               // 成功转换为 2 utf8 字符 或 2 utf16 字符
            ok_3 = 3,               // 成功转换为 3 utf8 字符
            ok_4 = 4,               // 成功转换为 4 utf8 字符
            ok_string = 5,          // 输入字符串转换成功
            null_utf32_ptr = -1,    // UTF-32 输入为空
            null_utf8_ptr = -2,     // UTF-8 输出为空
            null_utf16_ptr = -3,    // UTF-16 输出为空
            zero_length = -4,       // 可用缓冲区长度不足
            invalid_code_point = -5 // UTF-32 码点非法（超范围或代理区）
        };

        /**
         * @brief 将 UTF-32 单个字符转换为 UTF-8 字符
         *
         * @param utf32_char 输入的 UTF-32 字符串指针
         * @param utf8_char 输出的 UTF-8 字符存储位置
         * @param utf8_len 可用的 UTF-8 缓冲区字节数
         * @return utf32_error 错误码（或成功码）
         */
        inline utf32_error utf32_char_to_utf8(const char32_t *utf32_char, char8_t *utf8_char, size_t utf8_len)
        {
            if (!utf32_char)
                return utf32_error::null_utf32_ptr;
            if (!utf8_char)
                return utf32_error::null_utf8_ptr;

            char32_t ch = *utf32_char;

            // 对输入码点进行严格校验
            if (ch >= 0xD800 && ch <= 0xDFFF)
                return utf32_error::invalid_code_point; // 非法码点：代理对码点 (U+D800-U+DFFF) 不能直接编码
            if (ch > 0x10FFFF)
                return utf32_error::invalid_code_point; // 非法码点：超出 Unicode 最大范围 (U+10FFFF)
            if ((ch >= 0xFDD0 && ch <= 0xFDEF) or (ch & 0xFFFE) == 0xFFFE)
                return utf32_error::invalid_code_point; // 保留码点：Unicode 非字符不允许使用

            if (ch <= 0x7F)
            {
                if (utf8_len < 1)
                    return utf32_error::zero_length;
                utf8_char[0] = static_cast<char8_t>(ch);
                return utf32_error::ok_1;
            }
            else if (ch <= 0x7FF)
            {
                if (utf8_len < 2)
                    return utf32_error::zero_length;
                utf8_char[0] = static_cast<char8_t>(0xC0 | (ch >> 6));
                utf8_char[1] = static_cast<char8_t>(0x80 | (ch & 0x3F));
                return utf32_error::ok_2;
            }
            else if (ch <= 0xFFFF)
            {
                if (utf8_len < 3)
                    return utf32_error::zero_length;
                utf8_char[0] = static_cast<char8_t>(0xE0 | (ch >> 12));
                utf8_char[1] = static_cast<char8_t>(0x80 | ((ch >> 6) & 0x3F));
                utf8_char[2] = static_cast<char8_t>(0x80 | (ch & 0x3F));
                return utf32_error::ok_3;
            }
            else
            {
                if (utf8_len < 4)
                    return utf32_error::zero_length;
                utf8_char[0] = static_cast<char8_t>(0xF0 | (ch >> 18));
                utf8_char[1] = static_cast<char8_t>(0x80 | ((ch >> 12) & 0x3F));
                utf8_char[2] = static_cast<char8_t>(0x80 | ((ch >> 6) & 0x3F));
                utf8_char[3] = static_cast<char8_t>(0x80 | (ch & 0x3F));
                return utf32_error::ok_4;
            }
        }

        /**
         * @brief 将 UTF-32 单个字符转换为 UTF-16 字符
         *
         * @param utf32_char 输入的 UTF-32 字符串指针
         * @param utf16_char 输出的 UTF-16 字符存储位置
         * @param utf16_len 可用的 UTF-16 单元数量
         * @return utf32_error 错误码（或成功码）
         */
        inline utf32_error utf32_char_to_utf16(const char32_t *utf32_char, char16_t *utf16_char, size_t utf16_len)
        {
            if (!utf32_char)
                return utf32_error::null_utf32_ptr;
            if (!utf16_char)
                return utf32_error::null_utf16_ptr;

            char32_t ch = *utf32_char;

            // 对输入码点进行严格校验
            if (ch >= 0xD800 && ch <= 0xDFFF)
                return utf32_error::invalid_code_point; // 非法码点：代理对码点 (U+D800-U+DFFF) 不能直接编码
            if (ch > 0x10FFFF)
                return utf32_error::invalid_code_point; // 非法码点：超出 Unicode 最大范围 (U+10FFFF)
            if ((ch >= 0xFDD0 && ch <= 0xFDEF) or (ch & 0xFFFE) == 0xFFFE)
                return utf32_error::invalid_code_point; // 保留码点：Unicode 非字符不允许使用

            if (ch <= 0xFFFF)
            {
                if (utf16_len < 1)
                    return utf32_error::zero_length;
                utf16_char[0] = static_cast<char16_t>(ch);
                return utf32_error::ok_1;
            }
            else
            {
                if (utf16_len < 2)
                    return utf32_error::zero_length;
                ch -= 0x10000;
                utf16_char[0] = static_cast<char16_t>((ch >> 10) + 0xD800);
                utf16_char[1] = static_cast<char16_t>((ch & 0x3FF) + 0xDC00);
                return utf32_error::ok_2;
            }
        }

        /**
         * @brief 将 UTF-32 字符串转换为 UTF-8 字符串
         * @param utf32_str UTF-32 字符串
         * @param utf16_str UTF-8 输出字符串
         * @return 转换的字符数
         * @throws std::invalid_argument 当输入参数无效时抛出
         * @throws unicode_utf32_error 当码点非法或缓冲区不足
         */
        inline utf32_error utf32_str_to_utf8(const std::u32string_view &utf32_str, std::u8string &utf8_str)
        {
            utf8_str.clear();
            utf8_str.reserve(utf32_str.length() * 4); // 最坏情况：每个字符4字节
            char8_t buffer[4];
            for (char32_t c32 : utf32_str)
            {
                auto written = utf32_char_to_utf8(&c32, buffer, 4);
                if (static_cast<int64_t>(written) <= 0)
                    return written;
                utf8_str.append(buffer, static_cast<int64_t>(written));
            }
            return utf32_error::ok_string;
        }
        /**
         * @brief 将 UTF-32 字符串转换为 UTF-16 字符串
         * @param utf32_str UTF-32 字符串
         * @param utf16_str UTF-16 输出字符串
         * @return 转换的字符数
         * @throws std::invalid_argument 当输入参数无效时抛出
         * @throws unicode_utf32_error 当码点非法或缓冲区不足
         */
        inline utf32_error utf32_str_to_utf16(const std::u32string_view &utf32_str, std::u16string &utf16_str)
        {
            utf16_str.clear();
            utf16_str.reserve(utf32_str.length() * 2); // 最坏情况：每个字符2个单元
            char16_t buffer[2];
            for (char32_t c32 : utf32_str)
            {
                auto written = utf32_char_to_utf16(&c32, buffer, 2);
                if (static_cast<int64_t>(written) <= 0)
                    return written;
                utf16_str.append(buffer, static_cast<int64_t>(written));
            }
            return utf32_error::ok_string;
        }
    }

}

#endif