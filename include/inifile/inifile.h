/**************************************************************************************************************
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * @description: Easy-to-use ini file parsing library that supports parsing, modifying and saving ini files.
 * - Features :
 *   - Lightweight & Easy-to-Use: A header-only INI parser with no external dependencies (C++11 only).  
 *   - Read, Modify & Write: Easily handle INI configuration files. 
 *   - Intuitive API: Simple and clear interface for reading, modifying, and writing INI files.  
 *   - Versatile Data Handling: Supports `std::string` and `std::istream` for input/output.  
 *   - Automatic Type Conversion: Seamlessly handles various data types.  
 * 
 * 
 * @author: abin
 * @date: 2025-02-23
 * @license: MIT
 * @repository: https://github.com/abin-z/IniFile
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 **************************************************************************************************************/

#ifndef INI_FILE_H_
#define INI_FILE_H_

#include <string>
#include <limits>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <stdexcept>
#include <cstdlib>
#include <cerrno>

#ifdef __cpp_lib_string_view // If we have std::string_view
#include <string_view>
#endif

namespace ini
{

  namespace detail
  {
    /** whitespace characters. */
    static constexpr char whitespaces[] = " \t\n\r\f\v";

    // 除去str两端空白字符
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

    /**
     * @brief 通用转换模板，未特化的 convert 结构体
     * 由于 SFINAE（替换失败不算错误）原则，未特化的 convert 不能实例化
     */
    template <typename T, typename Enable = void>
    struct convert;

    /**
     * @brief convert<bool> 特化版本
     * 提供 `decode` 和 `encode` 方法，支持 `bool` 与 `std::string` 之间的转换
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
    struct convert<char>
    {
      void decode(const std::string &value, char &result)
      {
        if (value.empty())
        {
          throw std::invalid_argument("<inifile> Cannot convert empty string to char.");
        }
        result = value[0];
      }
      void encode(const char value, std::string &result)
      {
        result = std::string(1, value);
      }
    };

    template <>
    struct convert<unsigned char>
    {
      void decode(const std::string &value, unsigned char &result)
      {
        if (value.empty())
        {
          throw std::invalid_argument("<inifile> Cannot convert empty string to unsigned char.");
        }
        result = static_cast<unsigned char>(value[0]);
      }

      void encode(const unsigned char value, std::string &result)
      {
        result = std::string(1, value);
      }
    };

    template <>
    struct convert<signed char>
    {
      void decode(const std::string &value, signed char &result)
      {
        if (value.empty())
        {
          throw std::invalid_argument("<inifile> Cannot convert empty string to signed char.");
        }
        result = static_cast<signed char>(value[0]);
      }

      void encode(const signed char value, std::string &result)
      {
        result = std::string(1, value);
      }
    };

    // 处理 `std::string`
    template <>
    struct convert<std::string>
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
    struct convert<const char *>
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

    // 处理 `char *`
    template <>
    struct convert<char *>
    {
      void encode(char *value, std::string &result)
      {
        result = value;
      }
    };

    // 处理 `char[N]` 类型（即固定大小的字符数组）
    template <std::size_t N>
    struct convert<char[N]>
    {
      void encode(const char (&value)[N], std::string &result)
      {
        result = value;
      }
    };

    /**
     * @brief char系列类型特征：所有字符类型（包括 const 和引用的 char 类型）
     * 该特征判断类型是否为 char 系列类型（包括 `char`、`signed char`、`unsigned char`、`wchar_t`、`char8_t`、`char16_t` 和 `char32_t`）。
     * 它会移除 `const` 和 `reference` 修饰符，以确保正确判断字符类型。
     */
    template <typename T>
    struct is_char_type
        : std::integral_constant<
              bool,
              std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, char>::value ||
                  std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, signed char>::value ||
                  std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, unsigned char>::value ||
                  std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, wchar_t>::value ||
#if defined(__cpp_char8_t) // 仅在 C++20 及更高版本支持 char8_t
                  std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, char8_t>::value ||
#endif
                  std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, char16_t>::value ||
                  std::is_same<typename std::remove_reference<typename std::remove_const<T>::type>::type, char32_t>::value>
    {
    };

    /**
     * @brief convert 模板特化：处理所有整数类型（不包括字符类型 `char`、`signed char`、`unsigned char`、`wchar_t`、`char16_t`、`char32_t` 等）
     *
     * 该模板特化适用于所有 `std::is_integral<T>::value` 为 `true` 的类型，但排除了以下字符类型：
     * - `char`、`signed char`、`unsigned char`
     * - `wchar_t`（宽字符类型）
     * - `char16_t`、`char32_t`（UTF-16 和 UTF-32 编码字符类型）
     * - 在 C++20 及以上版本中，排除 `char8_t`（用于 UTF-8 编码的字符类型）。
     *
     * 该特化确保只有整型类型（例如 `int`、`long` 等）能够匹配，而字符类型将被排除。
     */
    template <typename T>
    struct convert<T, typename std::enable_if<std::is_integral<T>::value && !is_char_type<T>::value>::type>
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
          throw std::invalid_argument("<inifile> Cannot convert empty string to integer.");
        }

        char *endPtr = nullptr;
        errno = 0; // 清除错误状态

        if (std::is_signed<T>::value)
        {
          long long temp = std::strtoll(value.c_str(), &endPtr, 10);
          if (errno == ERANGE || temp < std::numeric_limits<T>::min() || temp > std::numeric_limits<T>::max())
          {
            throw std::out_of_range("<inifile> Integer conversion out of range.");
          }
          result = static_cast<T>(temp);
        }
        else
        {
          unsigned long long temp = std::strtoull(value.c_str(), &endPtr, 10);
          if (errno == ERANGE || temp > std::numeric_limits<T>::max())
          {
            throw std::out_of_range("<inifile> Unsigned integer conversion out of range.");
          }
          result = static_cast<T>(temp);
        }

        if (endPtr == value.c_str() || *endPtr != '\0') // 检查是否转换完整
        {
          throw std::invalid_argument("<inifile> Invalid integer format: " + value);
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
     * @brief convert 模板特化：处理浮点数类型 (`float`, `double`, `long double`)
     *
     * 该模板特化适用于所有 `std::is_floating_point<T>::value` 为 `true` 的类型，
     * 即 `float`、`double` 和 `long double`。
     */
    template <typename T>
    struct convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
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
          throw std::invalid_argument("<inifile> Cannot convert empty string to floating point.");
        }

        char *endPtr = nullptr;
        errno = 0;
        long double temp = std::strtold(value.c_str(), &endPtr);

        if (errno == ERANGE || temp < std::numeric_limits<T>::lowest() || temp > std::numeric_limits<T>::max())
        {
          throw std::out_of_range("<inifile> Floating point conversion out of range.");
        }

        result = static_cast<T>(temp);

        if (endPtr == value.c_str() || *endPtr != '\0') // 检查是否转换完整
        {
          throw std::invalid_argument("<inifile> Invalid floating point format: " + value);
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
    struct convert<std::string_view>
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

  /// @brief ini文件的字段值
  class field
  {
    friend std::ostream &operator<<(std::ostream &os, const field &data);

  public:
    /// 默认构造函数，使用编译器生成的默认实现。
    field() = default;

    /// 参数化构造函数，通过给定的字符串初始化 value_。
    field(const std::string &value) : value_(value) {}

    /// 默认析构函数，使用编译器生成的默认实现。
    ~field() = default;

    /// 默认拷贝构造函数，按值拷贝另一个 field 对象。
    field(const field &other) = default;

    /// 默认拷贝赋值运算符，按值拷贝另一个 field 对象。
    field &operator=(const field &rhs) = default;

    /// 默认移动构造函数，按值移动另一个 field 对象。
    field(field &&other) noexcept = default;

    /// 默认移动赋值运算符，按值移动另一个 field 对象。
    field &operator=(field &&rhs) noexcept = default;

    /**
     * @brief 模板构造函数：从其他类型的值构造 `field` 对象。
     *
     * @tparam T 传入的值的类型
     * @param other 传入的值，类型为 `T`，将被转换并存储到 `value_` 中
     */
    template <typename T>
    field(const T &other)
    {
      detail::convert<T> conv;    // 创建一个转换器对象，处理 T 类型到字符串的转换
      conv.encode(other, value_); // 将传入的值编码成字符串并存储到 value_ 中
    }

    /**
     * @brief 类型赋值操作符：允许将其他类型的值赋值给 `field` 对象。
     *
     * @tparam T 赋值操作符所接收的值的类型
     * @param rhs 右侧值，类型为 `T`，会被转换为字符串并赋值给当前对象
     * @return 当前对象的引用，支持链式赋值
     */
    template <typename T>
    field &operator=(const T &rhs)
    {
      detail::convert<T> conv;  // 创建一个转换器对象，处理 T 类型到字符串的转换
      conv.encode(rhs, value_); // 将右侧值编码成字符串并存储到 value_ 中
      return *this;             // 返回当前对象的引用，支持链式赋值
    }

    /**
     * 将 INI 字段的值转换为目标类型 T。
     * 使用一个转换器（假设为 detail::convert<T>）来执行解码操作。
     * 例如：将字符串值转换为 int、double 或其他类型。
     *
     * @tparam T 目标类型
     * @return 转换后的值
     */
    template <typename T>
    T as() const
    {
      detail::convert<T> conv;     // 创建一个转换器对象
      T result;                    // 用于存储转换后的结果
      conv.decode(value_, result); // 将 value_ 字符串解码为目标类型 T
      return result;               // 返回转换结果
    }

    /**
     * 类型转换操作符：允许将 field 对象转换为目标类型 T。
     * 实际上，它会调用 `as<T>()` 函数来执行类型转换。
     *
     * @tparam T 目标类型
     * @return 转换后的目标类型的值
     */
    template <typename T>
    operator T() const
    {
      return this->as<T>(); // 使用 as<T> 方法将值转换为目标类型 T
    }

    /**
     * 设置字段值：将传入的值编码并存储到 value_ 中。
     * 使用一个转换器（假设为 detail::convert<T>）将给定的值编码为字符串。
     *
     * @tparam T 设置的值的类型
     * @param value 需要设置的值
     */
    template <typename T>
    void set(const T &value)
    {
      detail::convert<T> conv;
      conv.encode(value, value_);
    }

  private:
    std::string value_; // 存储字符串值，用于存储读取的 INI 文件字段值
  };

  inline std::ostream &operator<<(std::ostream &os, const field &data)
  {
    return os << data.value_;
  }

  /// @brief ini文件的section
  class section
  {
    using DataContainer = std::unordered_map<std::string, field>;

  public:
    using key_type = DataContainer::key_type;
    using mapped_type = DataContainer::mapped_type;
    using value_type = DataContainer::value_type;
    using size_type = DataContainer::size_type;
    using difference_type = DataContainer::difference_type;

    using iterator = DataContainer::iterator;
    using const_iterator = DataContainer::const_iterator;

    /// @brief 获取或插入一个字段，键是常量引用, 如果key不存在，插入一个默认构造的 field 对象
    /// @param key 键名称
    /// @return 返回对应键的 field 引用
    field &operator[](const std::string &key)
    {
      return data_[key];
    }
    field &operator[](std::string &&key)
    {
      return data_[std::move(key)];
    }

    /// @brief 设置值
    /// @tparam T
    /// @param key
    /// @param value
    template <typename T>
    void set(std::string key, T &&value)
    {
      detail::trim(key);
      data_[std::move(key)] = std::forward<T>(value);
    }

    void set(std::initializer_list<std::pair<std::string, field>> args)
    {
      for (auto &&pair : args)
      {
        data_[std::move(pair.first)] = std::move(pair.second);
      }
    }

    /// @brief key是否存在
    /// @param key
    /// @return
    bool contains(std::string key) const
    {
      detail::trim(key);
      return data_.find(key) != data_.end();
    }

    /// @brief 返回指定key键的元素的字段值的引用。如果不存在这样的元素，则会出现 std::out_of_range 类型的异常。
    /// @param key key键 - key不存在将抛出异常
    /// @return 字段值引用
    field &at(std::string key)
    {
      detail::trim(key);
      return data_.at(key);
    }
    // const 重载函数
    const field &at(std::string key) const
    {
      detail::trim(key);
      return data_.at(key);
    }

    /// @brief 获取key对应的值(副本), 若key不存在则返回default_value默认值
    /// @param key key键
    /// @param default_value 默认值 - 当key不存在返回默认值
    /// @return 字段值
    field get(std::string key, field default_value = field{}) const
    {
      detail::trim(key);
      if (data_.find(key) != data_.end())
      {
        return data_.at(key);
      }
      return default_value;
    }

    /// @brief 删除值
    /// @param key
    /// @return 是否删除成功
    bool remove(std::string key)
    {
      detail::trim(key);
      return data_.erase(key) != 0;
    }

    /// @brief 清除所有key - value
    void clear() noexcept
    {
      data_.clear();
    }

    size_type size() const noexcept
    {
      return data_.size();
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

  private:
    DataContainer data_; // key - value
  };

  /// @brief ini文件类
  class inifile
  {
    using DataContainer = std::unordered_map<std::string, section>;

  public:
    using key_type = DataContainer::key_type;
    using mapped_type = DataContainer::mapped_type;
    using value_type = DataContainer::value_type;
    using size_type = DataContainer::size_type;
    using difference_type = DataContainer::difference_type;

    using iterator = DataContainer::iterator;
    using const_iterator = DataContainer::const_iterator;

    /// @brief 获取或插入一个字段，键是常量引用, 如果section_name不存在，插入一个默认构造的 section 对象
    /// @param section section名称
    /// @return 返回对应键的 section 引用
    section &operator[](const std::string &section)
    {
      return data_[section];
    }
    section &operator[](std::string &&section)
    {
      return data_[std::move(section)];
    }

    /// @brief 设置section key-value
    /// @tparam T 字段值类型
    /// @param section section名称
    /// @param key key键
    /// @param value 字段值
    template <typename T>
    void set(const std::string &section, const std::string &key, T &&value)
    {
      data_[section][key] = std::forward<T>(value);
    }

    /// @brief 判断指定的section是否存在
    /// @param section section名称
    /// @return 是否存在
    bool contains(std::string section) const
    {
      detail::trim(section);
      return data_.find(section) != data_.end();
    }

    /// @brief 判断指定section下指定的key是否存在
    /// @param section section名称
    /// @param key key键
    /// @return 是否存在
    bool contains(std::string section, std::string key) const
    {
      detail::trim(section);
      auto sec_it = data_.find(section);
      if (sec_it != data_.end())
      {
        return sec_it->second.contains(std::move(key));
      }
      return false;
    }

    /// @brief 返回指定section的引用。如果不存在这样的元素，则会抛出 std::out_of_range 类型的异常。
    /// @param section section名称 - section不存在将抛出异常
    /// @return section引用
    section &at(std::string section)
    {
      detail::trim(section);
      return data_.at(section);
    }
    const section &at(std::string section) const
    {
      detail::trim(section);
      return data_.at(section);
    }

    /// @brief 返回指定section的指定key键的字段值
    /// @param sec section名称
    /// @param key key键
    /// @param default_value 默认值 - key不存在时将返回该默认值
    /// @return 字段值
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

    /// @brief 从istream中读取ini信息
    /// @param is istream
    void read(std::istream &is)
    {
      data_.clear();
      std::string line, current_section;
      while (std::getline(is, line))
      {
        detail::trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') // 跳过注释
        {
          continue;
        }
        if (line.front() == '[' && line.back() == ']') // 处理section
        {
          current_section = line.substr(1, line.size() - 2);
          detail::trim(current_section);
          if (!current_section.empty())
          {
            data_[current_section]; // 添加没有key=value的section
          }
        }
        else // 处理key=value
        {
          auto pos = line.find('=');
          if (pos != std::string::npos)
          {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            detail::trim(key);
            detail::trim(value);
            data_[current_section][key] = value; // 允许section为空字符串
          }
        }
      }
    }

    /// @brief 向ostream中写入ini信息
    /// @param os ostream
    void write(std::ostream &os) const
    {
      bool first_section = true;

      // 先处理空 section（无 section 的键值对）
      auto it = data_.find("");
      if (it != data_.end())
      {
        for (const auto &kv : it->second)
        {
          os << kv.first << "=" << kv.second << "\n";
        }
        first_section = false;
      }

      // 处理非空 section
      for (const auto &sec : data_)
      {
        if (sec.first.empty()) // 空 section 已经写过了
          continue;

        if (!first_section)
          os << "\n"; // Section 之间插入空行
        first_section = false;
        os << "[" << sec.first << "]\n";
        for (const auto &kv : sec.second)
        {
          os << kv.first << "=" << kv.second << "\n";
        }
      }
    }

    /// @brief 从str中读取ini信息
    /// @param str ini字符串
    void from_string(const std::string &str)
    {
      std::istringstream is(str);
      read(is);
    }

    /// @brief 将inifile对象转为对应字符串
    /// @return ini字符串
    std::string to_string() const
    {
      std::ostringstream ss;
      write(ss);
      return ss.str();
    }

    /// @brief 从ini文件中加载ini信息
    /// @param filename 读取文件路径
    /// @return 是否读取成功
    bool load(const std::string &filename)
    {
      std::ifstream is(filename);
      if (!is)
        return false;

      read(is);
      // 仅当 fail() 不是由于 EOF 造成的，并且没有发生 bad()，才认为读取成功
      return !(is.fail() && !is.eof()) && !is.bad();
    }

    /// @brief 将ini信息保存到ini文件
    /// @param filename 保存文件路径
    /// @return 是否保存成功
    bool save(const std::string &filename)
    {
      std::ofstream os(filename);
      if (!os)
        return false;

      write(os);
      os.flush();
      return !os.fail() && !os.bad();
    }

  private:
    DataContainer data_; // section_name - key_value
  };

} // namespace ini

#endif // INI_FILE_H_