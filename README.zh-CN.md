##  🌟 轻量级ini文件解析库

🌍 Languages/语言:  [English](README.md)  |  [简体中文](README.zh-CN.md)

### 📌 项目简介
这是一个轻量级、高效且 header-only 的 INI 配置文件解析库，专为 C++ 项目设计。它提供简洁直观的 API，支持快速解析、修改和写入 INI 文件，让配置管理变得更简单。

### 🚀 特性
- **轻量级 & 无依赖**：仅依赖 C++11 标准库，无需额外依赖项
- **易于集成**：Header-only 设计，开箱即用
- **直观 API**：提供清晰友好的接口，简化 INI 文件操作
- **全面支持**：可读取、修改、写入 INI 数据至文件
- **多种数据源**：支持从 `std::string` 或 `std::istream` 解析 INI 数据，并写入其中
- **自动类型转换**：支持多种数据类型，自动处理类型转换

适用于对 INI 配置文件有 **解析、编辑、存储** 需求的 C++ 项目。

### 📦 使用方式

**header-only方式**

1. 直接将[`inifile.h`](./include/inifile/inifile.h)头文件复制到您的项目文件夹中

2. 然后在源代码文件中直接`#include "inifile.h"`即可使用

**cmake方式**

1. 在项目中创建一个`inifile`文件夹(名称随意)

2. 将本项目的[`include`](./include/)文件夹中的所有内容复制到刚才步骤1创建的`inifile`文件夹内

3. 然后在您的主`CMakeLists.txt`中添加以下内容

   ```cmake
   add_subdirectory(inifile) # inifile为步骤1创建的文件夹名称
   ```

4. 在源代码中添加`#include <inifile/inifile.h>`既可使用

## 🛠️ 使用示例

下面提供简单的使用案例, 更多详细的案例请查看[`./examples/`](./examples/)文件夹下的案例

#### 创建并保存ini文件

```cpp
#include "inifile.h"
int main()
{
  constexpr char path[] = "path/to/ini/file";
  ini::inifile inif;
  inif["section"]["key0"] = true;
  inif["section"]["key1"] = 3.14159;
  inif["section"]["key2"] = "value";
  // Save the ini file, returns whether the save was successful or not.
  bool isok = inif.save(path);
}
```

#### 读取ini文件

```cpp
#include "inifile.h"
int main()
{
  constexpr char path[] = "path/to/ini/file";
  ini::inifile inif;
  // Load the ini file, return whether the loading was successful or not.
  bool isok = inif.load(path);
  bool b = inif["section"]["key0"];
  double d = inif["section"]["key1"];
  std::string s = inif["section"]["key2"];
}
```

#### `stream`流中读/写ini信息

支持`stream`流, 允许从`std::istream`流中读取ini数据, 也能向`std::ostream`中写入ini数据.

```cpp
#include "inifile.h"
int main()
{
  // create istream object "is" ...
  ini::inifile inif;
  inif.read(is);
}
```

```cpp
#include "inifile.h"
int main()
{
  // create ostream object "os" ...
  ini::inifile inif;
  inif.write(os);
}
```

#### `std::string`中读写ini信息

支持从`std::string`中读取ini数据, 也能将`inifile` 转为`std::string`.

```cpp
#include "inifile.h"
int main()
{
  // create string object "s" ...
  ini::inifile inif;
  inif.from_string(s);
}
```

```cpp
#include "inifile.h"
int main()
{
  ini::inifile inif;
  inif["section"]["key"] = "value";
  std::string s = inif.to_string();
}
```

#### 设置值

说明: 若section-key不存在, `operator[]`操作符和`set`函数会**直接插入**section-key, 若section-key存在则**更新**field值.

```cpp
#include "inifile.h"
int main()
{
  ini::inifile inif;
    
  /// Setting a single value
  inif["section"]["key1"] = "value";
  inif["section"]["key2"].set("hello");
  inif.set("section", "key3", true);
  inif["section"].set("key4", 3.14159);
    
  /// Setting multiple values for the same section, supporting different types
  inif["section2"].set({{"bool", false},
                        {"int", 123},
                        {"double", 999.888},
                        {"string", "ABC"},
                        {"char", 'm'}});
}
```

#### 获取值

说明: 获取值的时候需要注意以下两点:

- 给定的section-key是否存在, 当section-key不存在时调用不同的函数会有不同的策略处理;
  - 使用`operator[]`返回**引用**, 若给定section或key不存在则**会插入**空的field值, 并设置field为空字符串. (行为类似`std::map`的[])
  - 使用`get()`函数返回**值**, 若给定的section或key不存在**不会插入**field, 而是返回一个默认的空field值(可以指定默认值).
  - 使用`at()`函数返回**引用**, 若给定的section或key不存在则**抛出异常** :`std::out_of_range`
- 是否可以执行类型自动转换, 以上三个函数返回的是`ini::field`包装对象, 若将该对象转为其他类型需注意:
  - 类型转换是否允许, 若类型转换不允许则**抛出异常**: `std::invalid_argument`, (例如将`"abc"`转为`int`)
  - 数值类型转换范围是否溢出, 若超出目标类型的范围则**抛出异常**: `std::out_of_range`, (例如将`INT_MAX`转为`uint8_t`)

```cpp
#include "inifile.h"
int main()
{
  ini::inifile inif;
    
  /// automatic type conversion
  std::string s0 = inif["section"]["key1"];
  bool isok = inif["section"]["key2"];
    
  int ii0 = inif.get("section", "key3");
  int ii2 = inif.get("section", "key3", -1); // Specify default values
    
  std::string ss2 = inif["section"].get("key4");
  std::string ss3 = inif["section"].get("key5", "default"); // Specify default values
    
  double dd0 = inif.at("section").at("key");
}
```

#### 关于自动类型转换

自动类型转换作用在`ini::field`对象上, 允许`ini::field` <=> `other type`互相转换; 但是需要注意: **若转换失败会抛出异常.**

```cpp
#include "inifile.h"
int main()
{
  /// other type -> ini::field
  ini::field f(true);
  ini::field f1(10);
  ini::field f2 = 3.14;
  ini::field f3 = 'c';
  ini::field f4 = "abc";
  
  /// ini::field -> other type
  bool b = f;
  int i = f1;
  double d = f2;
  char c = f3;
  std::string s = f4;
  
  ini::inifile inif;
  inif["section"]["key"] = true; // bool -> ini::field
  
  /// Get direct type(ini::field)
  auto val = inif["section"]["key"]; // val type is ini::field
  ini::field val2 = inif["section"]["key"]; 
  
  /// explicit type conversion
  bool bb = inif["section"]["key"].as<bool>();
    
  /// automatic type conversion
  bool bb2 = inif["section"]["key"];
  
  /// Type conversion failure throws an exception
  double n =  inif["section"]["key"]; // error: Converting true to double is not allowed.
}
```

支持自动转换的类型包括:

- `bool`
- `char`, `signed char`, `unsigned char`
- `short`, `unsigned short`
- `int`, `unsigned int`
- `long`, `unsigned long`
- `long long`, `unsigned long long`
- `float`
- `double`
- `long double`
- `std::string`
- `const char *`
- `std::string_view` (C++17)

#### 其他工具函数

提供多种工具函数, 查询个数`size()`,  是否包含元素`contains()`,  移除元素 `remove()`,  清除所有元素`clear()`,  迭代器访问: `begin()`, `end()`, `cbegin()`, `cend()`, 支持range-base for循环.  具体详情请查看常用 API 说明. 下面提供一个迭代器访问ini信息:

```cpp
#include "inifile.h"
int main()
{
  constexpr char path[] = "path/to/ini/file";
  ini::inifile inif;
  bool isok = inif.load(path);
  for (const auto &sec_pair : inif)
  {
    const std::string &section_name = sec_pair.first;
    const ini::section &section = sec_pair.second;
    std::cout << "  section '" << section_name << "' has " << section.size() << " key-values." << std::endl;

    for (const auto &kv : section)
    {
      const std::string &key = kv.first;
      const auto &value = kv.second;
      std::cout << "    kv: '" << key << "' = '" << value << "'" << std::endl;
    }
  }
}
```



### 📄 常用API说明

#### class 类型说明

| class名称    | 描述                                                         |
| ------------ | ------------------------------------------------------------ |
| ini::inifile | 对应整个ini数据, 包含了所有的section                         |
| ini::section | 对应整个section内容, 里面包含了本section所有的key-value值    |
| ini::field   | 对应ini文件中的 value 字段, 支持多种数据类型,  支持自动类型转换 |

#### ini::field类API说明

以下函数类型转换失败或者值溢出将抛异常

| 函数名     | 函数签名                         | 功能描述                          |
| ---------- | -------------------------------- | --------------------------------- |
| field      | `field(const T &other)`          | 构造field对象, 将T类型转为field值 |
| set        | `void set(const T &value)`       | 设置field值, 将T类型转为field值   |
| operator=  | `field &operator=(const T &rhs)` | 设置field值, 将T类型转为field值   |
| operator T | `operator T() const`             | 将field类型转为T类型              |
| as         | `T as() const`                   | 将field类型转为T类型              |

#### ini::section类API说明

| 函数名     | 函数签名                                                     | 功能描述                                                     |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| operator[] | `field &operator[](const std::string &key)`                  | 返回ini::field引用, 不存在则插入空ini::field                 |
| set        | `void set(std::string key, T &&value)`                       | 插入或更新指定key的field                                     |
| contains   | `bool contains(std::string key) const`                       | 判断key是否存在                                              |
| at         | `field &at(std::string key)`                                 | 返回指定key键的元素的字段值的引用。如果元素不存则抛 std::out_of_range异常 |
| get        | `field get(std::string key, field default_value = field{}) const` | 获取key对应的值(副本), 若key不存在则返回default_value默认值  |
| remove     | `bool remove(std::string key)`                               | 删除指定的key-value键值对, 若不存在则什么都不做              |
| clear      | `void clear() noexcept`                                      | 清除所有key - value键值对                                    |
| size       | `size_type size() const noexcept`                            | 返回有多少key - value键值对                                  |
| begin      | `iterator begin() noexcept`                                  | 返回起始迭代器                                               |
| end        | `iterator end() noexcept`                                    | 返回末尾迭代器                                               |

#### ini::inifile类API说明

| 函数名      | 函数签名                                                     | 功能描述                                                     |
| ----------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| operator[]  | `section &operator[](const std::string &section)`            | 返回ini::section引用, 不存在则插入空ini::section             |
| set         | `void set(const std::string &section, const std::string &key, T &&value)` | 设置section key-value                                        |
| contains    | `bool contains(std::string section) const`                   | 判断指定的section是否存在                                    |
| contains    | `bool contains(std::string section, std::string key) const`  | 判断指定section下指定的key是否存在                           |
| at          | `section &at(std::string section)`                           | 返回指定section的引用。如果不存在这样的元素，则会抛出 std::out_of_range 类型的异常 |
| get         | `field get(std::string sec, std::string key, field default_value = field{}) const` | 返回指定section的指定key键的字段值, 若section或key不存在则返回默认值default_value |
| remove      | `bool remove(std::string sec)`                               | 删除指定的section(包括其所有元素)                            |
| clear       | `void clear() noexcept`                                      | 清空所有的section                                            |
| size        | `size_type size() const noexcept`                            | 返回有多少section                                            |
| begin       | `iterator begin() noexcept`                                  | 返回起始迭代器                                               |
| end         | `iterator end() noexcept`                                    | 返回末尾迭代器                                               |
| read        | `void read(std::istream &is)`                                | 从istream中读取ini信息                                       |
| write       | `void write(std::ostream &os) const`                         | 向ostream中写入ini信息                                       |
| from_string | `void from_string(const std::string &str)`                   | 从string中读取ini信息                                        |
| to_strig    | `std::string to_string() const`                              | 将inifile对象转为对应字符串                                  |
| load        | `bool load(const std::string &filename)`                     | 从ini文件中加载ini信息, 返回是否成功                         |
| save        | `bool save(const std::string &filename)`                     | 将ini信息保存到ini文件, 返回是否成功                         |

### 💡 贡献指南

欢迎提交 **Issue** 和 **Pull request** 来改进本项目！

### 📜 许可证

本项目采用[ **MIT** 许可证](./LICENSE)。
