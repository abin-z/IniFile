/**************************************************************************************************************
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * @file: inifile.h
 * @version: v1.0.1
 * @description: Easy-to-use ini file parsing library that supports parsing, modifying and saving ini files.
 * - Features :
 *   - Lightweight & Easy-to-Use: A header-only INI parser with no external dependencies (C++11 only).
 *   - Read, Modify & Write: Easily handle INI configuration files.
 *   - Cross-platform: Supports Linux, Windows, MacOS and other systems.
 *   - Intuitive API: Simple and clear interface for reading, modifying, and writing INI files.
 *   - Multiple data source handling: support input/output from files `std::string` and `std::istream`.
 *   - Automatic Type Conversion: Seamlessly handles various data types.
 *   - Support Comment: Supports `[section]` and `key=value` line comments (`;` or `#`)
 *     (but end-of-line comments are not supported)
 *   - Custom type conversion: After customization, support automatic conversion for user-defined types
 *   - Support case insensitivity: Provides optional case insensitivity (for `section` and `key`)
 *   - Fully tested and memory-safe: Functionality has been verified with the Catch2 unit testing framework
 *     and memory management is leak-free with Valgrind.
 *
 * @author: abin
 * @date: 2025-02-23
 * @license: MIT
 * @repository: https://github.com/abin-z/inifile
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 **************************************************************************************************************/

#ifndef INI_FILE_H_
#define INI_FILE_H_

// clang-format off

// ========================================
// Feature detection
// ========================================
// ------ [[nodiscard]] ------
#if defined(__has_cpp_attribute)
#  if __has_cpp_attribute(nodiscard)
#    define INIFILE_NODISCARD [[nodiscard]]
#  endif
#endif

#ifndef INIFILE_NODISCARD
#  define INIFILE_NODISCARD
#endif

// ------ std::string_view ------
#if defined(__has_include)
#  if __has_include(<string_view>)
#    include <string_view>

#    if defined(__cpp_lib_string_view)
#      define INIFILE_HAS_STRING_VIEW 1
#    endif

#  endif
#endif

#ifndef INIFILE_HAS_STRING_VIEW
#  define INIFILE_HAS_STRING_VIEW 0
#endif


// ========================================
// User configuration
// ========================================
#ifndef INIFILE_TYPE_CONVERTER
#  define INIFILE_TYPE_CONVERTER ini::detail::convert
#endif

// clang-format on

// ========================================
// Standard headers
// ========================================
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ini
{

namespace detail
{
/** whitespace characters. */
static constexpr char whitespaces[] = " \t\n\r\f\v";

/// @brief 除去str两端空白字符
/// @param str
inline void trim(std::string &str)
{
  auto lastpos = str.find_last_not_of(whitespaces);
  if (lastpos == std::string::npos)
  {
    str.clear();
    return;
  }
  str.erase(lastpos + 1);
  str.erase(0, str.find_first_not_of(whitespaces));
}

/// @brief 判断字符串是否全是空白字符
/// @param str 输入的字符串
/// @return 如果字符串全是空白字符，则返回true，否则返回false
inline bool is_all_whitespace(const std::string &str)
{
  return str.find_first_not_of(whitespaces) == std::string::npos;
}

/// @brief 实现 C++11 中缺失的 std::make_unique
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args)
{
  static_assert(!std::is_array<T>::value, "detail::make_unique: array types (T[]) are not supported");
  static_assert(!std::is_reference<T>::value, "detail::make_unique: T must not be a reference");
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/// @brief 字符串切割功能
/// @param str 待处理字符串
/// @param delimiter 分割字符串(支持多字符)
/// @param skip_empty 是否忽略空字符串
/// @return 分割后的内容
inline std::vector<std::string> split(const std::string &str, const std::string &delimiter, bool skip_empty = false)
{
  std::vector<std::string> tokens;
  if (delimiter.empty())
  {
    if (!skip_empty || !str.empty()) tokens.emplace_back(str);
    return tokens;
  }
  std::string::size_type start = 0;
  std::string::size_type pos = 0;
  while ((pos = str.find(delimiter, start)) != std::string::npos)
  {
    if (!skip_empty || pos != start) tokens.emplace_back(str.substr(start, pos - start));
    start = pos + delimiter.length();
  }
  if (!skip_empty || start < str.size()) tokens.emplace_back(str.substr(start));
  return tokens;
}

// 1. 检查类型 T 是否支持 std::begin() 和 std::end()
template <typename T>
class has_begin_end {
 private:
  // 内部辅助模板, 尝试调用 std::begin() 和 std::end() 来检查类型 T 是否支持它们
  // 这个测试会先尝试通过 std::begin 和 std::end 获取迭代器
  // 如果这两个函数存在, 并且能正常编译, 最后会返回 std::true_type
  template <typename U>
  static auto test(int) -> decltype(std::begin(std::declval<U &>()),  // 检查是否支持 std::begin
                                    std::end(std::declval<U &>()),    // 检查是否支持 std::end
                                    std::true_type{});                // 如果能编译成功, 返回 std::true_type

  // 如果不支持 std::begin() 或 std::end(), 会匹配到这个重载, 返回 std::false_type
  template <typename>
  static std::false_type test(...);

 public:
  // 静态常量值, 使用 decltype 和 test<T>(0) 调用来决定类型 T 是否支持 begin() 和 end()
  static constexpr bool value = decltype(test<T>(0))::value;
};

// 2. 检查容器是否是 map 类型(如 std::map 或 std::unordered_map)
template <typename T>
class is_map {
 private:
  template <typename U>
  static auto test(int) ->
    typename std::is_same<typename U::value_type, std::pair<const typename U::key_type, typename U::mapped_type>>::type;

  template <typename>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(0))::value;
};

// 3. 检查元素类型是否支持 ostream 输出操作 <<
template <typename T>
class is_ostreamable {
 private:
  template <typename U>
  static auto test(int) -> decltype(std::declval<std::ostream &>() << std::declval<const U &>(), std::true_type{});

  template <typename>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(0))::value;
};

/// @brief 将容器中的元素连接为一个字符串,元素之间以指定分隔符分隔.空容器返回空字符串.
///        注意:容器的元素类型不能是指针类型.
/// @tparam Iterable 支持 std::begin() / std::end() 的序列式容器类型(如 vector、list、set 等)
/// @param iterable 要拼接的容器
/// @param separator 用于连接每个元素之间的分隔字符串
/// @return 拼接后的字符串结果
template <typename Iterable>
inline std::string join(const Iterable &iterable, const std::string &separator)
{
  // 断言 iterable 支持 begin() 和 end(), 不是 map 类型, 元素类型不是指针并且元素类型可通过 << 输出到 std::ostream
  using value_type = typename Iterable::value_type;
  static_assert(has_begin_end<Iterable>::value, "join() error: The type must support std::begin() and std::end()");
  static_assert(!std::is_pointer<value_type>::value, "join() error: Container elements cannot be of pointer type");
  static_assert(!is_map<Iterable>::value, "join() error: Map types (e.g. std::map) are not supported");
  static_assert(is_ostreamable<value_type>::value, "join() error: Elements must be streamable to std::ostream (<<)");

  std::ostringstream oss;
  auto it = std::begin(iterable);
  auto end = std::end(iterable);
  // 处理第一个元素
  if (it != end)
  {
    oss << *it++;  // 第一个元素,随后递增迭代器
  }
  while (it != end)
  {
    oss << separator << *it++;  // 添加分割符和后续元素,随后递增迭代器
  }
  return oss.str();
}

/// @brief 格式化注释字符串
/// @param comment 注释内容(值传递方式)
/// @param symbol 注释前缀符号
/// @return 格式化的注释字符串
inline std::string format_comment(std::string comment, char symbol)
{
  trim(comment);
  char comment_prefix = symbol == '#' ? '#' : ';';  // 只支持 ';' 和 '#' 注释, 默认使用 ';'
  if (comment.empty())
  {
    return {comment_prefix};
  }
  if (comment[0] != comment_prefix)
  {
    comment = comment_prefix + std::string(" ") + comment;
  }
  return comment;
}

/// @brief 去除单行字符串末尾的 Windows 换行符 '\r'
/// @param line 输入字符串
inline void remove_trailing_cr(std::string &line)
{
  if (!line.empty() && line.back() == '\r')
  {
    line.pop_back();
  }
}

/**
 * @brief 通用转换模板,未特化的 convert 结构体
 * 由于 SFINAE(替换失败不算错误)原则,未特化的 convert 不能实例化
 */
template <typename T, typename Enable = void>
struct convert;

/**
 * @brief convert<bool> 特化版本
 * 提供 `decode` 和 `encode` 方法,支持 `bool` 与 `std::string` 之间的转换
 */
template <>
struct convert<bool> {
  /**
   * @brief 将 std::string 转换为 bool 类型
   * @param value 输入的字符串
   * @param result 解析后的布尔值
   * @details
   *  - 允许大小写混合的 "false" 解析为 `false`
   *  - "0" 解析为 `false`
   *  - 空字符串 `""` 解析为 `false`
   *  - 其他情况一律解析为 `true`
   */
  static void decode(const std::string &value, bool &result)
  {
    std::string str(value);  // 复制字符串, 避免修改原始数据
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    // 除了 "false"、"0" 和空串,其他都应该为 true
    result = !(str == "false" || str == "0" || str.empty());
  }

  /**
   * @brief 将 bool 值转换为 std::string
   * @param value 布尔值
   * @param result 输出字符串:"true" 或 "false"
   */
  static void encode(const bool value, std::string &result)
  {
    result = value ? "true" : "false";
  }
};

template <>
struct convert<char> {
  static void decode(const std::string &value, char &result)
  {
    if (value.empty())
    {
      throw std::invalid_argument("[inifile] error: Cannot convert empty string to char: \"" + value + '"');
    }
    result = value[0];
  }
  static void encode(const char value, std::string &result)
  {
    result = std::string(1, value);
  }
};

template <>
struct convert<unsigned char> {
  static void decode(const std::string &value, unsigned char &result)
  {
    if (value.empty())
    {
      throw std::invalid_argument("[inifile] error: Cannot convert empty string to unsigned char: \"" + value + '"');
    }
    result = static_cast<unsigned char>(value[0]);
  }

  static void encode(const unsigned char value, std::string &result)
  {
    result = std::string(1, static_cast<char>(value));
  }
};

template <>
struct convert<signed char> {
  static void decode(const std::string &value, signed char &result)
  {
    if (value.empty())
    {
      throw std::invalid_argument("[inifile] error: Cannot convert empty string to signed char: \"" + value + '"');
    }
    result = static_cast<signed char>(value[0]);
  }

  static void encode(const signed char value, std::string &result)
  {
    result = std::string(1, value);
  }
};

// 处理 `std::string`
template <>
struct convert<std::string> {
  static void decode(const std::string &value, std::string &result)
  {
    result = value;
  }

  static void encode(const std::string &value, std::string &result)
  {
    result = value;
  }
};

// 处理 `const char*`
template <>
struct convert<const char *> {
  static void decode(const std::string &value, const char *&result)
  {
    result = value.c_str();
  }

  static void encode(const char *value, std::string &result)
  {
    result = value;
  }
};

// 处理 `char *`
template <>
struct convert<char *> {
  static void encode(char *value, std::string &result)
  {
    result = value;
  }
};

// 处理 `char[N]` 类型(即固定大小的字符数组)
template <std::size_t N>
struct convert<char[N]> {
  static void encode(const char (&value)[N], std::string &result)
  {
    result = value;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
// ~~~~~~~~~~~~~~~~~~~ is_to_stringable<T>::value 表示可以被std::to_string()处理的类型~~~~~~~~~~~~~~~~~~~
// 模拟 C++14 中的 std::void_t
template <typename...>
using void_t = void;

/// @brief is_to_stringable: 检查类型 T 是否支持 std::to_string()
template <typename T, typename = void>
struct is_to_stringable : std::false_type {};
// 如果 T 能传入 std::to_string, 则 is_to_stringable<T> 为 true
template <typename T>
struct is_to_stringable<T, void_t<decltype(std::to_string(std::declval<T>()))>> : std::true_type {};
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief convert 模板特化:处理所有整数类型(不包括字符类型 `char`、`signed char`、`unsigned char`、
 * `wchar_t`、`char8_t`、`char16_t`、`char32_t` 等)
 *
 * 该模板特化适用于所有满足 `std::is_integral<T>::value` 为 `true` 且可以被 `std::to_string()` 处理的类型.
 * 该特化确保只有整型类型(例如 `int`、`long` 等)能够匹配,而字符类型将被排除.
 */
template <typename T>
struct convert<T, typename std::enable_if<std::is_integral<T>::value && is_to_stringable<T>::value>::type> {
  /**
   * @brief 将字符串转换为整数
   * @param value 输入的字符串
   * @param result 转换后的整数
   *
   * - 处理空字符串,默认返回 `0`
   * - 使用 `std::strtoll()` / `std::strtoull()` 进行转换
   * - 检查 `errno == ERANGE`,防止溢出
   * - 确保转换值在 `T` 的范围内
   * - 检查 `end_ptr` 以确保完整转换
   */
  static void decode(const std::string &value, T &result)
  {
    if (value.empty())
    {
      throw std::invalid_argument("[inifile] error: Cannot convert empty string to integer: \"" + value + '"');
    }

    char *end_ptr = nullptr;
    errno = 0;  // 清除错误状态

    if (std::is_signed<T>::value)
    {
      long long temp = std::strtoll(value.c_str(), &end_ptr, 10);
      if (errno == ERANGE || temp < (std::numeric_limits<T>::min)() || temp > (std::numeric_limits<T>::max)())
      {
        throw std::out_of_range("[inifile] error: Integer conversion out of range: \"" + value + '"');
      }
      result = static_cast<T>(temp);
    }
    else
    {
      // 防止 -123 被 strtoull 转换成很大的数
      if (!value.empty() && value[0] == '-')
      {
        throw std::out_of_range("[inifile] error: Unsigned integer cannot be negative: \"" + value + '"');
      }

      unsigned long long temp = std::strtoull(value.c_str(), &end_ptr, 10);
      if (errno == ERANGE || temp > (std::numeric_limits<T>::max)())
      {
        throw std::out_of_range("[inifile] error: Unsigned integer conversion out of range: \"" + value + '"');
      }
      result = static_cast<T>(temp);
    }

    if (end_ptr == value.c_str() || *end_ptr != '\0')  // 检查是否转换完整
    {
      throw std::invalid_argument("[inifile] error: Invalid integer format: \"" + value + '"');
    }
  }

  /**
   * @brief 将整数转换为字符串
   * @param value 需要转换的整数
   * @param result 转换后的字符串
   *
   * - 直接调用 `std::to_string()` 进行转换
   */
  static void encode(const T value, std::string &result)
  {
    result = std::to_string(value);
  }
};

// 通用浮点字符串解析模板
template <typename T>
inline T parse_string_to_floating_point(const char *str, char **end_ptr)
{
  return static_cast<T>(std::strtold(str, end_ptr));
}
// 特化 float
template <>
inline float parse_string_to_floating_point<float>(const char *str, char **end_ptr)
{
  return std::strtof(str, end_ptr);
}
// 特化 double
template <>
inline double parse_string_to_floating_point<double>(const char *str, char **end_ptr)
{
  return std::strtod(str, end_ptr);
}
// 特化 long double
template <>
inline long double parse_string_to_floating_point<long double>(const char *str, char **end_ptr)
{
  return std::strtold(str, end_ptr);
}

/**
 * @brief convert 模板特化:处理浮点数类型 (`float`, `double`, `long double`)
 *
 * 该模板特化适用于所有 `std::is_floating_point<T>::value` 为 `true` 的类型,
 * 即 `float`、`double` 和 `long double`.
 */
template <typename T>
struct convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
  /**
   * @brief 将字符串转换为浮点数
   * @param value 输入的字符串
   * @param result 转换后的浮点数
   *
   * - 处理空字符串,默认返回 `0.0`
   * - 使用 `std::strtold()` 进行转换,以支持 `long double` 的精度
   * - 检查 `errno == ERANGE`,防止溢出
   * - 确保转换值在 `T` 的范围内
   * - 检查 `end_ptr` 以确保完整转换
   */
  static void decode(const std::string &value, T &result)
  {
    if (value.empty())
    {
      throw std::invalid_argument("[inifile] error: Cannot convert empty string to floating-point: \"" + value + '"');
    }

    // 检查长度为 3 或 4 的特殊值(inf或者nan)
    if (value.size() == 3 || value.size() == 4)
    {
      static const std::unordered_map<std::string, T> special_values = {
        {"inf", std::numeric_limits<T>::infinity()},   {"nan", std::numeric_limits<T>::quiet_NaN()},
        {"+inf", std::numeric_limits<T>::infinity()},  {"+nan", std::numeric_limits<T>::quiet_NaN()},
        {"-inf", -std::numeric_limits<T>::infinity()}, {"-nan", -std::numeric_limits<T>::quiet_NaN()}};
      auto it = special_values.find(value);
      if (it != special_values.end())
      {
        result = it->second;
        return;
      }
    }

    char *end_ptr = nullptr;
    errno = 0;
    T temp = parse_string_to_floating_point<T>(value.c_str(), &end_ptr);

    if (errno == ERANGE || temp < (std::numeric_limits<T>::lowest)() || temp > (std::numeric_limits<T>::max)())
    {
      throw std::out_of_range("[inifile] error: Floating-point conversion out of range: \"" + value + '"');
    }

    result = temp;

    if (end_ptr == value.c_str() || *end_ptr != '\0')  // 检查是否转换完整
    {
      throw std::invalid_argument("[inifile] error: Invalid floating-point format: \"" + value + '"');
    }
  }

  /**
   * @brief 将浮点数转换为字符串
   * @param value 需要转换的浮点数
   * @param result 转换后的字符串
   *
   * - 不使用 `std::to_string()` 进行转换, `std::to_string()` 会影响浮点数精度;
   * - 对于高精度需求可使用 `std::stringstream`;
   */
  static void encode(const T value, std::string &result)
  {
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
    result = oss.str();
  }
};

#if INIFILE_HAS_STRING_VIEW
template <>
struct convert<std::string_view> {
  static void decode(const std::string &value, std::string_view &result)
  {
    result = value;
  }

  static void encode(const std::string_view value, std::string &result)
  {
    result = value;
  }
};
#endif

/// @brief 大小写不敏感的哈希函数
struct case_insensitive_hash {
  std::size_t operator()(std::string s) const  // pass by value
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return std::hash<std::string>{}(s);
  }
};

/// @brief 大小写不敏感的比较函数
struct case_insensitive_equal {
  bool operator()(const std::string &lhs, const std::string &rhs) const
  {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                      [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
  }
};

}  // namespace detail

/**
 * @brief Multi-line comment block for INI sections or key-value pairs.
 *
 * Each line is stored with a leading `;` or `#` prefix. Whitespace-only lines are ignored.
 */
class comment {
  using comment_container = std::vector<std::string>;  // 注释容器

 public:
  using const_iterator = typename comment_container::const_iterator;
  using const_reverse_iterator = typename comment_container::const_reverse_iterator;

  /// @brief Constructs an empty comment.
  comment() = default;

  /// @brief Destroys the comment.
  ~comment() = default;

  /**
   * @brief Constructs a comment from a string (may contain multiple lines).
   * @param str Input text; lines are separated by `\n`.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  explicit comment(const std::string &str, char symbol = ';')
  {
    add(str, symbol);
  }

  /**
   * @brief Constructs a comment from a vector of lines.
   * @param vec Comment lines (each may already include a prefix).
   * @param symbol Comment prefix for lines that do not already have one;
   *               only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  explicit comment(const std::vector<std::string> &vec, char symbol = ';')
  {
    for (const auto &item : vec) add(item, symbol);
  }

  /**
   * @brief Constructs a comment from an initializer list of lines.
   * @param list Comment lines.
   * @param symbol Comment prefix for lines that do not already have one;
   *               only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    for (const auto &item : list) add(item, symbol);
  }

  /**
   * @brief Swaps contents with another comment.
   * @param other The other comment to swap with.
   */
  void swap(comment &other) noexcept
  {
    using std::swap;
    swap(comments_, other.comments_);
  }

  /**
   * @brief Swaps two comments (ADL-friendly non-member overload).
   * @param lhs Left-hand comment.
   * @param rhs Right-hand comment.
   */
  friend void swap(comment &lhs, comment &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  /**
   * @brief Copy-constructs a comment (deep copy).
   * @param other Source comment.
   */
  comment(const comment &other) :
    comments_(other.comments_ ? detail::make_unique<comment_container>(*other.comments_) : nullptr)
  {}

  /**
   * @brief Move-constructs a comment.
   * @param other Source comment; left empty after the move.
   */
  comment(comment &&other) noexcept : comments_(std::move(other.comments_))
  {
    other.comments_.reset();  // 显式清空, 跨平台行为一致
  }

  /**
   * @brief Copy-assigns from another comment (copy-and-swap).
   * @param rhs Source comment.
   * @return Reference to `*this`.
   */
  comment &operator=(const comment &rhs)
  {
    comment temp(rhs);  // copy ctor
    swap(temp);         // noexcept swap
    return *this;
  }

  /**
   * @brief Move-assigns from another comment.
   * @param rhs Source comment; left empty after the move.
   * @return Reference to `*this`.
   */
  comment &operator=(comment &&rhs) noexcept
  {
    comment temp(std::move(rhs));  // move ctor
    swap(temp);                    // noexcept swap
    return *this;
  }

  /**
   * @brief Checks whether the comment has no lines.
   * @return `true` if empty, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool empty() const noexcept
  {
    return !comments_ || comments_->empty();
  }

  /// @brief Removes all comment lines.
  void clear() noexcept
  {
    comments_.reset();
  }

  /**
   * @brief Returns a copy of all comment lines.
   * @return Vector of formatted comment lines (each starts with `;` or `#`).
   */
  INIFILE_NODISCARD
  std::vector<std::string> to_vector() const
  {
    return comments_ ? *comments_ : comment_container{};
  }

  /**
   * @brief Returns a const view of the internal comment lines.
   * @return Const reference to the line vector (empty static storage if unset).
   */
  INIFILE_NODISCARD
  const std::vector<std::string> &view() const
  {
    return comments_ ? *comments_ : empty_comments();  // 避免返回空引用
  }

  /**
   * @brief Appends comment text from a string (multi-line supported).
   * @param str Text to append; lines separated by `\n`. Whitespace-only input is ignored.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void add(const std::string &str, char symbol = ';')
  {
    if (detail::is_all_whitespace(str)) return;
    ensure_comments_initialized();
    add_comments_from_string(str, symbol);
  }

  /**
   * @brief Appends all lines from another comment (copy).
   * @param other Comment whose lines are appended.
   */
  void add(const comment &other)
  {
    if (other.empty()) return;
    ensure_comments_initialized();
    comments_->insert(comments_->end(), other.comments_->begin(), other.comments_->end());
  }

  /**
   * @brief Appends all lines from another comment (move).
   * @param other Comment whose lines are moved; cleared afterwards.
   */
  void add(comment &&other)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
  {
    if (other.empty()) return;
    ensure_comments_initialized();
    comments_->insert(comments_->end(), std::make_move_iterator(other.comments_->begin()),
                      std::make_move_iterator(other.comments_->end()));
    other.clear();  // 清空 other 的 comments_，防止重复使用
  }

  /**
   * @brief Appends comment lines from an initializer list.
   * @param list Comment lines to append.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void add(std::initializer_list<std::string> list, char symbol = ';')
  {
    for (const auto &item : list) add(item, symbol);
  }

  /**
   * @brief Replaces the comment with text from a string.
   * @param str New comment text; lines separated by `\n`. Whitespace-only clears the comment.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void set(const std::string &str, char symbol = ';')
  {
    if (!detail::is_all_whitespace(str))
    {
      ensure_comments_initialized();
      comments_->clear();
      add_comments_from_string(str, symbol);
    }
    else
    {
      comments_.reset();  // 不需要保留空注释
    }
  }

  /**
   * @brief Replaces the comment with a copy of another comment.
   * @param other Source comment.
   */
  void set(const comment &other)
  {
    comment temp(other);  // copy
    swap(temp);           // noexcept swap
  }

  /**
   * @brief Replaces the comment by moving from another comment.
   * @param other Source comment; left empty after the move.
   */
  void set(comment &&other) noexcept
  {
    comment temp(std::move(other));  // move
    swap(temp);                      // noexcept swap
  }

  /**
   * @brief Replaces the comment with an initializer list of lines.
   * @param list New comment lines.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void set(std::initializer_list<std::string> list, char symbol = ';')
  {
    set(comment(list, symbol));
  }

  // NOLINTBEGIN(modernize-use-nodiscard)

  /**
   * @brief Returns a const iterator to the first comment line.
   * @return Const iterator to the beginning.
   */
  const_iterator begin() const
  {
    return comments_ ? comments_->cbegin() : empty_comments().cbegin();
  }

  /**
   * @brief Returns a const iterator past the last comment line.
   * @return Const iterator to the end.
   */
  const_iterator end() const
  {
    return comments_ ? comments_->cend() : empty_comments().cend();
  }

  /**
   * @brief Returns a const iterator to the first comment line.
   * @return Const iterator to the beginning.
   */
  const_iterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns a const iterator past the last comment line.
   * @return Const iterator to the end.
   */
  const_iterator cend() const
  {
    return end();
  }

  /**
   * @brief Returns a const reverse iterator to the last comment line.
   * @return Const reverse iterator to the reverse beginning.
   */
  const_reverse_iterator rbegin() const
  {
    return comments_ ? comments_->crbegin() : empty_comments().crbegin();
  }

  /**
   * @brief Returns a const reverse iterator past the first comment line.
   * @return Const reverse iterator to the reverse end.
   */
  const_reverse_iterator rend() const
  {
    return comments_ ? comments_->crend() : empty_comments().crend();
  }

  /**
   * @brief Returns a const reverse iterator to the last comment line.
   * @return Const reverse iterator to the reverse beginning.
   */
  const_reverse_iterator crbegin() const
  {
    return rbegin();
  }

  /**
   * @brief Returns a const reverse iterator past the first comment line.
   * @return Const reverse iterator to the reverse end.
   */
  const_reverse_iterator crend() const
  {
    return rend();
  }
  // NOLINTEND(modernize-use-nodiscard)

  /**
   * @brief Equality comparison.
   * @param rhs Comment to compare with.
   * @return `true` if both comments have the same lines (or are both empty).
   */
  bool operator==(const comment &rhs) const
  {
    if (comments_ && rhs.comments_) return *comments_ == *rhs.comments_;
    return !comments_ && !rhs.comments_;
  }

  /**
   * @brief Inequality comparison.
   * @param rhs Comment to compare with.
   * @return `true` if the comments differ.
   */
  bool operator!=(const comment &rhs) const
  {
    return !(*this == rhs);
  }

 private:
  /// @brief 初始化 comments_, 确保不为nullptr
  void ensure_comments_initialized()
  {
    if (!comments_) comments_ = detail::make_unique<comment_container>();
  }

  static std::string format_comment_line(std::string comment, char symbol)
  {
    detail::trim(comment);
    char comment_prefix = symbol == '#' ? '#' : ';';  // 只支持 ';' 和 '#' 注释, 默认使用 ';'
    if (comment.empty())
    {
      return {comment_prefix};
    }
    if (comment[0] != comment_prefix)
    {
      comment.insert(0, 1, ' ');
      comment.insert(0, 1, comment_prefix);
    }
    return comment;
  }

  void add_comments_from_string(const std::string &str, char symbol)
  {
    std::istringstream stream(str);
    std::string line;
    while (std::getline(stream, line))
    {
      if (detail::is_all_whitespace(line)) continue;
      comments_->emplace_back(format_comment_line(std::move(line), symbol));
    }
  }

  /// @brief 提供一个空的注释容器, 用于避免空指针异常, 主要提供给迭代器使用
  /// @return 一个始终为空的注释容器
  static const comment_container &empty_comments()
  {
    static const comment_container empty;
    return empty;
  }

 private:
  std::unique_ptr<comment_container> comments_{nullptr};  // 行级注释容器, 使用unique_ptr主要考虑内存占用更小
};

/**
 * @brief Writes each comment line to an output stream, followed by a newline.
 * @param os Output stream.
 * @param c Comment to write.
 * @return Reference to @p os.
 */
inline std::ostream &operator<<(std::ostream &os, const comment &c)
{
  for (const auto &line : c.view())
  {
    os << line << '\n';
  }
  return os;
}

// 先声明模板类 basic_inifile, 声明友元的时候需要
// 声明完整的类型, 否则编译器会报错
template <typename, typename>
class basic_inifile;

/**
 * @brief A single INI field value (`key = value`), with optional associated comments.
 *
 * Values are stored as strings and can be converted to/from common C++ types via
 * `as()`, `set()`, assignment, and conversion operators.
 */
class field {
  friend std::ostream &operator<<(std::ostream &os, const field &data);

 public:
  /// @brief Constructs an empty field.
  field() = default;

  /**
   * @brief Constructs a field from a string value.
   * @param value Field value (passed by value for efficient copy/move).
   */
  explicit field(std::string value) : value_(std::move(value)) {}

  /// @brief Destroys the field.
  ~field() = default;

  /**
   * @brief Swaps contents with another field.
   * @param other The other field to swap with.
   */
  void swap(field &other) noexcept
  {
    using std::swap;
    swap(value_, other.value_);
    swap(comments_, other.comments_);
  }

  /**
   * @brief Swaps two fields (ADL-friendly non-member overload).
   * @param lhs Left-hand field.
   * @param rhs Right-hand field.
   */
  friend void swap(field &lhs, field &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  /**
   * @brief Move-constructs a field.
   * @param other Source field; left empty after the move.
   */
  field(field &&other) noexcept : value_(std::move(other.value_)), comments_(std::move(other.comments_))
  {
    other.value_.clear();     // 显式清空, 跨平台行为一致
    other.comments_.clear();  // 显式清空, 跨平台行为一致
  }

  /**
   * @brief Move-assigns from another field.
   * @param rhs Source field; left empty after the move.
   * @return Reference to `*this`.
   */
  field &operator=(field &&rhs) noexcept
  {
    field temp(std::move(rhs));  // move ctor
    swap(temp);                  // noexcept swap
    return *this;
  }

  /**
   * @brief Copy-constructs a field.
   * @param other Source field.
   */
  field(const field &other) : value_(other.value_), comments_(other.comments_) {}

  /**
   * @brief Copy-assigns from another field (copy-and-swap).
   * @param rhs Source field.
   * @return Reference to `*this`.
   */
  field &operator=(const field &rhs)  // `rhs` pass by reference
  {
    field temp(rhs);  // 使用拷贝构造函数创建一个临时对象, 这里会分配内存
    swap(temp);       // 利用拷贝构造+swap, 确保异常安全,也能处理自赋值问题
    return *this;
  }

  /**
   * @brief Constructs a field by converting a value of type @p T to string.
   * @tparam T Source type (must be supported by the type converter).
   * @param other Value to encode into the field.
   */
  template <typename T>
  field(const T &other)  // NOLINT(google-explicit-constructor)
  {
    detail::convert<T>::encode(other, value_);  // 将传入的值编码成字符串并存储到 value_ 中
  }

  /**
   * @brief Assigns a value of type @p T by converting it to string.
   * @tparam T Source type (must be supported by the type converter).
   * @param rhs Value to encode into the field.
   * @return Reference to `*this`.
   */
  template <typename T>
  field &operator=(const T &rhs)
  {
    detail::convert<T>::encode(rhs, value_);  // 将右侧值编码成字符串并存储到 value_ 中
    return *this;                             // 返回当前对象的引用,支持链式赋值
  }

  /**
   * @brief Converts the field value to type @p T.
   * @tparam T Target type.
   * @return Converted value.
   * @throws std::invalid_argument If the value cannot be converted to @p T
   *         (depends on @p T; e.g. bool/string conversions do not throw).
   * @throws std::out_of_range If the value is outside the valid range for @p T
   *         (integral/floating conversions).
   */
  template <typename T>
  T as() const
  {
    T result;                                    // 用于存储转换后的结果
    detail::convert<T>::decode(value_, result);  // 将 value_ 字符串解码为目标类型 T
    return result;                               // 返回转换结果
  }

  /**
   * @brief Converts the field value to type @p T and writes it to @p out.
   * @tparam T Target type.
   * @param out Output variable that receives the converted value.
   * @return Reference to @p out.
   * @throws std::invalid_argument If the value cannot be converted to @p T
   *         (depends on @p T; e.g. bool/string conversions do not throw).
   * @throws std::out_of_range If the value is outside the valid range for @p T
   *         (integral/floating conversions).
   */
  template <typename T>
  T &as_to(T &out) const
  {
    detail::convert<T>::decode(value_, out);  // 将 value_ 字符串解码为目标类型 T, 并存储到 out 中
    return out;                               // 返回转换后的引用
  }

  /**
   * @brief Implicit conversion to type @p T.
   * @tparam T Target type.
   * @return Converted value.
   * @throws std::invalid_argument If the value cannot be converted to @p T
   *         (depends on @p T; e.g. bool/string conversions do not throw).
   * @throws std::out_of_range If the value is outside the valid range for @p T
   *         (integral/floating conversions).
   */
  template <typename T>
  operator T() const  // NOLINT(google-explicit-constructor)
  {
    return this->as<T>();  // 使用 as<T> 方法将值转换为目标类型 T, 转换失败抛异常: std::invalid_argument
  }

  /**
   * @brief Sets the field value from a typed value.
   * @tparam T Type of @p value.
   * @param value Value to store (encoded as string).
   * @return Reference to `*this` (for chaining).
   */
  template <typename T>
  field &set(const T &value)
  {
    detail::convert<T>::encode(value, value_);  // 将值编码为字符串存储到 value_ 中
    return *this;
  }

  /**
   * @brief Replaces the key-value comment with text from a string.
   * @param str Comment text; multi-line allowed, lines separated by `\n`.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void set_comment(const std::string &str, char symbol = ';')
  {
    comments_.set(str, symbol);
  }

  /**
   * @brief Replaces the key-value comment with a copy of another comment.
   * @param other Source comment.
   */
  void set_comment(const comment &other)
  {
    comments_.set(other);
  }

  /**
   * @brief Replaces the key-value comment by moving from another comment.
   * @param other Source comment; left empty after the move.
   */
  void set_comment(comment &&other) noexcept
  {
    comments_.set(std::move(other));
  }

  /**
   * @brief Replaces the key-value comment with an initializer list of lines.
   * @param list Comment lines.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void set_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.set(list, symbol);
  }

  /**
   * @brief Appends text to the key-value comment.
   * @param str Comment text; multi-line allowed, lines separated by `\n`.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void add_comment(const std::string &str, char symbol = ';')
  {
    comments_.add(str, symbol);
  }

  /**
   * @brief Appends lines from another comment (copy).
   * @param other Comment whose lines are appended.
   */
  void add_comment(const comment &other)
  {
    comments_.add(other);
  }

  /**
   * @brief Appends lines from another comment (move).
   * @param other Comment whose lines are moved; left empty afterwards.
   */
  void add_comment(comment &&other) noexcept
  {
    comments_.add(std::move(other));
  }

  /**
   * @brief Appends comment lines from an initializer list.
   * @param list Comment lines to append.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void add_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.add(list, symbol);
  }

  /**
   * @brief Returns a const reference to the associated comment.
   * @return Const reference to the internal `comment` object.
   */
  INIFILE_NODISCARD
  const ini::comment &comment() const
  {
    return comments_;
  }

  /**
   * @brief Returns a mutable reference to the associated comment.
   * @return Reference to the internal `comment` object.
   */
  ini::comment &comment()
  {
    return comments_;
  }

  /// @brief Clears the key-value comment.
  void clear_comment()
  {
    comments_.clear();
  }

  /**
   * @brief Checks whether the field value string is empty.
   * @return `true` if the stored value is empty, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool empty() const noexcept
  {
    return value_.empty();
  }

 private:
  std::string value_;      // 存储字符串值,用于存储读取的 INI 文件字段值
  ini::comment comments_;  // key-value 键值对的注释
};

/**
 * @brief Writes the field's raw string value to an output stream.
 * @param os Output stream.
 * @param data Field to write.
 * @return Reference to @p os.
 */
inline std::ostream &operator<<(std::ostream &os, const field &data)
{
  return os << data.value_;
}

/**
 * @brief One INI section: a map of keys to @ref field values, plus an optional section comment.
 *
 * @tparam Hash Hash functor for key strings (default: `std::hash<std::string>`).
 * @tparam Equal Equality functor for key strings (default: `std::equal_to<std::string>`).
 *
 * Key-accepting APIs trim leading/trailing whitespace from the key argument
 * (`operator[]`, `set`, `contains`, `at`, `get`, `remove`, `find`, `erase(key)`, `count`).
 */
template <typename Hash = std::hash<std::string>, typename Equal = std::equal_to<std::string>>
class basic_section {
  using data_container = std::unordered_map<std::string, field, Hash, Equal>;  // 数据容器类型

 public:
  using key_type = typename data_container::key_type;
  using mapped_type = typename data_container::mapped_type;
  using value_type = typename data_container::value_type;
  using size_type = typename data_container::size_type;
  using difference_type = typename data_container::difference_type;

  using iterator = typename data_container::iterator;
  using const_iterator = typename data_container::const_iterator;

  /**
   * @brief Swaps contents with another section.
   * @param other The other section to swap with.
   */
  void swap(basic_section &other) noexcept
  {
    using std::swap;
    swap(data_, other.data_);
    swap(comments_, other.comments_);
  }

  /**
   * @brief Swaps two sections (ADL-friendly non-member overload).
   * @param lhs Left-hand section.
   * @param rhs Right-hand section.
   */
  friend void swap(basic_section &lhs, basic_section &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  /// @brief Constructs an empty section.
  basic_section() = default;

  /// @brief Destroys the section.
  ~basic_section() = default;

  /**
   * @brief Copy-constructs a section.
   * @param other Source section.
   */
  basic_section(const basic_section &other) : data_(other.data_), comments_(other.comments_) {}

  /**
   * @brief Copy-assigns from another section (copy-and-swap).
   * @param rhs Source section.
   * @return Reference to `*this`.
   */
  basic_section &operator=(const basic_section &rhs)
  {
    basic_section temp(rhs);  // copy ctor
    swap(temp);               // noexcept swap
    return *this;
  }

  /**
   * @brief Move-constructs a section.
   * @param other Source section; left empty after the move.
   */
  basic_section(basic_section &&other) noexcept : data_(std::move(other.data_)), comments_(std::move(other.comments_))
  {
    other.data_.clear();      // 显式清空, 跨平台行为一致
    other.comments_.clear();  // 显式清空, 跨平台行为一致
  }

  /**
   * @brief Move-assigns from another section.
   * @param rhs Source section; left empty after the move.
   * @return Reference to `*this`.
   */
  basic_section &operator=(basic_section &&rhs) noexcept
  {
    basic_section temp(std::move(rhs));  // move ctor
    swap(temp);                          // noexcept swap
    return *this;
  }

  /**
   * @brief Returns a reference to the field for @p key, inserting a default field if missing.
   * @param key Key name (whitespace trimmed).
   * @return Reference to the field associated with @p key.
   */
  field &operator[](std::string key)
  {
    detail::trim(key);
    return data_[std::move(key)];
  }

  /**
   * @brief Sets a single key-value pair.
   * @tparam T Type of @p value.
   * @param key Key name (whitespace trimmed).
   * @param value Value to store (converted to @ref field).
   * @return Reference to the inserted or updated field.
   */
  template <typename T>
  field &set(std::string key, T &&value)
  {
    detail::trim(key);
    return data_[std::move(key)] = std::forward<T>(value);
  }

  /**
   * @brief Sets multiple key-value pairs from an initializer list.
   * @param args Key-value pairs to insert or overwrite.
   */
  void set(std::initializer_list<std::pair<std::string, field>> args)
  {
    for (auto &&pair : args)
    {
      std::string key = pair.first;                    // 拷贝 key，准备去除空白
      detail::trim(key);                               // trim 去除前后空白，避免 key 带空格导致查找异常
      data_[std::move(key)] = std::move(pair.second);  // 插入键值对
    }
  }

  /**
   * @brief Checks whether a key exists in this section.
   * @param key Key name (whitespace trimmed).
   * @return `true` if the key exists, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool contains(std::string key) const
  {
    detail::trim(key);
    return data_.find(key) != data_.end();
  }

  /**
   * @brief Returns a reference to the field for @p key.
   * @param key Key name (whitespace trimmed).
   * @return Reference to the field.
   * @throws std::out_of_range If @p key does not exist.
   */
  field &at(std::string key)
  {
    detail::trim(key);
    return data_.at(key);
  }

  /**
   * @brief Returns a const reference to the field for @p key.
   * @param key Key name (whitespace trimmed).
   * @return Const reference to the field.
   * @throws std::out_of_range If @p key does not exist.
   */
  INIFILE_NODISCARD
  const field &at(std::string key) const
  {
    detail::trim(key);
    return data_.at(key);
  }

  /**
   * @brief Returns a copy of the field for @p key, or @p default_value if missing.
   * @param key Key name (whitespace trimmed).
   * @param default_value Value returned when @p key is not found.
   * @return Copy of the field, or @p default_value.
   */
  INIFILE_NODISCARD
  field get(std::string key, field default_value = field{}) const
  {
    detail::trim(key);
    if (data_.find(key) != data_.end())
    {
      return data_.at(key);
    }
    return default_value;
  }

  /**
   * @brief Returns all keys in this section.
   * @return Vector of key names (order is unspecified).
   */
  INIFILE_NODISCARD
  std::vector<key_type> keys() const
  {
    std::vector<key_type> result;
    result.reserve(data_.size());
    for (const auto &pair : data_)
    {
      result.emplace_back(pair.first);
    }
    return result;
  }

  /**
   * @brief Returns all field values in this section.
   * @return Vector of @ref field values (order is unspecified).
   */
  INIFILE_NODISCARD
  std::vector<mapped_type> values() const
  {
    std::vector<mapped_type> result;
    result.reserve(data_.size());
    for (const auto &pair : data_)
    {
      result.emplace_back(pair.second);
    }
    return result;
  }

  /**
   * @brief Returns all key-value pairs in this section.
   * @return Vector of `(key, field)` pairs (order is unspecified).
   */
  INIFILE_NODISCARD
  std::vector<value_type> items() const
  {
    return {data_.begin(), data_.end()};
  }

  /**
   * @brief Removes the key-value pair for @p key.
   * @param key Key name (whitespace trimmed).
   * @return `true` if a pair was removed, `false` if @p key was not found.
   */
  bool remove(std::string key)
  {
    detail::trim(key);
    return data_.erase(key) != 0;
  }

  /// @brief Removes all key-value pairs from this section (section comment is kept).
  void clear() noexcept
  {
    data_.clear();
  }

  /**
   * @brief Returns the number of key-value pairs.
   * @return Pair count.
   */
  INIFILE_NODISCARD
  size_type size() const noexcept
  {
    return data_.size();
  }

  /**
   * @brief Checks whether the section has no key-value pairs.
   * @return `true` if empty, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool empty() const noexcept
  {
    return data_.empty();
  }

  /**
   * @brief Finds a key and returns an iterator to the element.
   * @param key Key name (whitespace trimmed).
   * @return Iterator to the element, or @ref end() if not found.
   */
  iterator find(key_type key)
  {
    detail::trim(key);
    return data_.find(key);
  }

  /**
   * @brief Finds a key and returns a const iterator to the element.
   * @param key Key name (whitespace trimmed).
   * @return Const iterator to the element, or @ref end() if not found.
   */
  const_iterator find(key_type key) const
  {
    detail::trim(key);
    return data_.find(key);
  }

  /**
   * @brief Returns the number of elements with the given key (0 or 1).
   * @param key Key name (whitespace trimmed).
   * @return `1` if present, otherwise `0`.
   */
  size_type count(key_type key) const
  {
    detail::trim(key);
    return data_.count(key);
  }

  /**
   * @brief Erases the element at @p pos.
   * @param pos Iterator to the element to erase.
   * @return Iterator following the erased element.
   */
  iterator erase(iterator pos)
  {
    return data_.erase(pos);
  }

  /**
   * @brief Erases the element at @p pos.
   * @param pos Const iterator to the element to erase.
   * @return Iterator following the erased element.
   */
  iterator erase(const_iterator pos)
  {
    return data_.erase(pos);
  }

  /**
   * @brief Erases the elements in the half-open range `[first, last)`.
   * @param first Start of the range.
   * @param last End of the range.
   * @return Iterator following the last erased element.
   */
  iterator erase(const_iterator first, const_iterator last)
  {
    return data_.erase(first, last);
  }

  /**
   * @brief Erases the element with the given key.
   * @param key Key name (whitespace trimmed).
   * @return Number of elements erased (`0` or `1`).
   */
  size_type erase(key_type key)
  {
    detail::trim(key);
    return data_.erase(key);
  }

  /**
   * @brief Returns an iterator to the first key-value pair.
   * @return Iterator to the beginning.
   */
  iterator begin() noexcept
  {
    return data_.begin();
  }

  /**
   * @brief Returns a const iterator to the first key-value pair.
   * @return Const iterator to the beginning.
   */
  const_iterator begin() const noexcept
  {
    return data_.begin();
  }

  /**
   * @brief Returns an iterator past the last key-value pair.
   * @return Iterator to the end.
   */
  iterator end() noexcept
  {
    return data_.end();
  }

  /**
   * @brief Returns a const iterator past the last key-value pair.
   * @return Const iterator to the end.
   */
  const_iterator end() const noexcept
  {
    return data_.end();
  }

  /**
   * @brief Returns a const iterator to the first key-value pair.
   * @return Const iterator to the beginning.
   */
  const_iterator cbegin() const noexcept
  {
    return data_.cbegin();
  }

  /**
   * @brief Returns a const iterator past the last key-value pair.
   * @return Const iterator to the end.
   */
  const_iterator cend() const noexcept
  {
    return data_.cend();
  }

  /**
   * @brief Replaces the `[section]` comment with text from a string.
   * @param str Comment text; multi-line allowed, lines separated by `\n`.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void set_comment(const std::string &str, char symbol = ';')
  {
    comments_.set(str, symbol);
  }

  /**
   * @brief Replaces the `[section]` comment with a copy of another comment.
   * @param other Source comment.
   */
  void set_comment(const comment &other)
  {
    comments_.set(other);
  }

  /**
   * @brief Replaces the `[section]` comment by moving from another comment.
   * @param other Source comment; left empty after the move.
   */
  void set_comment(comment &&other) noexcept
  {
    comments_.set(std::move(other));
  }

  /**
   * @brief Replaces the `[section]` comment with an initializer list of lines.
   * @param list Comment lines.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void set_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.set(list, symbol);
  }

  /**
   * @brief Appends text to the `[section]` comment.
   * @param str Comment text; multi-line allowed, lines separated by `\n`.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void add_comment(const std::string &str, char symbol = ';')
  {
    comments_.add(str, symbol);
  }

  /**
   * @brief Appends lines from another comment (copy).
   * @param other Comment whose lines are appended.
   */
  void add_comment(const comment &other)
  {
    comments_.add(other);
  }

  /**
   * @brief Appends lines from another comment (move).
   * @param other Comment whose lines are moved; left empty afterwards.
   */
  void add_comment(comment &&other) noexcept
  {
    comments_.add(std::move(other));
  }

  /**
   * @brief Appends comment lines from an initializer list.
   * @param list Comment lines to append.
   * @param symbol Comment prefix; only `#` selects `#`, any other value uses `;`. Defaults to `;`.
   */
  void add_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.add(list, symbol);
  }

  /**
   * @brief Returns a const reference to the section-level comment.
   * @return Const reference to the internal `comment` object.
   */
  INIFILE_NODISCARD
  const ini::comment &comment() const
  {
    return comments_;
  }

  /**
   * @brief Returns a mutable reference to the section-level comment.
   * @return Reference to the internal `comment` object.
   */
  ini::comment &comment()
  {
    return comments_;
  }

  /// @brief Clears the `[section]` comment.
  void clear_comment()
  {
    comments_.clear();
  }

 private:
  data_container data_;    // key-value pairs
  ini::comment comments_;  // section-level comments
};

/**
 * @brief INI document: a map of section names to @ref basic_section, with load/save helpers.
 *
 * @tparam Hash Hash functor for section names (default: `std::hash<std::string>`).
 * @tparam Equal Equality functor for section names (default: `std::equal_to<std::string>`).
 *
 * Section-name and key-name arguments are trimmed of leading/trailing whitespace by
 * the corresponding lookup/insert APIs.
 * An empty section name represents keys that appear before any `[section]` header.
 */
template <typename Hash = std::hash<std::string>, typename Equal = std::equal_to<std::string>>
class basic_inifile {
  using section = basic_section<Hash, Equal>;  // 在 basic_inifile 内部定义 section 别名
  using data_container = std::unordered_map<std::string, section, Hash, Equal>;  // 数据容器类型

 public:
  using key_type = typename data_container::key_type;
  using mapped_type = typename data_container::mapped_type;
  using value_type = typename data_container::value_type;
  using size_type = typename data_container::size_type;
  using difference_type = typename data_container::difference_type;

  using iterator = typename data_container::iterator;
  using const_iterator = typename data_container::const_iterator;

  /**
   * @brief Swaps contents with another inifile.
   * @param other The other inifile to swap with.
   */
  void swap(basic_inifile &other) noexcept
  {
    using std::swap;
    swap(data_, other.data_);
  }

  /**
   * @brief Swaps two inifiles (ADL-friendly non-member overload).
   * @param lhs Left-hand inifile.
   * @param rhs Right-hand inifile.
   */
  friend void swap(basic_inifile &lhs, basic_inifile &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  /// @brief Constructs an empty inifile.
  basic_inifile() = default;

  /// @brief Destroys the inifile.
  ~basic_inifile() = default;

  /**
   * @brief Copy-constructs an inifile.
   * @param other Source inifile.
   */
  basic_inifile(const basic_inifile &other) = default;

  /**
   * @brief Copy-assigns from another inifile.
   * @param rhs Source inifile.
   * @return Reference to `*this`.
   */
  basic_inifile &operator=(const basic_inifile &rhs) = default;

  /**
   * @brief Move-constructs an inifile.
   * @param other Source inifile; left empty after the move.
   */
  basic_inifile(basic_inifile &&other) noexcept : data_(std::move(other.data_))
  {
    other.data_.clear();  // 显式清空, 跨平台行为一致
  };

  /**
   * @brief Move-assigns from another inifile.
   * @param rhs Source inifile; left empty after the move.
   * @return Reference to `*this`.
   */
  basic_inifile &operator=(basic_inifile &&rhs) noexcept
  {
    basic_inifile temp(std::move(rhs));  // move ctor
    swap(temp);                          // noexcept swap
    return *this;
  };

  /**
   * @brief Returns a reference to the section named @p sec, inserting an empty section if missing.
   * @param sec Section name (whitespace trimmed).
   * @return Reference to the section.
   */
  section &operator[](std::string sec)
  {
    detail::trim(sec);
    return data_[std::move(sec)];
  }

  /**
   * @brief Sets a value under the given section and key.
   * @tparam T Type of @p value.
   * @param sec Section name (whitespace trimmed).
   * @param key Key name (whitespace trimmed).
   * @param value Value to store.
   * @return Reference to the inserted or updated field.
   */
  template <typename T>
  field &set(std::string sec, std::string key, T &&value)
  {
    detail::trim(sec);
    detail::trim(key);
    return data_[std::move(sec)][std::move(key)] = std::forward<T>(value);
  }

  /**
   * @brief Checks whether a section exists.
   * @param sec Section name (whitespace trimmed).
   * @return `true` if the section exists, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool contains(std::string sec) const
  {
    detail::trim(sec);
    return data_.find(sec) != data_.end();
  }

  /**
   * @brief Checks whether a key exists in the given section.
   * @param sec Section name (whitespace trimmed).
   * @param key Key name (whitespace trimmed).
   * @return `true` if both the section and key exist, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool contains(std::string sec, std::string key) const
  {
    detail::trim(sec);
    auto sec_it = data_.find(sec);
    if (sec_it != data_.end())
    {
      return sec_it->second.contains(std::move(key));
    }
    return false;
  }

  /**
   * @brief Returns a reference to the section named @p sec.
   * @param sec Section name (whitespace trimmed).
   * @return Reference to the section.
   * @throws std::out_of_range If the section does not exist.
   */
  section &at(std::string sec)
  {
    detail::trim(sec);
    return data_.at(sec);
  }

  /**
   * @brief Returns a const reference to the section named @p sec.
   * @param sec Section name (whitespace trimmed).
   * @return Const reference to the section.
   * @throws std::out_of_range If the section does not exist.
   */
  const section &at(std::string sec) const
  {
    detail::trim(sec);
    return data_.at(sec);
  }

  /**
   * @brief Returns a copy of the field under section @p sec and key @p key,
   *        or @p default_value if missing.
   * @param sec Section name (whitespace trimmed).
   * @param key Key name (whitespace trimmed).
   * @param default_value Value returned when the section or key is not found.
   * @return Copy of the field, or @p default_value.
   */
  INIFILE_NODISCARD
  field get(std::string sec, std::string key, field default_value = field{}) const
  {
    detail::trim(sec);
    auto sec_it = data_.find(sec);
    if (sec_it != data_.end())
    {
      if (sec_it->second.contains(key))
      {
        return sec_it->second.at(std::move(key));
      }
    }
    return default_value;
  }

  /**
   * @brief Returns all section names.
   * @return Vector of section names (order is unspecified).
   */
  INIFILE_NODISCARD
  std::vector<key_type> sections() const
  {
    std::vector<key_type> result;
    result.reserve(data_.size());
    for (const auto &pair : data_)
    {
      result.push_back(pair.first);
    }
    return result;
  }

  /**
   * @brief Removes the section named @p sec (and all of its keys).
   * @param sec Section name (whitespace trimmed).
   * @return `true` if a section was removed, `false` if it was not found.
   */
  bool remove(std::string sec)
  {
    detail::trim(sec);
    return data_.erase(sec) != 0;
  }

  /// @brief Removes all sections and keys.
  void clear() noexcept
  {
    data_.clear();
  }

  /**
   * @brief Returns the number of sections.
   * @return Section count.
   */
  INIFILE_NODISCARD
  size_type size() const noexcept
  {
    return data_.size();
  }

  /**
   * @brief Checks whether the inifile has no sections.
   * @return `true` if empty, otherwise `false`.
   */
  INIFILE_NODISCARD
  bool empty() const noexcept
  {
    return data_.empty();
  }

  /**
   * @brief Finds a section and returns an iterator to it.
   * @param key Section name (whitespace trimmed).
   * @return Iterator to the section, or @ref end() if not found.
   */
  iterator find(key_type key)
  {
    detail::trim(key);
    return data_.find(key);
  }

  /**
   * @brief Finds a section and returns a const iterator to it.
   * @param key Section name (whitespace trimmed).
   * @return Const iterator to the section, or @ref end() if not found.
   */
  const_iterator find(key_type key) const
  {
    detail::trim(key);
    return data_.find(key);
  }

  /**
   * @brief Returns the number of sections with the given name (0 or 1).
   * @param key Section name (whitespace trimmed).
   * @return `1` if present, otherwise `0`.
   */
  size_type count(key_type key) const
  {
    detail::trim(key);
    return data_.count(key);
  }

  /**
   * @brief Erases the section at @p pos.
   * @param pos Iterator to the section to erase.
   * @return Iterator following the erased element.
   */
  iterator erase(iterator pos)
  {
    return data_.erase(pos);
  }

  /**
   * @brief Erases the section at @p pos.
   * @param pos Const iterator to the section to erase.
   * @return Iterator following the erased element.
   */
  iterator erase(const_iterator pos)
  {
    return data_.erase(pos);
  }

  /**
   * @brief Erases the sections in the half-open range `[first, last)`.
   * @param first Start of the range.
   * @param last End of the range.
   * @return Iterator following the last erased element.
   */
  iterator erase(const_iterator first, const_iterator last)
  {
    return data_.erase(first, last);
  }

  /**
   * @brief Erases the section with the given name.
   * @param key Section name (whitespace trimmed).
   * @return Number of elements erased (`0` or `1`).
   */
  size_type erase(key_type key)
  {
    detail::trim(key);
    return data_.erase(key);
  }

  /**
   * @brief Returns an iterator to the first section.
   * @return Iterator to the beginning.
   */
  iterator begin() noexcept
  {
    return data_.begin();
  }

  /**
   * @brief Returns a const iterator to the first section.
   * @return Const iterator to the beginning.
   */
  const_iterator begin() const noexcept
  {
    return data_.begin();
  }

  /**
   * @brief Returns an iterator past the last section.
   * @return Iterator to the end.
   */
  iterator end() noexcept
  {
    return data_.end();
  }

  /**
   * @brief Returns a const iterator past the last section.
   * @return Const iterator to the end.
   */
  const_iterator end() const noexcept
  {
    return data_.end();
  }

  /**
   * @brief Returns a const iterator to the first section.
   * @return Const iterator to the beginning.
   */
  const_iterator cbegin() const noexcept
  {
    return data_.cbegin();
  }

  /**
   * @brief Returns a const iterator past the last section.
   * @return Const iterator to the end.
   */
  const_iterator cend() const noexcept
  {
    return data_.cend();
  }

  /**
   * @brief Replaces the in-memory contents by parsing INI text from an input stream.
   *
   * Clears existing data first. Pending line comments (`;` / `#`) are attached to the
   * next `[section]` or `key=value` line; blank lines are skipped and do not clear the
   * pending comment. End-of-line comments are not supported.
   *
   * @param is Input stream providing INI text.
   */
  void read(std::istream &is)
  {
    data_.clear();
    std::string line;
    std::string current_section;
    comment comments;  // 注释类
    while (std::getline(is, line))
    {
      detail::trim(line);
      if (line.empty())  // 跳过空行
      {
        continue;
      }
      if (line[0] == ';' || line[0] == '#')  // 添加注释行
      {
        comments.add(line, line[0]);
        continue;
      }
      if (line.front() == '[' && line.back() == ']')  // 处理section
      {
        current_section = line.substr(1, line.size() - 2);
        detail::trim(current_section);
        if (!current_section.empty())
        {
          data_[current_section];  // 添加没有key=value的section
          if (!comments.empty())   // 添加注释
          {
            // After set_comment, comments.clear() should be called, but it is not necessary after using std::move
            data_[current_section].set_comment(std::move(comments));
          }
        }
      }
      else  // 处理key=value
      {
        auto pos = line.find('=');
        if (pos != std::string::npos)
        {
          std::string key = line.substr(0, pos);
          std::string value = line.substr(pos + 1);
          detail::trim(key);
          detail::trim(value);
          data_[current_section][key] = value;  // 允许section为空字符串
          if (!comments.empty())                // 添加注释
          {
            // set_comment后应该调用comments.clear()的, 但使用std::move后就不需要了
            data_[current_section][key].set_comment(std::move(comments));
          }
        }
      }
    }
  }

  /**
   * @brief Serializes the inifile to an output stream.
   *
   * Writes keys under the empty section name first (no `[section]` header), then
   * each named section. A blank line is inserted between sections.
   *
   * @param os Output stream to write to.
   */
  void write(std::ostream &os) const
  {
    bool first_section = true;

    // 先处理空 section(无 section 的键值对)
    auto it = data_.find("");
    if (it != data_.end())
    {
      for (const auto &kv : it->second)
      {
        write_comment(os, kv.second.comment());  // 添加kv注释
        os << kv.first << "=" << kv.second << "\n";
      }
      first_section = false;
    }

    // 处理非空 section
    for (const auto &sec : data_)
    {
      // 空 section 已经写过了
      if (sec.first.empty()) continue;

      if (!first_section) os << "\n";  // Section 之间插入空行
      first_section = false;
      write_comment(os, sec.second.comment());  // 添加section注释
      os << "[" << sec.first << "]\n";
      for (const auto &kv : sec.second)
      {
        write_comment(os, kv.second.comment());  // 添加kv注释
        os << kv.first << "=" << kv.second << "\n";
      }
    }
  }

  /**
   * @brief Replaces the in-memory contents by parsing an INI string.
   * @param str INI text.
   */
  void from_string(const std::string &str)
  {
    std::istringstream is(str);
    read(is);
  }

  /**
   * @brief Serializes the inifile to an INI-formatted string.
   * @return INI text.
   */
  INIFILE_NODISCARD
  std::string to_string() const
  {
    std::ostringstream ss;
    write(ss);
    return ss.str();
  }

  /**
   * @brief Loads and parses an INI file from disk.
   *
   * On success (or after the file is opened), existing in-memory data is replaced via @ref read.
   * If the file cannot be opened, this object is left unchanged and `false` is returned.
   *
   * @param filename Path to the file to read.
   * @return `true` on success, `false` if the file could not be opened or a hard I/O error occurred.
   */
  INIFILE_NODISCARD
  bool load(const std::string &filename)
  {
    std::ifstream is(filename);
    if (!is) return false;

    read(is);
    // 仅当 fail() 不是由于 EOF 造成的,并且没有发生 bad(),才认为读取成功
    return (!is.fail() || is.eof()) && !is.bad();
  }

  /**
   * @brief Writes the inifile to a file on disk.
   * @param filename Path to the file to write.
   * @return `true` on success, `false` if the file could not be opened or a write error occurred.
   */
  INIFILE_NODISCARD
  bool save(const std::string &filename) const
  {
    std::ofstream os(filename);
    if (!os) return false;

    write(os);
    os.flush();
    return !os.fail() && !os.bad();
  }

 private:
  /// @brief Writes comment lines to @p os (one line each).
  /// @param os Output stream.
  /// @param comments Comment block to write.
  static void write_comment(std::ostream &os, const comment &comments)
  {
    if (!comments.empty())
    {
      for (const auto &item : comments)
      {
        os << item << '\n';
      }
    }
  }

 private:
  data_container data_;  // section_name - key_value
};

/**
 * @brief Returns a copy of @p str with leading and trailing whitespace removed.
 * @param str Input string (passed by value).
 * @return Trimmed string.
 */
inline std::string trim(std::string str)
{
  detail::trim(str);
  return str;
}

/**
 * @brief Splits a string by a single-character delimiter.
 * @param str Input string.
 * @param delimiter Character used as the separator.
 * @param skip_empty If `true`, empty tokens are omitted; otherwise they are kept.
 * @return Vector of substrings.
 */
inline std::vector<std::string> split(const std::string &str, char delimiter, bool skip_empty = false)
{
  return detail::split(str, std::string(1, delimiter), skip_empty);
}

/**
 * @brief Splits a string by a string delimiter (may be multi-character).
 * @param str Input string.
 * @param delimiter Substring used as the separator.
 * @param skip_empty If `true`, empty tokens are omitted; otherwise they are kept.
 * @return Vector of substrings.
 */
inline std::vector<std::string> split(const std::string &str, const std::string &delimiter, bool skip_empty = false)
{
  return detail::split(str, delimiter, skip_empty);
}

/**
 * @brief Joins elements of a sequence container into a string, separated by a character.
 *
 * Requirements: @p Iterable must support `begin()`/`end()`, must not be a map-like type,
 * and elements must be streamable with `operator<<` (raw pointers are not allowed).
 *
 * @tparam Iterable Sequence container type (e.g. vector, list, set).
 * @param iterable Container whose elements are joined.
 * @param separator Character inserted between elements.
 * @return Joined string (empty if @p iterable is empty).
 */
template <typename Iterable>
inline std::string join(const Iterable &iterable, char separator)
{
  return detail::join(iterable, std::string(1, separator));
}

/**
 * @brief Joins elements of a sequence container into a string, separated by a string.
 *
 * Requirements: @p Iterable must support `begin()`/`end()`, must not be a map-like type,
 * and elements must be streamable with `operator<<` (raw pointers are not allowed).
 *
 * @tparam Iterable Sequence container type (e.g. vector, list, set).
 * @param iterable Container whose elements are joined.
 * @param separator String inserted between elements.
 * @return Joined string (empty if @p iterable is empty).
 */
template <typename Iterable>
inline std::string join(const Iterable &iterable, const std::string &separator)
{
  return detail::join(iterable, separator);
}

/// @brief Default case-sensitive section type (`basic_section<>`).
using section = basic_section<>;

/// @brief Default case-sensitive inifile type (`basic_inifile<>`).
using inifile = basic_inifile<>;

/// @brief Section type with case-insensitive key comparison.
using case_insensitive_section = basic_section<detail::case_insensitive_hash, detail::case_insensitive_equal>;

/// @brief Inifile type with case-insensitive section and key comparison.
using case_insensitive_inifile = basic_inifile<detail::case_insensitive_hash, detail::case_insensitive_equal>;

}  // namespace ini

#endif  // INI_FILE_H_