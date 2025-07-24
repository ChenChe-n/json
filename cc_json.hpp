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

class cc_json;
class cc_json_node;

inline double fast_str_to_f64(std::string_view sv)
{
    double value;
    auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (result.ec != std::errc() or result.ptr != sv.data() + sv.size())
    {
        throw std::invalid_argument("fast_str_to_f64: Invalid floating-point string");
    }
    return value;
}

inline std::string f64_to_str(double f64)
{
    std::array<char, 32> buffer; // 足以容纳大多数 double
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), f64);
    if (result.ec == std::errc())
    {
        return std::string(buffer.data(), result.ptr);
    }
    // 备用方案，或抛出异常
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10) << f64;
    return oss.str();
}

class cc_json_node
{
public:
    struct nul;
    struct bol;
    struct num;
    struct str;
    struct arr;
    struct obj;

    using node = cc_json_node;
    using key = std::string;
    using kvp = std::pair<key, node>;

    struct nul
    {
        std::nullptr_t value_ = nullptr;
    };
    struct bol
    {
        bool value_;
    };

    struct num
    {
        using value_type = std::variant<int64_t, double, std::string>;
        value_type value_;

        template <typename T>
        num(T value)
        {
            if constexpr (std::is_integral_v<T>)
            {
                value_ = static_cast<int64_t>(value);
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                value_ = static_cast<double>(value);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                value_ = value;
            }
            else if constexpr (std::is_same_v<T, const char *>)
            {
                value_ = std::string(value);
            }
            else
            {
                static_assert(std::is_same_v<T, int64_t> or std::is_same_v<T, double> or std::is_same_v<T, std::string>,
                              "Unsupported type for num");
            }
        }
        num(std::string &&value) : value_(std::move(value)) {}

        // 方便用户获取值的辅助函数
        double as_double() const
        {
            return std::visit([](auto &&arg) -> double
                              {
                                  using T = std::decay_t<decltype(arg)>;
                                  if constexpr (std::is_same_v<T, int64_t>)
                                      return static_cast<double>(arg);
                                  if constexpr (std::is_same_v<T, double>)
                                      return arg;
                                  if constexpr (std::is_same_v<T, std::string>)
                                      return std::stod(arg); // 或使用 fast_str_to_f64
                              },
                              value_);
        }

        int64_t as_int64() const
        {
            return std::visit([](auto &&arg) -> int64_t
                              {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, int64_t>) return arg;
                if constexpr (std::is_same_v<T, double>) return static_cast<int64_t>(arg);
                if constexpr (std::is_same_v<T, std::string>) return std::stoll(arg); }, value_);
        }

        std::string as_string() const
        {
            return std::visit([](auto &&arg) -> std::string
                              {
                                  using T = std::decay_t<decltype(arg)>;
                                  if constexpr (std::is_same_v<T, int64_t>)
                                      return std::to_string(arg);
                                  if constexpr (std::is_same_v<T, double>)
                                      return f64_to_str(arg);
                                  if constexpr (std::is_same_v<T, std::string>)
                                      return arg;
                                  return ""; // Should not happen
                              },
                              value_);
        }
    };

    struct str
    {
        std::string value_;
        str() = default;
        str(const std::string &value) : value_(value) {}
        str(std::string &&value) : value_(std::move(value)) {}
        str(const char *value) : value_(value) {}
    };

    struct arr
    {
        std::vector<node> value_;
        arr() = default;
        arr(const std::vector<node> &value) : value_(value) {}
        arr(std::vector<node> &&value) : value_(std::move(value)) {}
        arr(const std::initializer_list<node> &value) : value_(value) {}
    };

    struct obj
    {
        std::map<key, node> value_;
        obj() = default;
        obj(const std::map<key, node> &value) : value_(value) {}
        obj(std::map<key, node> &&value) : value_(std::move(value)) {}
        obj(const std::initializer_list<kvp> &value)
        {
            // value_.reserve(value.size());
            for (const auto &pair : value)
            {
                value_.emplace(pair.first, pair.second);
            }
        }
    };

    using val = std::variant<nul, bol, num, str, arr, obj>;

private:
    val data_;

public:
    // 构造函数
    cc_json_node() : data_(nul{}) {}
    cc_json_node(const nul &value) : data_(value) {}
    cc_json_node(const bol &value) : data_(value) {}
    cc_json_node(const num &value) : data_(value) {}
    cc_json_node(const str &value) : data_(value) {}
    cc_json_node(const arr &value) : data_(value) {}
    cc_json_node(const obj &value) : data_(value) {}

    // 移动构造函数
    cc_json_node(bol &&value) : data_(std::move(value)) {}
    cc_json_node(num &&value) : data_(std::move(value)) {}
    cc_json_node(str &&value) : data_(std::move(value)) {}
    cc_json_node(arr &&value) : data_(std::move(value)) {}
    cc_json_node(obj &&value) : data_(std::move(value)) {}

    // 拷贝与移动
    cc_json_node(const node &other) : data_(other.data_) {}
    cc_json_node(node &&other) noexcept : data_(std::move(other.data_)) {}

    cc_json_node &operator=(const node &other)
    {
        if (this != &other)
        {
            data_ = other.data_;
        }
        return *this;
    }
    cc_json_node &operator=(node &&other) noexcept
    {
        if (this != &other)
        {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // 各种类型的赋值操作符 (为方便起见保留)
    template <typename T,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<T>, cc_json_node>>>
    cc_json_node &operator=(T &&value)
    {
        // 这个模板现在只对非 cc_json_node 类型生效
        data_ = std::forward<T>(value);
        return *this;
    }

    // 类型判断
    bool is_null() const { return std::holds_alternative<nul>(data_); }
    bool is_boolean() const { return std::holds_alternative<bol>(data_); }
    bool is_number() const { return std::holds_alternative<num>(data_); }
    bool is_string() const { return std::holds_alternative<str>(data_); }
    bool is_array() const { return std::holds_alternative<arr>(data_); }
    bool is_object() const { return std::holds_alternative<obj>(data_); }

    // 获取数据
    const bol &get_boolean() const { return std::get<bol>(data_); }
    const num &get_number() const { return std::get<num>(data_); }
    const str &get_string() const { return std::get<str>(data_); }
    const arr &get_array() const { return std::get<arr>(data_); }
    const obj &get_object() const { return std::get<obj>(data_); }
    bol &get_boolean() { return std::get<bol>(data_); }
    num &get_number() { return std::get<num>(data_); }
    str &get_string() { return std::get<str>(data_); }
    arr &get_array() { return std::get<arr>(data_); }
    obj &get_object() { return std::get<obj>(data_); }

    // 访问操作符
    const node &operator[](const key &key) const
    {
        if (is_object())
        {
            const auto &obj_val = get_object().value_;
            auto it = obj_val.find(key);
            if (it != obj_val.end())
            {
                return it->second;
            }
            throw std::out_of_range("cc_json: key not found");
        }
        throw std::runtime_error("cc_json: not an object");
    }

    node &operator[](const key &key)
    {
        if (!is_object())
        {
            // 如果不是 object, 自动转换为空 object
            *this = obj{};
        }
        return get_object().value_[key];
    }

    const node &operator[](size_t index) const
    {
        if (is_array())
        {
            return get_array().value_.at(index);
        }
        throw std::runtime_error("cc_json: not an array");
    }

    node &operator[](size_t index)
    {
        if (!is_array())
        {
            // 如果不是 array, 自动转换为空 array
            *this = arr{};
        }
        auto &arr_val = get_array().value_;
        if (index >= arr_val.size())
        {
            // 自动扩展数组，并填充 null
            arr_val.resize(index + 1);
        }
        return arr_val[index];
    }

    friend class cc_json;
};

class cc_json
{
private:
    cc_json_node root_;

public:
    using key = cc_json_node::key;
    using val = cc_json_node::node;
    using kvp = cc_json_node::kvp;
    using nul = cc_json_node::nul;
    using bol = cc_json_node::bol;
    using num = cc_json_node::num;
    using str = cc_json_node::str;
    using arr = cc_json_node::arr;
    using obj = cc_json_node::obj;

    // 构造函数
    cc_json() : root_(nul{}) {};
    cc_json(const std::string_view &json_str)
    {
        size_t pos = 0;
        root_ = parse(json_str, pos);
    }
    cc_json(const arr &array) : root_(array) {}
    cc_json(arr &&array) : root_(std::move(array)) {}
    cc_json(const obj &object) : root_(object) {}
    cc_json(obj &&object) : root_(std::move(object)) {}

    // 访问操作符
    const cc_json_node &data() const { return root_; }

    std::string to_string(const size_t indent = 2) const
    {
        std::string result;
        // 预估大小，减少内存重分配
        result.reserve(1024);
        to_string_helper(result, root_, 0, indent, true);
        return result;
    }

    const val &operator[](const key &key) const { return root_[key]; }
    val &operator[](const key &key) { return root_[key]; }
    const val &operator[](size_t index) const { return root_[index]; }
    val &operator[](size_t index) { return root_[index]; }

private:
    // --- 解析函数 ---
    void skip_whitespace(const std::string_view &json_str, size_t &pos)
    {
        while (pos < json_str.length() and std::isspace(json_str[pos]))
        {
            pos++;
        }
    }

    void expect_char(const std::string_view &json_str, size_t &pos, char expected)
    {
        skip_whitespace(json_str, pos);
        if (pos >= json_str.length() or json_str[pos] != expected)
        {
            throw std::runtime_error(std::string("Expected '") + expected + "'");
        }
        pos++;
    }

    cc_json_node parse(const std::string_view &json_str, size_t &pos)
    {
        skip_whitespace(json_str, pos);
        auto result = parse_value(json_str, pos);
        skip_whitespace(json_str, pos);
        if (pos < json_str.length())
        {
            throw std::runtime_error("Unexpected characters at end of JSON");
        }
        return result;
    }

    cc_json_node::str parse_string(const std::string_view &json_str, size_t &pos)
    {
        expect_char(json_str, pos, '"');
        std::string result;
        result.reserve(32);
        while (pos < json_str.length() and json_str[pos] != '"')
        {
            if (json_str[pos] == '\\' and pos + 1 < json_str.length())
            {
                pos++;
                switch (json_str[pos])
                {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    result += json_str[pos];
                    break;
                }
            }
            else
            {
                result += json_str[pos];
            }
            pos++;
        }
        expect_char(json_str, pos, '"');
        return cc_json_node::str{std::move(result)};
    }

    cc_json_node::num parse_number(const std::string_view &json_str, size_t &pos)
    {
        size_t start = pos;

        // 简化的数字扫描逻辑 (与之前相同)
        if (pos < json_str.length() and json_str[pos] == '-')
            pos++;
        while (pos < json_str.length() and std::isdigit(json_str[pos]))
            pos++;
        bool is_float = false;
        if (pos < json_str.length() and json_str[pos] == '.')
        {
            is_float = true;
            pos++;
            while (pos < json_str.length() and std::isdigit(json_str[pos]))
                pos++;
        }
        if (pos < json_str.length() and (json_str[pos] == 'e' or json_str[pos] == 'E'))
        {
            is_float = true;
            pos++;
            if (pos < json_str.length() and (json_str[pos] == '+' or json_str[pos] == '-'))
                pos++;
            while (pos < json_str.length() and std::isdigit(json_str[pos]))
                pos++;
        }

        std::string_view num_sv(json_str.data() + start, pos - start);

        if (is_float)
        {
            return cc_json_node::num{fast_str_to_f64(num_sv)};
        }
        else
        {
            int64_t value;
            auto res = std::from_chars(num_sv.data(), num_sv.data() + num_sv.size(), value);
            if (res.ec == std::errc())
            {
                return cc_json_node::num{value};
            }
            else if (res.ec == std::errc::result_out_of_range)
            {
                return cc_json_node::num{std::string(num_sv)};
            }
            else
            {
                throw std::runtime_error("Invalid integer format");
            }
        }
        return cc_json_node::num(0.0);
    }

    cc_json_node::arr parse_array(const std::string_view &json_str, size_t &pos)
    {
        expect_char(json_str, pos, '[');
        skip_whitespace(json_str, pos);
        cc_json_node::arr result;
        result.value_.reserve(8);
        if (pos < json_str.length() and json_str[pos] == ']')
        {
            pos++;
            return result;
        }
        while (true)
        {
            result.value_.emplace_back(parse_value(json_str, pos));
            skip_whitespace(json_str, pos);
            if (pos < json_str.length() and json_str[pos] == ']')
            {
                pos++;
                break;
            }
            expect_char(json_str, pos, ',');
        }
        return result;
    }

    cc_json_node::obj parse_object(const std::string_view &json_str, size_t &pos)
    {
        expect_char(json_str, pos, '{');
        skip_whitespace(json_str, pos);
        cc_json_node::obj result;
        // result.value_.reserve(8);
        if (pos < json_str.length() and json_str[pos] == '}')
        {
            pos++;
            return result;
        }
        while (true)
        {
            auto key = parse_string(json_str, pos);
            expect_char(json_str, pos, ':');
            result.value_.emplace(std::move(key.value_), parse_value(json_str, pos));
            skip_whitespace(json_str, pos);
            if (pos < json_str.length() and json_str[pos] == '}')
            {
                pos++;
                break;
            }
            expect_char(json_str, pos, ',');
        }
        return result;
    }

    cc_json_node parse_value(const std::string_view &json_str, size_t &pos)
    {
        skip_whitespace(json_str, pos);
        if (pos >= json_str.length())
            throw std::runtime_error("Unexpected end of input");

        switch (json_str[pos])
        {
        case 'n':
            if (pos + 3 < json_str.length() and json_str.substr(pos, 4) == "null")
            {
                pos += 4;
                return cc_json_node{};
            }
            throw std::runtime_error("Invalid null value");
        case 't':
            if (pos + 3 < json_str.length() and json_str.substr(pos, 4) == "true")
            {
                pos += 4;
                return cc_json_node{bol{true}};
            }
            throw std::runtime_error("Invalid boolean value");
        case 'f':
            if (pos + 4 < json_str.length() and json_str.substr(pos, 5) == "false")
            {
                pos += 5;
                return cc_json_node{bol{false}};
            }
            throw std::runtime_error("Invalid boolean value");
        case '"':
            return parse_string(json_str, pos);
        case '[':
            return parse_array(json_str, pos);
        case '{':
            return parse_object(json_str, pos);
        case '-':
        case '0' ... '9':
            return parse_number(json_str, pos);
        default:
            throw std::runtime_error(std::string("Unexpected character: ") + json_str[pos]);
        }
    }

    // --- 序列化函数 ---
    void to_string_helper(std::string &result, const cc_json_node &node, size_t level, size_t indent, bool needs_indent) const
    {
        if (needs_indent and indent > 0)
        {
            result.append(level * indent, ' ');
        }

        std::visit([&](auto &&arg)
                   {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, nul>) {
                result.append("null");
            } else if constexpr (std::is_same_v<T, bol>) {
                result.append(arg.value_ ? "true" : "false");
            } else if constexpr (std::is_same_v<T, num>) {
                // 对 num 内部的 variant 进行二次 visit
                std::visit([&](auto&& num_val){
                    using NumT = std::decay_t<decltype(num_val)>;
                    if constexpr (std::is_same_v<NumT, int64_t>) {
                        result.append(std::to_string(num_val));
                    } else if constexpr (std::is_same_v<NumT, double>) {
                        result.append(f64_to_str(num_val));
                    } else if constexpr (std::is_same_v<NumT, std::string>) {
                        result.append(num_val);
                    }
                }, arg.value_);
            } else if constexpr (std::is_same_v<T, str>) {
                result.push_back('"');
                result.append(arg.value_); 
                result.push_back('"');
            } else if constexpr (std::is_same_v<T, arr>) {
                if (arg.value_.empty()) {
                    result.append("[]");
                    return;
                }
                result.append("[\n");
                for (size_t i = 0; i < arg.value_.size(); ++i) {
                    to_string_helper(result, arg.value_[i], level + 1, indent, true);
                    if (i < arg.value_.size() - 1) {
                        result.append(",\n");
                    } else {
                        result.push_back('\n');
                    }
                }
                if (indent > 0) result.append(level * indent, ' ');
                result.push_back(']');
            } else if constexpr (std::is_same_v<T, obj>) {
                if (arg.value_.empty()) {
                    result.append("{}");
                    return;
                }
                result.append("{\n");
                auto it = arg.value_.begin();
                while (it != arg.value_.end()) {
                    if (indent > 0) result.append((level + 1) * indent, ' ');
                    result.push_back('"');
                    result.append(it->first);
                    result.append("\" : ");
                    to_string_helper(result, it->second, level + 1, indent, false);
                    
                    if (++it != arg.value_.end()) {
                        result.append(",\n");
                    } else {
                        result.push_back('\n');
                    }
                }
                if (indent > 0) result.append(level * indent, ' ');
                result.push_back('}');
            } }, node.data_);
    }
};

#endif // CC_JSON_HPP