/**
 * @description: Small inifile parsing library, supports parsing, modifying and saving ini files.
 * @author: abin
 * @date: 2025-02-23
 * @license: MIT
 */

#ifndef INI_FILE_H_
#define INI_FILE_H_

#include <iostream>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <string>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <limits>

#ifdef __cpp_lib_string_view // If we have std::string_view
#include <string_view>
#endif

namespace ini
{

  namespace detail
  {
    /** whitespace characters. */
    static constexpr const char whitespaces[] = " \t\n\r\f\v";

    // 除去str两端空白字符
    inline void trim(std::string &str)
    {
      if (str.empty())
      {
        return;
      }
      auto start = str.find_first_not_of(whitespaces);
      auto end = str.find_last_not_of(whitespaces);
      if (start == std::string::npos) // 全是空白
      {
        str.clear();
      }
      else
      {
        str = str.substr(start, end - start + 1);
      }
    }

    /**
     * @brief 通用转换模板，未特化的 Convert 结构体
     * 由于 SFINAE（替换失败不算错误）原则，未特化的 Convert 不能实例化
     */
    template <typename T, typename Enable = void>
    struct Convert;

    /**
     * @brief Convert<bool> 特化版本
     * 提供 `decode` 和 `encode` 方法，支持 `bool` 与 `std::string` 之间的转换
     */
    template <>
    struct Convert<bool>
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
      void decode(const std::string &value, bool &result)
      {
        std::string str(value); // 复制字符串, 避免修改原始数据
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
                       { return std::tolower(c); });
        // 除了 "false"、"0" 和空串，其他都应该为 true
        result = !(str == "false" || str == "0" || str.empty());
      }

      /**
       * @brief 将 bool 值转换为 std::string
       * @param value 布尔值
       * @param result 输出字符串："true" 或 "false"
       */
      void encode(const bool value, std::string &result)
      {
        result = value ? "true" : "false";
      }
    };

    template <>
    struct Convert<char>
    {
      void decode(const std::string &value, char &result)
      {
        result = value.empty() ? '\0' : value[0];
      }
      void encode(const char value, std::string &result)
      {
        result = value;
      }
    };

    template <>
    struct Convert<unsigned char>
    {
      void decode(const std::string &value, unsigned char &result)
      {
        result = value.empty() ? '\0' : static_cast<unsigned char>(value[0]);
      }

      void encode(const unsigned char value, std::string &result)
      {
        result = std::string(1, value);
      }
    };

    template <>
    struct Convert<signed char>
    {
      void decode(const std::string &value, signed char &result)
      {
        result = value.empty() ? '\0' : static_cast<signed char>(value[0]);
      }

      void encode(const signed char value, std::string &result)
      {
        result = std::string(1, value);
      }
    };

    // 处理 `std::string`
    template <>
    struct Convert<std::string>
    {
      void decode(const std::string &value, std::string &result)
      {
        result = value;
      }

      void encode(const std::string &value, std::string &result)
      {
        result = value;
      }
    };

    // 处理 `const char*`
    template <>
    struct Convert<const char *>
    {
      void decode(const std::string &value, const char *&result)
      {
        result = value.c_str();
      }

      void encode(const char *value, std::string &result)
      {
        result = value;
      }
    };

    /**
     * @brief Convert 模板特化：处理所有整数类型（不包括 `char`、`wchar_t`、`char16_t` 等）
     *
     * 该模板特化适用于所有 `std::is_integral<T>::value` 为 `true` 的类型，
     * 但排除了 `char`、`signed char`、`unsigned char`、`wchar_t` 以及
     * C++11 及以上版本中的 `char16_t` 和 `char32_t`。
     *
     * 在 C++20 及以上版本，还会排除 `char8_t`。
     */
    template <typename T>
    struct Convert<T, typename std::enable_if<
                          std::is_integral<T>::value &&
                          !std::is_same<T, char>::value &&
                          !std::is_same<T, signed char>::value &&
                          !std::is_same<T, unsigned char>::value &&
                          !std::is_same<T, wchar_t>::value &&
#if __cplusplus >= 202002L // 仅在 C++20 及以上排除 char8_t
                          !std::is_same<T, char8_t>::value &&
#endif
                          !std::is_same<T, char16_t>::value &&
                          !std::is_same<T, char32_t>::value>::type>
    {

      /**
       * @brief 将字符串转换为整数
       * @param value 输入的字符串
       * @param result 转换后的整数
       *
       * - 处理空字符串，默认返回 `0`
       * - 使用 `std::strtoll()` / `std::strtoull()` 进行转换
       * - 检查 `errno == ERANGE`，防止溢出
       * - 确保转换值在 `T` 的范围内
       * - 检查 `endPtr` 以确保完整转换
       */
      void decode(const std::string &value, T &result)
      {
        if (value.empty())
        {
          result = 0;
          return;
        }

        char *endPtr = nullptr;
        errno = 0; // 清除错误状态

        if (std::is_signed<T>::value)
        {
          long long temp = std::strtoll(value.c_str(), &endPtr, 10);
          if (errno == ERANGE || temp < std::numeric_limits<T>::min() || temp > std::numeric_limits<T>::max())
          {
            result = 0;
            return;
          }
          result = static_cast<T>(temp);
        }
        else
        {
          unsigned long long temp = std::strtoull(value.c_str(), &endPtr, 10);
          if (errno == ERANGE || temp > std::numeric_limits<T>::max())
          {
            result = 0;
            return;
          }
          result = static_cast<T>(temp);
        }

        if (endPtr == value.c_str() || *endPtr != '\0') // 检查是否转换完整
        {
          result = 0;
        }
      }

      /**
       * @brief 将整数转换为字符串
       * @param value 需要转换的整数
       * @param result 转换后的字符串
       *
       * - 直接调用 `std::to_string()` 进行转换
       */
      void encode(const T value, std::string &result)
      {
        result = std::to_string(value);
      }
    };

    /**
     * @brief Convert 模板特化：处理浮点数类型 (`float`, `double`, `long double`)
     *
     * 该模板特化适用于所有 `std::is_floating_point<T>::value` 为 `true` 的类型，
     * 即 `float`、`double` 和 `long double`。
     */
    template <typename T>
    struct Convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
    {
      /**
       * @brief 将字符串转换为浮点数
       * @param value 输入的字符串
       * @param result 转换后的浮点数
       *
       * - 处理空字符串，默认返回 `0.0`
       * - 使用 `std::strtold()` 进行转换，以支持 `long double` 的精度
       * - 检查 `errno == ERANGE`，防止溢出
       * - 确保转换值在 `T` 的范围内
       * - 检查 `endPtr` 以确保完整转换
       */
      void decode(const std::string &value, T &result)
      {
        if (value.empty())
        {
          result = 0.0;
          return;
        }

        char *endPtr = nullptr;
        errno = 0;
        long double temp = std::strtold(value.c_str(), &endPtr);

        if (errno == ERANGE || temp < std::numeric_limits<T>::lowest() || temp > std::numeric_limits<T>::max())
        {
          result = 0.0;
          return;
        }

        result = static_cast<T>(temp);

        if (endPtr == value.c_str() || *endPtr != '\0') // 检查是否转换完整
        {
          result = 0.0;
        }
      }

      /**
       * @brief 将浮点数转换为字符串
       * @param value 需要转换的浮点数
       * @param result 转换后的字符串
       *
       * - 直接调用 `std::to_string()` 进行转换
       * - `std::to_string()` 可能会影响精度，对于高精度需求可使用 `std::stringstream`
       */
      void encode(const T value, std::string &result)
      {
        result = std::to_string(value);
      }
    };

#ifdef __cpp_lib_string_view
    template <>
    struct Convert<std::string_view>
    {
      void decode(const std::string &value, std::string_view &result)
      {
        result = value;
      }

      void encode(const std::string_view value, std::string &result)
      {
        result = value;
      }
    };
#endif

  } // namespace detail

} // namespace ini

#endif // INI_FILE_H_