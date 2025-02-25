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

    template <typename T, typename Enable = void>
    struct Convert;

    template <>
    struct Convert<bool>
    {
      void decode(const std::string &value, bool &result)
      {
        std::string str(value);
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
                       { return std::tolower(c); });
        // 除了 "false"、"0" 和空串，其他都应该为 true
        result = !(str == "false" || str == "0" || str.empty());
      }
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

    // 处理所有整数类型 (`short`, `int`, `long`, `long long` 等)
    template <typename T>
    struct Convert<T, typename std::enable_if<
                          std::is_integral<T>::value &&
                          !std::is_same<T, char>::value &&
                          !std::is_same<T, signed char>::value &&
                          !std::is_same<T, unsigned char>::value &&
                          !std::is_same<T, wchar_t>::value>::type>
    {
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

      void encode(const T value, std::string &result)
      {
        result = std::to_string(value);
      }
    };

    // 处理浮点类型 (`float`, `double`, `long double`)
    template <typename T>
    struct Convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
    {
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

        if (errno == ERANGE || temp < -std::numeric_limits<T>::max() || temp > std::numeric_limits<T>::max())
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