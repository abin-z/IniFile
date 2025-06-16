/**************************************************************************************************************
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * @file: inifile.h
 * @version: v0.9.9
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
 * @repository: https://github.com/abin-z/IniFile
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 **************************************************************************************************************/

#ifndef INI_FILE_H_
#define INI_FILE_H_

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

#ifdef __cpp_lib_string_view  // If we have std::string_view
#include <string_view>
#endif

// Provides custom type converters, users can customize type conversion
#ifndef INIFILE_TYPE_CONVERTER
#define INIFILE_TYPE_CONVERTER ini::detail::convert
#endif

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

/// @brief 字符串切割功能
/// @param str 待处理字符串
/// @param delimiter 分割字符串(支持多字符)
/// @param skip_empty 是否忽略空字符串
/// @return 分割后的内容
inline std::vector<std::string> split(const std::string &str, const std::string &delimiter, bool skip_empty = false)
{
  std::vector<std::string> tokens;
  std::string::size_type start = 0, pos = 0;
  while ((pos = str.find(delimiter, start)) != std::string::npos)
  {
    std::string token = str.substr(start, pos - start);
    if (!skip_empty || !token.empty())
    {
      tokens.emplace_back(std::move(token));
    }
    start = pos + delimiter.length();
  }
  // 处理最后一部分
  std::string last_token = str.substr(start);
  if (!skip_empty || !last_token.empty())
  {
    tokens.emplace_back(std::move(last_token));
  }
  return tokens;
}

// 1. 检查类型 T 是否支持 std::begin() 和 std::end()
template <typename T>
class has_begin_end
{
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
class is_map
{
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
class is_ostreamable
{
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
    return std::string(1, comment_prefix);
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
struct convert<bool>
{
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
struct convert<char>
{
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
struct convert<unsigned char>
{
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
    result = std::string(1, value);
  }
};

template <>
struct convert<signed char>
{
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
struct convert<std::string>
{
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
struct convert<const char *>
{
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
struct convert<char *>
{
  static void encode(char *value, std::string &result)
  {
    result = value;
  }
};

// 处理 `char[N]` 类型(即固定大小的字符数组)
template <std::size_t N>
struct convert<char[N]>
{
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
struct is_to_stringable : std::false_type
{
};
// 如果 T 能传入 std::to_string, 则 is_to_stringable<T> 为 true
template <typename T>
struct is_to_stringable<T, void_t<decltype(std::to_string(std::declval<T>()))>> : std::true_type
{
};
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief convert 模板特化:处理所有整数类型(不包括字符类型 `char`、`signed char`、`unsigned char`、
 * `wchar_t`、`char8_t`、`char16_t`、`char32_t` 等)
 *
 * 该模板特化适用于所有满足 `std::is_integral<T>::value` 为 `true` 且可以被 `std::to_string()` 处理的类型.
 * 该特化确保只有整型类型(例如 `int`、`long` 等)能够匹配,而字符类型将被排除.
 */
template <typename T>
struct convert<T, typename std::enable_if<std::is_integral<T>::value && is_to_stringable<T>::value>::type>
{
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
      if (errno == ERANGE || temp < std::numeric_limits<T>::min() || temp > std::numeric_limits<T>::max())
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
      if (errno == ERANGE || temp > std::numeric_limits<T>::max())
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
struct convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
{
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

    if (errno == ERANGE || temp < std::numeric_limits<T>::lowest() || temp > std::numeric_limits<T>::max())
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

#ifdef __cpp_lib_string_view
template <>
struct convert<std::string_view>
{
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
struct case_insensitive_hash
{
  std::size_t operator()(std::string s) const  // pass by value
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return std::hash<std::string>{}(s);
  }
};

/// @brief 大小写不敏感的比较函数
struct case_insensitive_equal
{
  bool operator()(const std::string &lhs, const std::string &rhs) const
  {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                      [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
  }
};

}  // namespace detail

/// @brief Represents a comment block for INI-style configuration, supporting multiple lines.
class comment
{
  using comment_container = std::vector<std::string>;  // 注释容器

 public:
  using const_iterator = typename comment_container::const_iterator;
  using const_reverse_iterator = typename comment_container::const_reverse_iterator;

  comment() = default;
  ~comment() = default;

  /// @brief Constructs a comment from a single string (can be multi-line).
  /// @param str Input string, lines separated by '\n'.
  /// @param symbol Comment symbol to use (';' or '#').
  explicit comment(const std::string &str, char symbol = ';')
  {
    add(str, symbol);
  }
  /// @brief Constructs a comment from a vector of lines.
  explicit comment(const std::vector<std::string> &vec, char symbol = ';')
  {
    for (const auto &item : vec) add(item, symbol);
  }
  /// @brief Constructs a comment from an initializer list of lines.
  comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    for (const auto &item : list) add(item, symbol);
  }
  /// @brief Swaps the internal comment data with another instance.
  void swap(comment &other) noexcept
  {
    using std::swap;
    swap(comments_, other.comments_);
  }
  friend void swap(comment &lhs, comment &rhs) noexcept
  {
    lhs.swap(rhs);
  }
  /// @brief Copy constructor.
  comment(const comment &other) :
    comments_(other.comments_ ? std::unique_ptr<comment_container>(new comment_container{*other.comments_}) : nullptr)
  {
  }
  /// @brief Move constructor.
  comment(comment &&other) noexcept : comments_(std::move(other.comments_))
  {
    other.comments_.reset();  // 显式清空, 跨平台行为一致
  }
  /// @brief Copy assignment.
  comment &operator=(const comment &rhs)
  {
    comment temp(rhs);  // copy ctor
    swap(temp);         // noexcept swap
    return *this;
  }
  /// @brief Move assignment.
  comment &operator=(comment &&rhs) noexcept
  {
    comment temp(std::move(rhs));  // move ctor
    swap(temp);                    // noexcept swap
    return *this;
  }
  /// @brief Checks if the comment is empty.
  bool empty() const noexcept
  {
    return !comments_ || comments_->empty();
  }
  /// @brief Clears the comment.
  void clear() noexcept
  {
    comments_.reset();
  }
  /// @brief Returns a copy of the internal comment lines.
  std::vector<std::string> to_vector() const
  {
    return comments_ ? *comments_ : comment_container{};
  }
  /// @brief Returns a const reference to the internal comment lines.
  const std::vector<std::string> &view() const
  {
    return comments_ ? *comments_ : empty_comments();  // 避免返回空引用
  }

  /// @brief Appends comment content from a string (multi-line supported).
  void add(const std::string &str, char symbol = ';')
  {
    if (detail::is_all_whitespace(str)) return;
    ensure_comments_initialized();
    add_comments_from_string(str, symbol);
  }
  /// @brief Appends comment lines from another comment.
  void add(const comment &other)
  {
    if (other.empty()) return;
    ensure_comments_initialized();
    comments_->insert(comments_->end(), other.comments_->begin(), other.comments_->end());
  }

  /// @brief Moves comment lines from another comment.
  void add(comment &&other) noexcept
  {
    if (other.empty()) return;
    ensure_comments_initialized();
    comments_->insert(comments_->end(), std::make_move_iterator(other.comments_->begin()),
                      std::make_move_iterator(other.comments_->end()));
    other.clear();  // 清空 other 的 comments_，防止重复使用
  }
  /// @brief Appends comment lines from an initializer list.
  void add(std::initializer_list<std::string> list, char symbol = ';')
  {
    for (const auto &item : list) add(item, symbol);
  }
  /// @brief Replaces current comment content with a string.
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
  /// @brief Replaces current comment content with another comment (copy).
  void set(const comment &other)
  {
    comment temp(other);  // copy
    swap(temp);           // noexcept swap
  }
  /// @brief Replaces current comment content with another comment (move).
  void set(comment &&other) noexcept
  {
    comment temp(std::move(other));  // move
    swap(temp);                      // noexcept swap
  }
  /// @brief Replaces current comment content with an initializer list.
  void set(std::initializer_list<std::string> list, char symbol = ';')
  {
    set(comment(list, symbol));
  }

  // Iterators for read-only access
  const_iterator begin() const
  {
    return comments_ ? comments_->cbegin() : empty_comments().cbegin();
  }
  const_iterator end() const
  {
    return comments_ ? comments_->cend() : empty_comments().cend();
  }
  const_iterator cbegin() const
  {
    return begin();
  }
  const_iterator cend() const
  {
    return end();
  }
  const_reverse_iterator rbegin() const
  {
    return comments_ ? comments_->crbegin() : empty_comments().crbegin();
  }
  const_reverse_iterator rend() const
  {
    return comments_ ? comments_->crend() : empty_comments().crend();
  }
  const_reverse_iterator crbegin() const
  {
    return rbegin();
  }
  const_reverse_iterator crend() const
  {
    return rend();
  }
  /// @brief Compares two comments for equality.
  bool operator==(const comment &rhs) const
  {
    if (comments_ && rhs.comments_) return *comments_ == *rhs.comments_;
    return !comments_ && !rhs.comments_;
  }
  /// @brief Compares two comments for inequality.
  bool operator!=(const comment &rhs) const
  {
    return !(*this == rhs);
  }

 private:
  /// @brief 初始化 comments_, 确保不为nullptr
  void ensure_comments_initialized()
  {
    if (!comments_) comments_.reset(new comment_container{});
  }

  static std::string format_comment_line(std::string comment, char symbol)
  {
    detail::trim(comment);
    char comment_prefix = symbol == '#' ? '#' : ';';  // 只支持 ';' 和 '#' 注释, 默认使用 ';'
    if (comment.empty())
    {
      return std::string(1, comment_prefix);
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

/// @brief ini field value
class field
{
  friend std::ostream &operator<<(std::ostream &os, const field &data);

 public:
  /// 默认构造函数,使用编译器生成的默认实现.
  field() = default;

  /// 参数构造函数：通过传入字符串初始化 value_
  /// 使用 pass-by-value 统一接收左值/右值，结合 std::move 实现高效构造
  field(std::string value) : value_(std::move(value)) {}

  /// 默认析构函数,使用编译器生成的默认实现.
  ~field() = default;

  /// @brief 成员swap函数
  void swap(field &other) noexcept
  {
    using std::swap;
    swap(value_, other.value_);
    swap(comments_, other.comments_);
  }

  // 友元 swap(非成员函数)(std::swap 支持)
  friend void swap(field &lhs, field &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  /// 移动构造函数
  field(field &&other) noexcept : value_(std::move(other.value_)), comments_(std::move(other.comments_))
  {
    other.value_.clear();     // 显式清空, 跨平台行为一致
    other.comments_.clear();  // 显式清空, 跨平台行为一致
  }

  /// 移动赋值运算符
  field &operator=(field &&rhs) noexcept
  {
    field temp(std::move(rhs));  // move ctor
    swap(temp);                  // noexcept swap
    return *this;
  }

  /// 重写拷贝构造函数,深拷贝 other 对象.
  field(const field &other) : value_(other.value_), comments_(other.comments_) {}

  /// 重写拷贝赋值(copy-and-swap 方式)
  field &operator=(const field &rhs) noexcept  // `rhs` pass by reference
  {
    field temp(rhs);  // 使用拷贝构造函数创建一个临时对象
    swap(temp);       // 利用拷贝构造+swap, 确保异常安全,也能处理自赋值问题
    return *this;
  }

  /// @brief Template constructor: allows construction of `field` objects from values ​​of other types.
  /// @tparam T Other type T
  /// @param other Other type value
  template <typename T>
  field(const T &other)
  {
    detail::convert<T>::encode(other, value_);  // 将传入的值编码成字符串并存储到 value_ 中
  }

  /// @brief Template copy assignment operator. Allows values ​​of other types to be assigned to `field` objects.
  /// @tparam T Other type T
  /// @param rhs Other type value
  /// @return `field` reference
  template <typename T>
  field &operator=(const T &rhs)
  {
    detail::convert<T>::encode(rhs, value_);  // 将右侧值编码成字符串并存储到 value_ 中
    return *this;                             // 返回当前对象的引用,支持链式赋值
  }

  /// @brief Converts an ini field to target type T. If the conversion fails, exception will be thrown.
  /// @tparam T Target type T
  /// @return Target type value
  /// @throws `std::invalid_argument` If the field cannot be converted to type T.
  /// @throws `std::out_of_range` If the field is out of the valid range for type T.
  template <typename T>
  T as() const
  {
    T result;                                    // 用于存储转换后的结果
    detail::convert<T>::decode(value_, result);  // 将 value_ 字符串解码为目标类型 T
    return result;                               // 返回转换结果
  }

  /// @brief Converts an ini field to target type T and stores the result in the given output variable.
  /// @tparam T Target type T
  /// @param out Output variable to receive the converted value
  /// @return Reference to the output variable after conversion
  /// @throws `std::invalid_argument` If the field cannot be converted to type T.
  /// @throws `std::out_of_range` If the field is out of the valid range for type T.
  template <typename T>
  T &as_to(T &out) const
  {
    detail::convert<T>::decode(value_, out);  // 将 value_ 字符串解码为目标类型 T, 并存储到 out 中
    return out;                               // 返回转换后的引用
  }

  /// @brief Type conversion operator: allows field objects to be converted to the target type T.
  //         If the conversion fails, exception will be thrown.
  /// @tparam T Target type to be converted
  /// @return The value of the target type after conversion
  /// @throws `std::invalid_argument` If the field cannot be converted to type T.
  /// @throws `std::out_of_range` If the field is out of the valid range for type T.
  template <typename T>
  operator T() const
  {
    return this->as<T>();  // 使用 as<T> 方法将值转换为目标类型 T, 转换失败抛异常: std::invalid_argument
  }

  /// @brief Sets the field value from a given typed value.
  /// @tparam T Type of the input value.
  /// @param value The value to be stored.
  /// @return Reference to the current field (for chaining).
  template <typename T>
  field &set(const T &value)
  {
    detail::convert<T>::encode(value, value_);  // 将值编码为字符串存储到 value_ 中
    return *this;
  }

  /// @brief Set `key=value` comment, overwriting the original comment.
  /// @param str Comment content. Multi-line input is allowed, lines separated by `\n`.
  /// @param symbol Comment symbol, default is `;`. Only `;` and `#` are supported.
  void set_comment(const std::string &str, char symbol = ';')
  {
    comments_.set(str, symbol);
  }
  /// @brief Overwrite the current comment with another comment (copy).
  void set_comment(const comment &other)
  {
    comments_.set(other);
  }
  /// @brief Overwrite the current comment with another comment (move).
  void set_comment(comment &&other) noexcept
  {
    comments_.set(std::move(other));
  }
  /// @brief Set the comment from an initializer list of strings.
  /// @param list List of comment lines.
  /// @param symbol Comment symbol, default is `;`. Only `;` and `#` are supported.
  void set_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.set(list, symbol);
  }

  /// @brief Add `key=value` comments by appending to the existing ones.
  /// @param str Comment content. Multi-line input is allowed, lines separated by `\n`.
  /// @param symbol Comment symbol, default is `;`. Only `;` and `#` are supported.
  void add_comment(const std::string &str, char symbol = ';')
  {
    comments_.add(str, symbol);
  }
  /// @brief Append comments from another comment object (copy).
  void add_comment(const comment &other)
  {
    comments_.add(other);
  }
  /// @brief Append comments from another comment object (move).
  void add_comment(comment &&other) noexcept
  {
    comments_.add(std::move(other));
  }
  /// @brief Append comments from an initializer list of strings.
  /// @param list List of comment lines.
  /// @param symbol Comment symbol, default is `;`. Only `;` and `#` are supported.
  void add_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.add(list, symbol);
  }

  /// @brief Get a const reference to the comment associated with this field.
  /// @return Const reference to the internal `comment` object.
  const ini::comment &comment() const
  {
    return comments_;
  }
  /// @brief Get a mutable reference to the comment associated with this field.
  /// @return Reference to the internal `comment` object.
  ini::comment &comment()
  {
    return comments_;
  }

  /// @brief Clear `key=value` comment
  void clear_comment()
  {
    comments_.clear();
  }

  bool empty() const noexcept
  {
    return value_.empty();
  }

 private:
  std::string value_;      // 存储字符串值,用于存储读取的 INI 文件字段值
  ini::comment comments_;  // key-value 键值对的注释
};

inline std::ostream &operator<<(std::ostream &os, const field &data)
{
  return os << data.value_;
}

/// @brief ini basic_section class
template <typename Hash = std::hash<std::string>, typename Equal = std::equal_to<std::string>>
class basic_section
{
  using data_container = std::unordered_map<std::string, field, Hash, Equal>;  // 数据容器类型

 public:
  using key_type = typename data_container::key_type;
  using mapped_type = typename data_container::mapped_type;
  using value_type = typename data_container::value_type;
  using size_type = typename data_container::size_type;
  using difference_type = typename data_container::difference_type;

  using iterator = typename data_container::iterator;
  using const_iterator = typename data_container::const_iterator;

  /// @brief 成员swap函数
  void swap(basic_section &other) noexcept
  {
    using std::swap;
    swap(data_, other.data_);
    swap(comments_, other.comments_);
  }

  // 友元 swap函数(非成员函数)
  friend void swap(basic_section &lhs, basic_section &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  // 默认构造
  basic_section() = default;
  // 默认析构函数
  ~basic_section() = default;
  /// 重写拷贝构造函数, 深拷贝
  basic_section(const basic_section &other) : data_(other.data_), comments_(other.comments_) {}
  /// 重写拷贝赋值函数(copy and swap方式)
  basic_section &operator=(const basic_section &rhs) noexcept
  {
    basic_section temp(rhs);  // copy ctor
    swap(temp);               // noexcept swap
    return *this;
  }
  // 移动构造函数
  basic_section(basic_section &&other) noexcept : data_(std::move(other.data_)), comments_(std::move(other.comments_))
  {
    other.data_.clear();      // 显式清空, 跨平台行为一致
    other.comments_.clear();  // 显式清空, 跨平台行为一致
  }
  // 移动赋值函数, 默认的不能处理移动自赋值情况
  basic_section &operator=(basic_section &&rhs) noexcept
  {
    basic_section temp(std::move(rhs));  // move ctor
    swap(temp);                          // noexcept swap
    return *this;
  }

  /// @brief Get or insert a field reference. If the key does not exist, insert a default constructed field object
  /// @param key key name
  /// @return Return the field reference corresponding to the key
  field &operator[](std::string key)
  {
    detail::trim(key);
    return data_[std::move(key)];
  }

  /// @brief Set key-value pairs
  /// @tparam T field value type
  /// @param key key
  /// @param value field value
  /// @return Reference to the inserted or updated field
  template <typename T>
  field &set(std::string key, T &&value)
  {
    detail::trim(key);
    return data_[std::move(key)] = std::forward<T>(value);
  }
  /// @brief Set multiple key-value pairs
  /// @param args initializer_list of multiple key-value pairs
  void set(std::initializer_list<std::pair<std::string, field>> args)
  {
    for (auto &&pair : args)
    {
      std::string key = pair.first;  // 拷贝 key，准备去除空白
      detail::trim(key);             // trim 去除前后空白，避免 key 带空格导致查找异常
      data_[std::move(key)] = std::move(pair.second);  // 插入键值对
    }
  }

  /// @brief key exists
  /// @param key
  /// @return returns true if exists
  bool contains(std::string key) const
  {
    detail::trim(key);
    return data_.find(key) != data_.end();
  }

  /// @brief Returns a reference to the field value of the specified key.
  ///        If the key does not exist, an `std::out_of_range` exception will be thrown.
  /// @param key key - an exception will be thrown if the key does not exist
  /// @return field value reference
  /// @throws `std::out_of_range` if key does not exist
  field &at(std::string key)
  {
    detail::trim(key);
    return data_.at(key);
  }
  // const overloading function
  const field &at(std::string key) const
  {
    detail::trim(key);
    return data_.at(key);
  }

  /// @brief Get the value corresponding to key. If key does not exist, return default_value.
  /// @param key key
  /// @param default_value default value - return default value when key does not exist
  /// @return field value (a copy)
  field get(std::string key, field default_value = field{}) const
  {
    detail::trim(key);
    if (data_.find(key) != data_.end())
    {
      return data_.at(key);
    }
    return default_value;
  }

  /// @brief Get all keys in the section.
  /// @return A vector containing all keys.
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

  /// @brief Get all values in the section.
  /// @return A vector containing all values, each value is a `ini::field` object.
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

  /// @brief Get all key-value pairs in the section.
  /// @return A vector containing all key-value pairs, each pair is a `std::pair<std::string, ini::field>`.
  std::vector<value_type> items() const
  {
    return {data_.begin(), data_.end()};
  }

  /// @brief Remove the specified key-value pairs
  /// @param key key
  /// @return Return true if the deletion is successful, return false if it is not found
  bool remove(std::string key)
  {
    detail::trim(key);
    return data_.erase(key) != 0;
  }

  /// @brief Clear all key-value pairs
  void clear() noexcept
  {
    data_.clear();
  }

  size_type size() const noexcept
  {
    return data_.size();
  }

  bool empty() const noexcept
  {
    return data_.empty();
  }

  iterator find(key_type key)
  {
    detail::trim(key);
    return data_.find(key);
  }
  const_iterator find(key_type key) const
  {
    detail::trim(key);
    return data_.find(key);
  }

  size_type count(key_type key) const
  {
    detail::trim(key);
    return data_.count(key);
  }

  iterator erase(iterator pos)
  {
    return data_.erase(pos);
  }
  iterator erase(const_iterator pos)
  {
    return data_.erase(pos);
  }
  iterator erase(const_iterator first, const_iterator last)
  {
    return data_.erase(first, last);
  }
  size_type erase(key_type key)
  {
    detail::trim(key);
    return data_.erase(key);
  }

  iterator begin() noexcept
  {
    return data_.begin();
  }
  const_iterator begin() const noexcept
  {
    return data_.begin();
  }

  iterator end() noexcept
  {
    return data_.end();
  }
  const_iterator end() const noexcept
  {
    return data_.end();
  }

  const_iterator cbegin() const noexcept
  {
    return data_.cbegin();
  }
  const_iterator cend() const noexcept
  {
    return data_.cend();
  }

  /// @brief Set `[section]` comment, overwriting the original comment.
  /// @param str Comment content, Multi-line comments are allowed, lines separated by `\n`.
  /// @param symbol Comment symbol, default is `;`, Only `;` and `#` are supported.
  void set_comment(const std::string &str, char symbol = ';')
  {
    comments_.set(str, symbol);
  }
  /// @brief Overwrite the current comment with another comment (copy).
  void set_comment(const comment &other)
  {
    comments_.set(other);
  }
  /// @brief Overwrite the current comment with another comment (move).
  void set_comment(comment &&other) noexcept
  {
    comments_.set(std::move(other));
  }
  /// @brief Set the comment from an initializer list of strings.
  /// @param list List of comment lines.
  /// @param symbol Comment symbol, default is `;`. Only `;` and `#` are supported.
  void set_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.set(list, symbol);
  }

  /// @brief Add `[section]` comments and then append them.
  /// @param str Comment content, Multi-line comments are allowed, lines separated by `\n`.
  /// @param symbol Comment symbol, default is `;`, Only `;` and `#` are supported.
  void add_comment(const std::string &str, char symbol = ';')
  {
    comments_.add(str, symbol);
  }
  /// @brief Append comments from another comment object (copy).
  void add_comment(const comment &other)
  {
    comments_.add(other);
  }
  /// @brief Append comments from another comment object (move).
  void add_comment(comment &&other) noexcept
  {
    comments_.add(std::move(other));
  }
  /// @brief Append comments from an initializer list of strings.
  /// @param list List of comment lines.
  /// @param symbol Comment symbol, default is `;`. Only `;` and `#` are supported.
  void add_comment(std::initializer_list<std::string> list, char symbol = ';')
  {
    comments_.add(list, symbol);
  }

  /// @brief Get a const reference to the comment associated with this field.
  /// @return Const reference to the internal `comment` object.
  const ini::comment &comment() const
  {
    return comments_;
  }
  /// @brief Get a mutable reference to the comment associated with this field.
  /// @return Reference to the internal `comment` object.
  ini::comment &comment()
  {
    return comments_;
  }

  /// @brief Clear `[section]` comment
  void clear_comment()
  {
    comments_.clear();
  }

 private:
  data_container data_;    // key-value pairs
  ini::comment comments_;  // section-level comments
};

/// @brief ini file class
template <typename Hash = std::hash<std::string>, typename Equal = std::equal_to<std::string>>
class basic_inifile
{
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

  void swap(basic_inifile &other) noexcept
  {
    using std::swap;
    swap(data_, other.data_);
  }

  friend void swap(basic_inifile &lhs, basic_inifile &rhs) noexcept
  {
    lhs.swap(rhs);
  }

  // 构造函数
  basic_inifile() = default;
  // 析构函数
  ~basic_inifile() = default;

  // 拷贝构造
  basic_inifile(const basic_inifile &other) = default;
  // 拷贝赋值
  basic_inifile &operator=(const basic_inifile &rhs) = default;

  // 移动构造
  basic_inifile(basic_inifile &&other) noexcept : data_(std::move(other.data_))
  {
    other.data_.clear();  // 显式清空, 跨平台行为一致
  };
  // 移动赋值 (move and swap)
  basic_inifile &operator=(basic_inifile &&rhs) noexcept
  {
    basic_inifile temp(std::move(rhs));  // move ctor
    swap(temp);                          // noexcept swap
    return *this;
  };

  /// @brief Get or insert a field. If section_name does not exist, insert a default constructed section object
  /// @param sec section name
  /// @return Returns the section reference corresponding to the key
  section &operator[](std::string sec)
  {
    detail::trim(sec);
    return data_[std::move(sec)];
  }

  /// @brief Set section key-value
  /// @tparam T Field value type
  /// @param sec Section name
  /// @param key Key
  /// @param value Field value
  /// @return Reference to the inserted or updated field
  template <typename T>
  field &set(std::string sec, std::string key, T &&value)
  {
    detail::trim(sec);
    detail::trim(key);
    return data_[std::move(sec)][std::move(key)] = std::forward<T>(value);
  }

  /// @brief Check if the specified section exists
  /// @param sec section name
  /// @return Return true if it exists, otherwise return false
  bool contains(std::string sec) const
  {
    detail::trim(sec);
    return data_.find(sec) != data_.end();
  }

  /// @brief Check if the specified key exists in the specified section
  /// @param sec section name
  /// @param key key
  /// @return Return true if it exists, otherwise return false
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

  /// @brief Returns a reference to the specified section.
  ///        If section does not exist, an exception of type `std::out_of_range` will be thrown.
  /// @param sec section-name - an exception will be thrown if the section does not exist
  /// @return section reference
  /// @throws `std::out_of_range` if section does not exist
  section &at(std::string sec)
  {
    detail::trim(sec);
    return data_.at(sec);
  }
  // const overloading function
  const section &at(std::string sec) const
  {
    detail::trim(sec);
    return data_.at(sec);
  }

  /// @brief Returns the field value of the specified section and the specified key
  /// @param sec section name
  /// @param key key
  /// @param default_value default value - the default value will be returned if the key does not exist
  /// @return field value(a copy)
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

  /// @brief Get all section names in the INI file.
  /// @return A vector containing all section names.
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

  /// @brief Remove the specified seciton
  /// @param sec section-name
  /// @return Return true if the deletion is successful, return false if it is not found
  bool remove(std::string sec)
  {
    detail::trim(sec);
    return data_.erase(sec) != 0;
  }

  void clear() noexcept
  {
    data_.clear();
  }

  size_type size() const noexcept
  {
    return data_.size();
  }

  bool empty() const noexcept
  {
    return data_.empty();
  }

  iterator find(key_type key)
  {
    detail::trim(key);
    return data_.find(key);
  }
  const_iterator find(key_type key) const
  {
    detail::trim(key);
    return data_.find(key);
  }

  size_type count(key_type key) const
  {
    detail::trim(key);
    return data_.count(key);
  }

  iterator erase(iterator pos)
  {
    return data_.erase(pos);
  }
  iterator erase(const_iterator pos)
  {
    return data_.erase(pos);
  }
  iterator erase(const_iterator first, const_iterator last)
  {
    return data_.erase(first, last);
  }
  size_type erase(key_type key)
  {
    detail::trim(key);
    return data_.erase(key);
  }

  iterator begin() noexcept
  {
    return data_.begin();
  }
  const_iterator begin() const noexcept
  {
    return data_.begin();
  }

  iterator end() noexcept
  {
    return data_.end();
  }
  const_iterator end() const noexcept
  {
    return data_.end();
  }

  const_iterator cbegin() const noexcept
  {
    return data_.cbegin();
  }
  const_iterator cend() const noexcept
  {
    return data_.cend();
  }

  /// @brief Read ini information from istream
  /// @param is istream
  void read(std::istream &is)
  {
    data_.clear();
    std::string line, current_section;
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

  /// @brief Write ini information to ostream
  /// @param os ostream
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

  /// @brief Read ini information from string
  /// @param str ini string
  void from_string(const std::string &str)
  {
    std::istringstream is(str);
    read(is);
  }

  /// @brief Convert the inifile object to a corresponding string
  /// @return ini string
  std::string to_string() const
  {
    std::ostringstream ss;
    write(ss);
    return ss.str();
  }

  /// @brief Load ini information from ini file
  /// @param filename Read file path
  /// @return Whether the loading is successful, return `true` if successful
  bool load(const std::string &filename)
  {
    std::ifstream is(filename);
    if (!is) return false;

    read(is);
    // 仅当 fail() 不是由于 EOF 造成的,并且没有发生 bad(),才认为读取成功
    return !(is.fail() && !is.eof()) && !is.bad();
  }

  /// @brief Save ini information to ini file
  /// @param filename Save file path
  /// @return Whether the save is successful, return `true` if successful
  bool save(const std::string &filename) const
  {
    std::ofstream os(filename);
    if (!os) return false;

    write(os);
    os.flush();
    return !os.fail() && !os.bad();
  }

 private:
  /// @brief 写注释内容
  /// @param os 输出流
  /// @param comments 注释内容
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

/// @brief Trims whitespace from both ends of the given string.
/// @param str The input string to be trimmed.
/// @return A new string with leading and trailing whitespace removed.
inline std::string trim(std::string str)
{
  detail::trim(str);
  return str;
}

/// @brief Splits a string into a vector of substrings based on a delimiter.
/// @param str The input string to be split.
/// @param delimiter The character used to split the string.
/// @param skip_empty If true, empty substrings are ignored; otherwise, they are included.
/// @return A vector of substrings obtained by splitting the input string.
inline std::vector<std::string> split(const std::string &str, char delimiter, bool skip_empty = false)
{
  return detail::split(str, std::string(1, delimiter), skip_empty);
}

/// @brief Splits a string into a vector of substrings based on a string delimiter.
/// @param str The input string to be split.
/// @param delimiter The substring used to split the string (can be multiple characters).
/// @param skip_empty If true, empty substrings are ignored; otherwise, they are included.
/// @return A vector of substrings obtained by splitting the input string.
inline std::vector<std::string> split(const std::string &str, const std::string &delimiter, bool skip_empty = false)
{
  return detail::split(str, delimiter, skip_empty);
}

/// @brief Joins elements of a sequence container into a string, separated by a character.
///        Note: The elements of the container must not be of pointer type.
/// @tparam Iterable Sequence container type (e.g., vector, list, set, array, deque) that supports begin() and end().
/// @param iterable The container whose elements are joined.
/// @param separator The character separating each element in the result.
/// @return A string with all elements separated by the given character.
template <typename Iterable>
inline std::string join(const Iterable &iterable, char separator)
{
  return detail::join(iterable, std::string(1, separator));
}

/// @brief Joins elements of a sequence container into a string, separated by a string.
///        Note: The elements of the container must not be of pointer type.
/// @tparam Iterable Sequence container type (e.g., vector, list, set, array, deque) that supports begin() and end().
/// @param iterable The container whose elements are joined.
/// @param separator The string separating each element in the result.
/// @return A string with all elements separated by the given string.
template <typename Iterable>
inline std::string join(const Iterable &iterable, const std::string &separator)
{
  return detail::join(iterable, separator);
}

/// @brief section class
using section = basic_section<>;
/// @brief inifile class
using inifile = basic_inifile<>;
/// @brief case_insensitive_section class
using case_insensitive_section = basic_section<detail::case_insensitive_hash, detail::case_insensitive_equal>;
/// @brief case_insensitive_inifile class
using case_insensitive_inifile = basic_inifile<detail::case_insensitive_hash, detail::case_insensitive_equal>;

}  // namespace ini

#endif  // INI_FILE_H_