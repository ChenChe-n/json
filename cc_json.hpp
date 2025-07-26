#ifndef CC_JSON_HPP
#define CC_JSON_HPP

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <variant>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <charconv>
#include <utility>
#include <memory>

/*
    是否启用 unordered_map
    还是默认使用 map
    理论上在小规模数据量下，map 速度更快，而 unordered_map 在大规模数据量下更快
    但是实际测试基本都是 map 更快
*/
#define CC_JSON_ENABLE_UNORDERED_MAP 0

inline double str_to_f64(std::string_view sv)
{
    double value;
    auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (result.ec != std::errc() or result.ptr != sv.data() + sv.size())
    {
        throw std::invalid_argument("str_to_f64: Invalid floating-point string");
    }
    return value;
}

inline std::string f64_to_str(double f64)
{
    std::string buffer;
    buffer.resize(32); // 足以容纳大多数 double
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), f64);
    if (result.ec == std::errc())
    {
        return buffer;
    }
    // 备用方案，或抛出异常
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10) << f64;
    return oss.str();
}

// 前向声明
class cc_json;

// 定义基本类型别名
using cc_json_key = std::string;
#if CC_JSON_ENABLE_UNORDERED_MAP
using cc_json_obj_map_t = std::unordered_map<cc_json_key, cc_json>;
#else
using cc_json_obj_map_t = std::map<cc_json_key, cc_json>;
#endif

class cc_json
{
public:
    struct nul;
    struct bol;
    struct num;
    struct str;

private:
    struct arr;
    struct obj;
    struct kvp;
    using node = cc_json;
    using key = cc_json_key;
    using obj_map_t = cc_json_obj_map_t;

public:
    struct nul
    {
        explicit nul() = default;
    };
    struct bol
    {
        inline explicit bol() : value_(false) {}
        inline explicit bol(bool value) : value_(value) {}
        template <typename T>
            requires std::integral<T>
        explicit bol(T value) : value_(static_cast<bool>(value))
        {
        }

        inline bool get_value() const { return value_; }

    private:
        bool value_;
    };
    struct num
    {
        inline explicit num() : value_(int64_t(0)) {}
        // 无符号整数构造函数
        template <typename T>
            requires std::unsigned_integral<T>
        explicit num(T value) : value_(static_cast<uint64_t>(value))
        {
        }
        // 有符号整数构造函数
        template <typename T>
            requires std::signed_integral<T>
        explicit num(T value) : value_(static_cast<int64_t>(value))
        {
        }
        // 浮点数构造函数
        template <typename T>
            requires std::floating_point<T>
        explicit num(T value) : value_(static_cast<double>(value))
        {
        }
        // 字符串构造函数
        inline explicit num(const char *value) : value_(value) {}
        inline explicit num(std::string &&value) : value_(std::move(value)) {}
        inline explicit num(const std::string_view &value) : value_(std::string(value)) {}

        inline uint64_t get_uint64_t() const
        {
            return std::visit([](auto &&arg) -> uint64_t
                              {
                                  using T = std::decay_t<decltype(arg)>;
                                  if constexpr (std::is_same_v<T, uint64_t>)
                                      return arg;
                                  else if constexpr (std::is_same_v<T, int64_t>)
                                      return static_cast<uint64_t>(arg);
                                  else if constexpr (std::is_same_v<T, double>)
                                      return static_cast<uint64_t>(arg);
                                  else if constexpr (std::is_same_v<T, std::string>)
                                      return static_cast<uint64_t>(std::stoull(arg));
                                  else
                                      throw std::invalid_argument("Invalid argument type"); },
                              value_);
        }
        inline int64_t get_int64_t() const
        {
            return std::visit([](auto &&arg) -> int64_t
                              {
                                  using T = std::decay_t<decltype(arg)>;
                                  if constexpr (std::is_same_v<T, uint64_t>)
                                      return static_cast<int64_t>(arg);
                                  else if constexpr (std::is_same_v<T, int64_t>)
                                      return arg;
                                  else if constexpr (std::is_same_v<T, double>)
                                      return static_cast<int64_t>(arg);
                                  else if constexpr (std::is_same_v<T, std::string>)
                                      return static_cast<int64_t>(std::stoll(arg));
                                  else
                                      throw std::invalid_argument("Invalid argument type"); },
                              value_);
        }
        inline double get_double() const
        {
            return std::visit([](auto &&arg) -> double
                              {
                                  using T = std::decay_t<decltype(arg)>;
                                  if constexpr (std::is_same_v<T, uint64_t>)
                                      return static_cast<double>(arg);
                                  else if constexpr (std::is_same_v<T, int64_t>)
                                      return static_cast<double>(arg);
                                  else if constexpr (std::is_same_v<T, double>)
                                      return arg;
                                  else if constexpr (std::is_same_v<T, std::string>)
                                      return std::stod(arg);
                                  else
                                      throw std::invalid_argument("Invalid argument type"); },
                              value_);
        }
        inline std::string get_string() const
        {
            return std::visit([](auto &&arg) -> std::string
                              {
                                  using T = std::decay_t<decltype(arg)>;
                                  if constexpr (std::is_same_v<T, uint64_t>)
                                      return std::to_string(arg);
                                  else if constexpr (std::is_same_v<T, int64_t>)
                                      return std::to_string(arg);
                                  else if constexpr (std::is_same_v<T, double>)
                                      return f64_to_str(arg);
                                  else if constexpr (std::is_same_v<T, std::string>)
                                      return arg;
                                  else
                                      throw std::invalid_argument("Invalid argument type"); },
                              value_);
        }

    private:
        std::variant<uint64_t, int64_t, double, std::string> value_;
    };
    struct str
    {
        inline explicit str() : value_({}) {}
        inline explicit str(const char *value) : value_(value) {}
        inline explicit str(std::string &&value) : value_(std::move(value)) {}
        inline explicit str(const std::string_view &value) : value_(value) {}

        inline const std::string &get_string_ref() const
        {
            return value_;
        }
        inline std::string &get_string_ref()
        {
            return value_;
        }

    private:
        std::string value_;
    };

private:
    struct arr
    {
        inline explicit arr() : value_({}) {}
        inline explicit arr(const std::initializer_list<node> &il) : value_(il) {}
        inline explicit arr(const std::vector<node> &value) : value_(value) {}
        inline explicit arr(std::vector<node> &&value) : value_(std::move(value)) {}

        inline const std::vector<node> &get_value_ref() const
        {
            return value_;
        }
        inline std::vector<node> &get_value_ref()
        {
            return value_;
        }

    private:
        std::vector<node> value_;
    };
    struct obj
    {
        inline explicit obj() : value_({}) {}
        inline explicit obj(std::initializer_list<std::pair<key, node>> il) : value_({})
        {
            for (const auto &pair : il)
            {
                value_.emplace(pair.first, pair.second);
            }
        }
        inline explicit obj(std::initializer_list<kvp> il) : value_({})
        {
            for (const auto &pair : il)
            {
                value_.emplace(pair.get_key(), pair.get_value());
            }
        }
        inline explicit obj(const obj_map_t &value) : value_(value) {}
        inline explicit obj(obj_map_t &&value) : value_(std::move(value)) {}

        inline const obj_map_t &get_value_ref() const
        {
            return value_;
        }
        inline obj_map_t &get_value_ref()
        {
            return value_;
        }

    private:
        obj_map_t value_;
    };
    struct kvp
    {
        inline explicit kvp(const key &k, const node &v) : key_(k), value_(std::make_shared<node>(v)) {}
        inline explicit kvp(const key &k, node &&v) : key_(k), value_(std::make_shared<node>(std::move(v))) {}

        inline const key &get_key() const { return key_; }
        inline const node &get_value() const { return *value_; }

    private:
        key key_;
        std::shared_ptr<node> value_;
    };

    using val = std::variant<nul, bol, num, str, arr, kvp, obj>;

private:
    val data_;

public:
    // 构造函数
    inline cc_json() : data_(nul{}) {}
    inline cc_json(const nul &value) : data_(value) {}
    inline cc_json(const bol &value) : data_(value) {}
    inline cc_json(const num &value) : data_(value) {}
    inline cc_json(const str &value) : data_(value) {}
    inline cc_json(const arr &value) : data_(value) {}
    inline cc_json(const kvp &value) : data_(value) {}
    inline cc_json(const obj &value) : data_(value) {}
    // kvp 构造函数
    inline cc_json(const key &k, const node &v) : data_(kvp(k, v)) {}
    inline cc_json(const key &k, node &&v) : data_(kvp(k, std::move(v))) {}
    inline cc_json(key &&k, const node &v) : data_(kvp(std::move(k), v)) {}
    inline cc_json(key &&k, node &&v) : data_(kvp(std::move(k), std::move(v))) {}
    inline cc_json(const key &k, const std::initializer_list<node> &v) : data_(kvp(k, v)) {}
    inline cc_json(key &&k, const std::initializer_list<node> &v) : data_(kvp(std::move(k), v)) {}
    inline cc_json(const char *k, const std::initializer_list<node> &v) : data_(kvp(k, v)) {}

    // 基础类型构造函数
    inline cc_json(std::nullptr_t) : data_(nul{}) {}
    inline cc_json(bool value) : data_(bol{value}) {}

    // 适用于所有数值类型 (int, float, double, ......)
    template <typename T>
        requires std::integral<T> or std::floating_point<T>
    cc_json(T value) : data_(num(value))
    {
    }

    inline cc_json(std::initializer_list<cc_json> il)
    {
        if (il.size() == 0)
        {
            data_ = arr{};
            return;
        }

        // 检查是否所有元素都是键值对形式（即数组大小为2，第一个元素是字符串）
        bool all_key_value_pairs = true;
        for (const auto &item : il)
        {
            // 检查是否是 kvp 类型或者是否是大小为2的数组且第一个元素是字符串
            if (!(item.is_kvp() ||
                  (item.is_array() &&
                   item.get_array().get_value_ref().size() == 2 &&
                   item.get_array().get_value_ref()[0].is_string())))
            {
                all_key_value_pairs = false;
                break;
            }
        }

        if (all_key_value_pairs and il.size() > 0)
        {
            // 创建对象
            obj o;
            for (const auto &item : il)
            {
                if (item.is_kvp())
                {
                    // 处理 kvp 类型
                    const auto &kvp_item = item.get_kvp();
                    o.get_value_ref()[kvp_item.get_key()] = kvp_item.get_value();
                }
                else
                {
                    // 处理数组形式的键值对 [key, value]
                    const auto &arr_item = item.get_array().get_value_ref();
                    std::string key = arr_item[0].get_string().get_string_ref();
                    cc_json value = arr_item[1];
                    o.get_value_ref()[key] = std::move(value);
                }
            }
            data_ = std::move(o);
        }
        else
        {
            // 创建数组
            arr a;
            for (const auto &item : il)
            {
                a.get_value_ref().emplace_back(item);
            }
            data_ = std::move(a);
        }
    }

    // 适用于字符串类型
    inline cc_json(const char *value) : data_(str{value}) {}
    inline cc_json(const std::string_view &value) : data_(str{value}) {}
    inline cc_json(std::string &&value) : data_(str{std::move(value)}) {}

    // 移动构造函数
    inline cc_json(bol &&value) : data_(std::move(value)) {}
    inline cc_json(num &&value) : data_(std::move(value)) {}
    inline cc_json(str &&value) : data_(std::move(value)) {}
    inline cc_json(arr &&value) : data_(std::move(value)) {}
    inline cc_json(kvp &&value) : data_(std::move(value)) {}
    inline cc_json(obj &&value) : data_(std::move(value)) {}

    // 拷贝与移动
    inline cc_json(const node &other) : data_(other.data_) {}
    inline cc_json(node &&other) noexcept : data_(std::move(other.data_)) {}

    inline cc_json &operator=(const node &other)
    {
        if (this != &other)
        {
            data_ = other.data_;
        }
        return *this;
    }
    inline cc_json &operator=(node &&other) noexcept
    {
        if (this != &other)
        {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // 类型检查
    inline bool is_null() const { return std::holds_alternative<nul>(data_); }
    inline bool is_bool() const { return std::holds_alternative<bol>(data_); }
    inline bool is_number() const { return std::holds_alternative<num>(data_); }
    inline bool is_string() const { return std::holds_alternative<str>(data_); }
    inline bool is_array() const { return std::holds_alternative<arr>(data_); }
    inline bool is_kvp() const { return std::holds_alternative<kvp>(data_); }
    inline bool is_object() const { return std::holds_alternative<obj>(data_); }
    inline std::string type_str() const
    {
        if (is_null())
            return "null";
        else if (is_bool())
            return "boolean";
        else if (is_number())
            return "number";
        else if (is_string())
            return "string";
        else if (is_array())
            return "array";
        else if (is_kvp())
            return "kvp";
        else if (is_object())
            return "object";
        else
            return "unknown"; // 不应该到达这里
    }

    // 获取值
    inline const bol &get_boolean() const
    {
        if (!is_bool())
            throw std::runtime_error("get_boolean: Not a boolean type");
        return std::get<bol>(data_);
    }
    inline bol &get_boolean()
    {
        if (!is_bool())
            throw std::runtime_error("get_boolean: Not a boolean type");
        return std::get<bol>(data_);
    }
    inline const num &get_number() const
    {
        if (!is_number())
            throw std::runtime_error("get_number: Not a number type");
        return std::get<num>(data_);
    }
    inline num &get_number()
    {
        if (!is_number())
            throw std::runtime_error("get_number: Not a number type");
        return std::get<num>(data_);
    }
    inline const str &get_string() const
    {
        if (!is_string())
            throw std::runtime_error("get_string: Not a string type");
        return std::get<str>(data_);
    }
    inline str &get_string()
    {
        if (!is_string())
            throw std::runtime_error("get_string: Not a string type");
        return std::get<str>(data_);
    }
    inline const arr &get_array() const
    {
        if (!is_array())
            throw std::runtime_error("get_array: Not an array type");
        return std::get<arr>(data_);
    }
    inline arr &get_array()
    {
        if (!is_array())
            throw std::runtime_error("get_array: Not an array type");
        return std::get<arr>(data_);
    }
    inline const kvp &get_kvp() const
    {
        if (!is_kvp())
            throw std::runtime_error("get_kvp: Not a kvp type");
        return std::get<kvp>(data_);
    }
    inline kvp &get_kvp()
    {
        if (!is_kvp())
            throw std::runtime_error("get_kvp: Not a kvp type");
        return std::get<kvp>(data_);
    }
    inline const obj &get_object() const
    {
        if (!is_object())
            throw std::runtime_error("get_object: Not an object type");
        return std::get<obj>(data_);
    }
    inline obj &get_object()
    {
        if (!is_object())
            throw std::runtime_error("get_object: Not an object type");
        return std::get<obj>(data_);
    }

    // 访问运算符
    inline const node &operator[](const key &key_) const
    {
        if (!is_object())
            throw std::runtime_error("operator[]: Not an object type");
        return std::get<obj>(data_).get_value_ref().at(key_); // 获取对象 / 没有则异常
    }
    inline node &operator[](const key &key_)
    {
        if (!is_object())
            throw std::runtime_error("operator[]: Not an object type");
        return std::get<obj>(data_).get_value_ref()[key_]; // 获取对象 / 没有则创建
    }
    inline const node &operator[](size_t index) const
    {
        if (!is_array())
            throw std::runtime_error("operator[]: Not an array type");
        return std::get<arr>(data_).get_value_ref().at(index); // 获取数组 / 超出则异常
    }
    inline node &operator[](size_t index)
    {
        if (!is_array())
            throw std::runtime_error("operator[]: Not an array type");
        return std::get<arr>(data_).get_value_ref().at(index); // 获取数组 / 超出则异常
    }

    // to_string 方法
    inline std::string to_string(size_t space = 2, bool enable_enter = true) const
    {
        std::string result;
        result.reserve(1024); // 预分配 1KB
        print_func(result, data_, 0, space, false, enable_enter);
        return result;
    }

    // 从 string 构造
    inline static cc_json parse(const std::string_view &str)
    {
        cc_json result;
        parse_value(result.data_, str, 0);
        return result;
    }

    inline static cc_json parse(const char *str)
    {
        return parse(std::string_view(str));
    }

private:
    inline static void print_func(std::string &result, const val &data, size_t level, size_t space, bool is_tab, bool enable_enter)
    {
        if (is_tab)
            result.append(level * space, ' ');

        std::visit([&](auto &&arg)
                   {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, nul>) {
                result.append("null");
            } else if constexpr (std::is_same_v<T, bol>) {
                result.append(arg.get_value() ? "true" : "false");
            } else if constexpr (std::is_same_v<T, num>) {
                result.append(arg.get_string());
            } else if constexpr (std::is_same_v<T, str>) {
                result.push_back('"');
                result.append(arg.get_string_ref());
                result.push_back('"');
            } else if constexpr (std::is_same_v<T, arr>) {
                const auto& arr = arg.get_value_ref();
                if (arr.empty()) {
                    result.append("[]");
                    return;
                }
                result.push_back('[');
                if (enable_enter) 
                    result.push_back('\n');
                for (size_t i = 0; i < arr.size(); ++i) {
                    print_func(result, arr[i].data_, level + 1, space, true,enable_enter);
                    if (i < arr.size() - 1) {
                        result.push_back(',');
                        if (enable_enter) 
                            result.push_back('\n');
                    } else {
                        if (enable_enter) 
                            result.push_back('\n');
                    }
                }
                if (space > 0) result.append(level * space, ' ');
                result.push_back(']');
            } else if constexpr (std::is_same_v<T, obj>) {
                const auto &obj = arg.get_value_ref();
                if (obj.empty()) {
                    result.append("{}");
                    return;
                }
                result.push_back('{');
                if (enable_enter) 
                    result.push_back('\n');
                auto it = obj.begin();
                while (it != obj.end()) {
                    if (space > 0) result.append((level + 1) * space, ' ');
                    result.push_back('"');
                    result.append(it->first);
                    result.append("\" : ");
                    print_func(result, it->second.data_, level + 1, space, false,enable_enter);
                    
                    if (++it != obj.end()) {
                        result.push_back(',');
                        if (enable_enter) 
                            result.push_back('\n');
                    } else {
                        if (enable_enter) 
                            result.push_back('\n');
                    }
                }
                if (space > 0) result.append(level * space, ' ');
                result.push_back('}');
            } }, data);
    }

private:
    inline static void skip_whitespace(const std::string_view &str, size_t &pos)
    {
        while (pos < str.length() and (str[pos] == ' ' or str[pos] == '\t' or str[pos] == '\n' or str[pos] == '\r'))
        {
            ++pos;
        }
    }
    inline static void expect_char(const std::string_view &str, size_t &pos, char expected)
    {
        if (pos >= str.length() or str[pos] != expected)
        {
            throw std::invalid_argument("Expected '" + std::string(1, expected) + "' but found '" +
                                        (pos < str.length() ? std::string(1, str[pos]) : "EOF") + "'");
        }
        ++pos;
    }
    inline static size_t parse_value(val &data, const std::string_view &str, size_t pos)
    {
        skip_whitespace(str, pos);
        if (pos >= str.length())
        {
            throw std::invalid_argument("parse_func: Empty input");
        }

        char ch = str[pos];
        if (ch == '{')
        {
            return parse_object(data, str, pos);
        }
        else if (ch == '[')
        {
            return parse_array(data, str, pos);
        }
        else if (ch == '"')
        {
            return parse_string(data, str, pos);
        }
        else if (ch == 't' or ch == 'f')
        {
            return parse_boolean(data, str, pos);
        }
        else if (ch == 'n')
        {
            return parse_null(data, str, pos);
        }
        else if (ch == '-' or (ch >= '0' and ch <= '9'))
        {
            return parse_number(data, str, pos);
        }
        else
        {
            throw std::invalid_argument("parse_func: Unexpected character: " + std::string(1, ch));
        }
    }
    inline static size_t parse_object(val &data, const std::string_view &str_, size_t pos)
    {
        size_t start_pos = pos;
        expect_char(str_, pos, '{');
        skip_whitespace(str_, pos);

        obj o;
        if (pos < str_.length() and str_[pos] == '}')
        {
            ++pos; // 空对象
            data = std::move(o);
            return pos - start_pos;
        }

        while (pos < str_.length())
        {
            skip_whitespace(str_, pos);

            // 解析键
            if (pos >= str_.length() or str_[pos] != '"')
            {
                throw std::invalid_argument("Expected string key in object");
            }

            val key_data;
            size_t key_len = parse_string(key_data, str_, pos);
            pos += key_len; // 更新位置
            std::string key = std::get<str>(key_data).get_string_ref();

            skip_whitespace(str_, pos);
            expect_char(str_, pos, ':');

            // 解析值
            skip_whitespace(str_, pos);
            cc_json value;
            size_t value_len = parse_value(value.data_, str_, pos);
            pos += value_len; // 更新位置

            o.get_value_ref()[key] = std::move(value);

            skip_whitespace(str_, pos);
            if (pos >= str_.length())
            {
                throw std::invalid_argument("Unexpected end of input in object");
            }

            if (str_[pos] == '}')
            {
                ++pos;
                break;
            }
            else if (str_[pos] == ',')
            {
                ++pos;
            }
            else
            {
                throw std::invalid_argument("Expected ',' or '}' in object, but found '" + std::string(1, str_[pos]) + "'");
            }
        }

        data = std::move(o);
        return pos - start_pos;
    }
    inline static size_t parse_array(val &data, const std::string_view &str, size_t pos)
    {
        size_t start_pos = pos;
        expect_char(str, pos, '[');
        skip_whitespace(str, pos);

        arr a;
        if (pos < str.length() and str[pos] == ']')
        {
            ++pos; // 空数组
            data = std::move(a);
            return pos - start_pos;
        }

        while (pos < str.length())
        {
            skip_whitespace(str, pos);

            // 解析元素
            cc_json element;
            size_t element_len = parse_value(element.data_, str, pos);
            pos += element_len;

            a.get_value_ref().emplace_back(std::move(element));

            skip_whitespace(str, pos);
            if (pos >= str.length())
            {
                throw std::invalid_argument("Unexpected end of input in array");
            }

            if (str[pos] == ']')
            {
                ++pos;
                break;
            }
            else if (str[pos] == ',')
            {
                ++pos;
            }
            else
            {
                throw std::invalid_argument("Expected ',' or ']' in array");
            }
        }

        data = std::move(a);
        return pos - start_pos;
    }
    inline static size_t parse_string(val &data, const std::string_view &str_, size_t pos)
    {
        size_t start_pos = pos;
        expect_char(str_, pos, '"');
        std::string result;
        size_t escape_character_count = 0;
        size_t escape_character_index = 0;

        while (pos < str_.length() and str_[pos] != '"')
        {
            if (str_[pos] == '\\')
            {
                if (escape_character_count != 0)
                {
                    result.append(str_.data() + escape_character_index, escape_character_count);
                    escape_character_count = 0;
                }
                ++pos;
                if (pos >= str_.length())
                {
                    throw std::invalid_argument("Unexpected end of input in escaped character");
                }

                switch (str_[pos])
                {
                case '"':
                    result.append("\\\"");
                    break;
                case '\\':
                    result.append("\\\\");
                    break;
                case '/':
                    result.append("\\/");
                    break;
                case 'b':
                    result.append("\\b");
                    break;
                case 'f':
                    result.append("\\f");
                    break;
                case 'n':
                    result.append("\\n");
                    break;
                case 'r':
                    result.append("\\r");
                    break;
                case 't':
                    result.append("\\t");
                    break;
                case 'u':
                    // 简单处理Unicode转义：保留原始转义序列
                    result.append("\\u");
                    // 检查是否有足够的字符
                    if (pos + 4 < str_.length())
                    {
                        result.append(str_.substr(pos + 1, 4));
                        pos += 4;
                    }
                    else
                    {
                        // 如果没有足够的字符，只添加可用的字符
                        result.append(str_.substr(pos + 1));
                        pos = str_.length() - 1;
                    }
                    break;
                default:
                    result.push_back(str_[pos]);
                    break;
                }
            }
            else
            {
                if (escape_character_count == 0)
                    escape_character_index = pos;
                ++escape_character_count;
            }
            ++pos;
        }
        if (escape_character_count != 0)
            result.append(str_.data() + escape_character_index, escape_character_count);

        expect_char(str_, pos, '"');
        data = str(std::move(result));
        return pos - start_pos;
    }
    inline static size_t parse_number(val &data, const std::string_view &str, size_t pos)
    {
        size_t start = pos;

        // 解析数字（包括负号、整数部分、小数部分、指数部分）
        if (pos < str.length() and str[pos] == '-')
        {
            ++pos;
        }

        // 整数部分
        if (pos < str.length() and str[pos] == '0')
        {
            ++pos;
        }
        else
        {
            while (pos < str.length() and str[pos] >= '0' and str[pos] <= '9')
            {
                ++pos;
            }
        }

        // 小数部分
        if (pos < str.length() and str[pos] == '.')
        {
            ++pos;
            while (pos < str.length() and str[pos] >= '0' and str[pos] <= '9')
            {
                ++pos;
            }
        }

        // 指数部分
        if (pos < str.length() and (str[pos] == 'e' or str[pos] == 'E'))
        {
            ++pos;
            if (pos < str.length() and (str[pos] == '+' or str[pos] == '-'))
            {
                ++pos;
            }
            while (pos < str.length() and str[pos] >= '0' and str[pos] <= '9')
            {
                ++pos;
            }
        }

        std::string num_str = std::string(str.substr(start, pos - start));

        try
        {
            // 判断是否为整数或浮点数
            if (num_str.find('.') != std::string::npos ||
                num_str.find('e') != std::string::npos ||
                num_str.find('E') != std::string::npos)
            {
                // 浮点数
                data = num(str_to_f64(num_str));
            }
            else
            {
                // 整数
                if (num_str[0] == '-' or (num_str.length() > 1 and num_str[0] == '+' and num_str[1] == '-'))
                {
                    data = num(std::stoll(num_str));
                }
                else
                {
                    data = num(std::stoull(num_str));
                }
            }
        }
        catch (...)
        {
            // 如果转换失败，当作字符串处理
            data = num(num_str);
        }
        return pos - start;
    }
    inline static size_t parse_boolean(val &data, const std::string_view &str, size_t pos)
    {
        if (str.substr(pos, 4) == "true")
        {
            pos += 4;
            data = bol(true);
            return 4;
        }
        else if (str.substr(pos, 5) == "false")
        {
            pos += 5;
            data = bol(false);
            return 5;
        }
        else
        {
            throw std::invalid_argument("Invalid boolean value");
        }
    }
    inline static size_t parse_null(val &data, const std::string_view &str, size_t pos)
    {
        if (str.substr(pos, 4) == "null")
        {
            pos += 4;
            data = nul{};
            return 4;
        }
        else
        {
            throw std::invalid_argument("Invalid null value");
        }
    }
};
#endif // CC_JSON_HPP