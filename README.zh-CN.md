##  🌟 轻量级ini文件解析库

[![iniparser](https://img.shields.io/badge/INI_Parser-8A2BE2)](https://github.com/abin-z/IniFile) [![headeronly](https://img.shields.io/badge/Header_Only-green)](https://github.com/abin-z/IniFile/blob/main/include/inifile/inifile.h) [![moderncpp](https://img.shields.io/badge/Modern_C%2B%2B-218c73)](https://learn.microsoft.com/en-us/cpp/cpp/welcome-back-to-cpp-modern-cpp?view=msvc-170) [![licenseMIT](https://img.shields.io/badge/License-MIT-green)](https://opensource.org/license/MIT) [![version](https://img.shields.io/badge/version-0.9.7-green)](https://github.com/abin-z/IniFile/releases)

🌍 Languages/语言:  [English](README.md)  |  [简体中文](README.zh-CN.md)

### 📌 项目简介
这是一个轻量级、跨平台、高效且 **header-only** 的 INI 配置解析库，专为 C++ 项目打造。它提供简洁、直观且优雅的 API，支持从文件、`std::istream` 或 `std::string` 解析、修改并写入 INI 配置信息，并具备行级注释保留功能，确保原始注释不丢失，使配置管理更加轻松高效。

### 🚀 特性
- **轻量级 & 无依赖**：仅依赖 C++11 标准库，无需额外依赖项
- **易于集成**：Header-only 设计，开箱即用，足够简单
- **直观 API**：提供清晰友好的接口，简化 INI 配置操作
- **跨平台支持**：支持Linux, Windows, MacOS等系统, 以及主流编译器
- **多种数据源**：支持从文件，`std::string` 或 `std::istream` 解析 INI 数据，并写入其中
- **自动类型转换**：支持多种数据类型，能自动处理类型转换(优雅的api接口)
- **支持注释功能**:  支持ini行注释(`;`或者`#`), 可以为`[section]`和`key=value`添加行注释(不支持行尾注释)
- **自定义类型转换**: 可以自定义类型转换, inifile将根据你写的定义进行自动转换(减少重复)
- **支持大小写不敏感功能**: 提供可选的大小写不敏感功能(针对`section`和`key`)
- **全面测试与内存安全**：已通过 [Catch2](https://github.com/catchorg/Catch2) 单元测试框架验证功能正确性，并使用 [Valgrind](https://valgrind.org/) 确保内存管理无泄漏

适用于对 INI 配置有 **解析、编辑、存储** 需求的 C++ 项目。以下是基础的ini格式:

```ini
; comment
[section]
key = value
```

> 注: 本库内部使用`std::string`类型封装filed值, 可以很好的和 `UTF-8` 编码兼容, 但其他编码具体情况可能会有所不同.

### 📦 使用方式

**方式1: Header-only**

1. 直接将[`inifile.h`](./include/inifile/inifile.h)头文件复制到您的项目文件夹中

2. 然后在源代码文件中直接`#include "inifile.h"`即可使用

**方式2: 使用CMake**

1. 在项目中创建一个`inifile`文件夹(名称随意)

2. 将本项目的[`include`](./include/)文件夹中的所有内容复制到刚才步骤1创建的`inifile`文件夹内

3. 然后在您的主`CMakeLists.txt`中添加以下内容

   ```cmake
   add_subdirectory(inifile) # inifile为步骤1创建的文件夹名称
   ```

4. 在源代码中添加`#include <inifile/inifile.h>`即可使用

### 🛠️ 基础使用案例

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
  bool        b = inif["section"]["key0"];
  double      d = inif["section"]["key1"];
  std::string s = inif["section"]["key2"];

  // Explicit type conversion via as function
  bool        bb = inif["section"]["key0"].as<bool>();
  double      dd = inif["section"]["key1"].as<double>();
  std::string ss = inif["section"]["key2"].as<std::string>();
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

- 给定的section-key是否存在, 当section-key不存在时调用不同的函数会有不同的处理策略;
  - 使用`operator[]`返回**引用**, 若给定section或key不存在则**会插入**空的field值, 并设置field为空字符串. (行为类似`std::map`的`[]`)
  - 使用`get()`函数返回**值**, 若给定的section或key不存在**不会插入**field, 而是返回一个默认的空field值(可以指定默认值).
  - 使用`at()`函数返回**引用**, 若给定的section或key不存在则**抛出异常** : `std::out_of_range`
- 是否可以执行类型自动转换, 以上三个函数返回的是 `ini::field` 包装对象, 若将该对象转为其他类型需注意:
  - 类型转换是否允许, 若类型转换不允许则**抛出异常**: `std::invalid_argument`, (例如将`"abc"`转为`int`)
  - 数值类型转换范围是否溢出, 若超出目标类型的范围则**抛出异常**: `std::out_of_range`, (例如将`INT_MAX`转为`uint8_t`)

```cpp
#include "inifile.h"
int main()
{
  ini::inifile inif;
  try
  {
    /// automatic type conversion
    std::string s0 = inif["section"]["key1"];
    bool isok = inif["section"]["key2"];

    int ii0 = inif.get("section", "key3");
    int ii2 = inif.get("section", "key3", -1); // Specify default values

    std::string ss2 = inif["section"].get("key4");
    std::string ss3 = inif["section"].get("key5", "default"); // Specify default values

    double dd0 = inif.at("section").at("key");
    std::cout << "section-key:" << inif["section"]["key"].as<double>() << std::endl;
  }
  catch (const std::exception &e)
  {
    std::cout << e.what() << std::endl;
  }
}
```

#### 注释功能

本库支持设置`[section]`和`key=value`的行级注释(不支持行尾注释), 注释符号可选`;`和`#`两种; 也能从数据源中保留注释内容. [查看案例](./examples/inifile_comment.cpp)

以下方法是**功能完全等价**的，选择哪一种主要取决于你的风格偏好：

| 简写形式             | 完整形式               | 说明                                       |
| -------------------- | ---------------------- | ------------------------------------------ |
| `f.set_comment(...)` | `f.comment().set(...)` | 设置注释, 覆盖原有的注释（支持单行或多行） |
| `f.add_comment(...)` | `f.comment().add(...)` | 追加注释（支持单行或多行）                 |
| `f.clear_comment()`  | `f.comment().clear()`  | 清除所有注释                               |

```cpp
#include "inifile.h"
int main()
{
  ini::inifile inif;
  // Set value
  inif["section"]["key0"] = true;
  inif["section"]["key1"] = 3.141592;
  inif["section"]["key2"] = "value";

  // Set comments if necessary
  inif["section"].set_comment("This is a section comment.");                     // set section comment, Overwrite Mode
  inif["section"]["key1"].set_comment("This is a key-value pairs comment", '#'); // set key=value pairs comment
	
  // Clear section comment
  inif["section"].clear_comment();
  
  // Add section comment
  inif["section"].add_comment("section comment01");                    // add section comment, Append Mode
  inif["section"].add_comment("section comment02\nsection comment03"); // Multi-line comments are allowed, lines separated by `\n`
  
  // Get the comment object reference
  ini::comment &cmt = inif["section"]["key"].comment();  // get reference to comment
  
  // Read comment content
  const std::vector<std::string> &view = cmt.view();  // view (non-owning)
  
  bool isok = inif.save("config.ini");
}
```

`config.ini`的内容应该为:

```ini
; section comment01
; section comment02
; section comment03
[section]
key0=true
# This is a key-value pairs comment
key1=3.141592
key2=value
```

#### 大小写不敏感功能

本库支持`section`和`key`的大小写不敏感功能, 使用`ini::case_insensitive_inifile`即可, [查看案例](./examples/inifile_case_insensitive.cpp)

```cpp
#include "inifile.h"
int main()
{
  const char *str = R"(
    [Section]
    KEY=Value
    Flag=123
    hello=world
  )";
  
  ini::case_insensitive_inifile inif;  // Create a case-insensitive INI file object
  inif.from_string(str);               // Read INI data from string

  // Test case-insensitive section and key access
  std::cout << "inif.contains(\"Section\") = " << inif.contains("Section") << std::endl;  // true
  std::cout << "inif.contains(\"SECTION\") = " << inif.contains("SECTION") << std::endl;  // true
  std::cout << "inif.contains(\"SeCtIoN\") = " << inif.contains("SeCtIoN") << std::endl;  // true

  std::cout << "inif.at(\"section\").contains(\"key\") = " << inif.at("section").contains("key") << std::endl;
  std::cout << "inif.at(\"section\").contains(\"Key\") = " << inif.at("section").contains("Key") << std::endl;
  std::cout << "inif.at(\"SECTION\").contains(\"KEY\") = " << inif.at("SECTION").contains("KEY") << std::endl;
  std::cout << "inif.at(\"SECTION\").contains(\"flag\") = " << inif.at("SECTION").contains("flag") << std::endl;
  std::cout << "inif.at(\"SECTION\").contains(\"FLAG\") = " << inif.at("SECTION").contains("FLAG") << std::endl;
  return 0;
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
  bool        b = f;
  int         i = f1;
  double      d = f2;
  char        c = f3;
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
  double n = inif["section"]["key"]; // error: Converting true to double is not allowed.
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

#### 自定义类型转换

> Q: 用户自定义类型可以像上面提到的基本数据类型一样自动转换吗? 
>
> A: 也是可以的, 只需要按以下规则自定义类型转换就能让inifile自动处理用户自定义类型.

你可以为用户自定义类型提供自动类型转换的特化模板类, 它能让inifile库根据你定义的规则进行自动转换, 使其可以将自定义类存储在ini字段中, 这样可以大幅减少代码的重复. 以下是自定义规则和模板:

1. 使用`INIFILE_TYPE_CONVERTER`宏**特化**自定义的类型(必须提供默认构造函数);

2. **定义`encode`函数**, 作用是定义如何将自定义类型转为ini存储字符串(存储字符串不能包含换行符);

3. **定义`decode`函数**, 作用是定义如何将ini存储字符串转为自定义类型;

```cpp
/// Specialized type conversion template
template <>
struct INIFILE_TYPE_CONVERTER<CustomClass>  // User-defined type `CustomClass`
{
  void encode(const CustomClass &obj, std::string &value)  // pass by reference
  {
    // Convert the CustomClass object `obj` to ini storage string `value`
  }
  void decode(const std::string &value, CustomClass &obj)
  {
    // Convert the ini storage string `value` to a CustomClass object `obj`
  }
}
```

> 为了方便编写 `encode` 和 `decode` 函数, 本库提供了 `ini::join`, `ini::split()` 和 `ini::trim()` 工具函数 

**案例1**: 下面是将一个用户自定义类`Person`对象转为ini字段案例. [查看案例](./examples/inifile_custom.cpp)

```cpp
/// @brief User-defined classes
struct Person
{
  Person() = default;  // Must have a default constructor
  Person(int id, int age, const std::string &name) : id(id), age(age), name(name) {}

  int id = 0;
  int age = 0;
  std::string name;
};
/// @brief Custom type conversion (Use INIFILE_TYPE_CONVERTER to specialize Person)
template <>
struct INIFILE_TYPE_CONVERTER<Person>
{
  // "Encode" the Person object into a value string
  void encode(const Person &obj, std::string &value)
  {
    const char delimiter = ',';
    // Format: id,age,name; Note: Please do not include line breaks in the value string
    value = std::to_string(obj.id) + delimiter + std::to_string(obj.age) + delimiter + obj.name;
  }

  // "Decode" the value string into a Person object
  void decode(const std::string &value, Person &obj)
  {
    const char delimiter = ',';
    std::vector<std::string> info = ini::split(value, delimiter);
    // Exception handling can be added
    obj.id = std::stoi(info[0]);
    obj.age = std::stoi(info[1]);
    obj.name = info[2];
  }
};

int main()
{
  ini::inifile inif;
  Person p = Person{123456, 18, "abin"};
  inif["section"]["key"] = p;          // set person object
  Person pp = inif["section"]["key"];  // get person object
}
```

**案例2**: 可以嵌套调用`INIFILE_TYPE_CONVERTER<T>`, 实现STL容器自动转换, 能实现以下直接对容器赋值或取值的效果, 具体实现请[点击查看详情](./examples/inifile_custom2.cpp)

```cpp
// Define vectors of different types
std::vector<int>         vec1 = {1, 2, 3, 4, 5};
std::vector<double>      vec2 = {1.1111, 2.2222, 3.3333, 4.4444, 5.5555};
std::vector<std::string> vec3 = {"aaa", "bbb", "ccc", "ddd", "eee"};

// Set different types of vectors in the INI file object
inif["section"]["key1"] = vec1;
inif["section"]["key2"] = vec2;
inif["section"]["key3"] = vec3;

// Get different vectors from INI file object
std::vector<int>         v1 = inif["section"]["key1"];
std::vector<double>      v2 = inif["section"]["key2"];
std::vector<std::string> v3 = inif["section"]["key3"];
```

#### 其他工具函数

提供其他多种工具函数,  判断是否为空`empty()`, 查询总个数`size()`, 查询key的个数`count()`,  是否包含元素`contains()`,  查找元素`find()`, 移除元素`remove()` 和 `erase()`,  清除所有元素`clear()`,  迭代器访问:`begin()`, `end()`, `cbegin()`, `cend()`, 支持范围`for`循环.  具体详情请查看常用 API 说明. 

下面提供一个迭代器访问ini信息:

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

| class名称                     | 描述                                                         |
| ----------------------------- | ------------------------------------------------------------ |
| ini::inifile                  | 对应整个ini数据, 包含了所有的section                         |
| ini::section                  | 对应整个section内容, 里面包含了本section所有的key-value值    |
| ini::case_insensitive_inifile | 对`section`和`key`大小写不敏感, 其他功能和`ini::inifile`一致 |
| ini::case_insensitive_section | 对`key`大小写不敏感, 其他功能和`ini::section`一致            |
| ini::field                    | 对应ini文件中的 value 字段, 支持多种数据类型,  支持自动类型转换 |
| ini::comment                  | ini文件中注释类, 管理section和key-value的注释                |

#### ini::comment类API说明

<details>
  <summary>点击展开</summary>

| 函数名    | 函数签名                                                     | 功能描述           |
| --------- | ------------------------------------------------------------ | ------------------ |
| comment   | `comment(const std::string &str, char symbol = ';')`         | 构造函数           |
| comment   | `comment(const std::vector<std::string> &vec, char symbol = ';')` | 构造函数           |
| comment   | `comment(std::initializer_list<std::string> list, char symbol = ';')` | 构造函数           |
| swap      | `void swap(comment &other) noexcept`                         | 交换函数           |
| empty     | `bool empty() const noexcept`                                | 是否为空           |
| clear     | `void clear() noexcept`                                      | 清空注释           |
| set       | `void set(const std::string &str, char symbol = ';')`        | 设置注释(覆盖模式) |
| add       | `void add(const std::string &str, char symbol = ';')`        | 添加注释(追加模式) |
| view      | `const std::vector<std::string> &view() const`               | 返回注释容器常引用 |
| to_vector | `std::vector<std::string> to_vector() const`                 | 返回注释容器的拷贝 |

</details>

#### ini::field类API说明

<details>
  <summary>点击展开</summary>

以下函数类型转换失败或者值溢出将抛异常

| 函数名        | 函数签名                                                     | 功能描述                          |
| ------------- | ------------------------------------------------------------ | --------------------------------- |
| field         | `field(const T &other)`                                      | 构造field对象, 将T类型转为field值 |
| set           | `void set(const T &value)`                                   | 设置field值, 将T类型转为field值   |
| operator=     | `field &operator=(const T &rhs)`                             | 设置field值, 将T类型转为field值   |
| operator T    | `operator T() const`                                         | 将field类型转为T类型              |
| as            | `T as() const`                                               | 将field类型转为T类型              |
| swap          | `void swap(field &other) noexcept`                           | 交换函数                          |
| set_comment   | `void set_comment(const std::string &str, char symbol = ';')` | 设置key-value的注释, 覆盖模式     |
| add_comment   | `void add_comment(const std::string &str, char symbol = ';')` | 添加key-value的注释, 追加模式     |
| clear_comment | `void clear_comment()`                                       | 清除key-value的注释               |
| comment       | `ini::comment &comment()`                                    | 获取key-value的注释内容(引用)     |

</details>

#### ini::section类API说明

<details>
  <summary>点击展开</summary>

| 函数名        | 函数签名                                                     | 功能描述                                                     |
| ------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| operator[]    | `field &operator[](const std::string &key)`                  | 返回ini::field引用, 不存在则插入空ini::field                 |
| set           | `void set(std::string key, T &&value)`                       | 插入或更新指定key的field                                     |
| contains      | `bool contains(std::string key) const`                       | 判断key是否存在                                              |
| at            | `field &at(std::string key)`                                 | 返回指定key键的元素的字段值的引用。如果元素不存则抛 std::out_of_range异常 |
| swap          | `void swap(basic_section &other) noexcept`                   | 交换函数                                                     |
| get           | `field get(std::string key, field default_value = field{}) const` | 获取key对应的值(副本), 若key不存在则返回default_value默认值  |
| find          | `iterator find(const key_type &key)`                         | 查找指定key值的迭代器, 不存在返回end迭代器                   |
| erase         | `iterator erase(iterator pos)`                               | 删除指定迭代器的key-value键值对                              |
| remove        | `bool remove(std::string key)`                               | 删除指定的key-value键值对, 若不存在则什么都不做              |
| empty         | `bool empty() const noexcept`                                | 判断key-value键值对是否为空, 为空返回true                    |
| clear         | `void clear() noexcept`                                      | 清除所有key - value键值对                                    |
| keys          | `std::vector<key_type> keys() const`                         | 获取所有的keys                                               |
| values        | `std::vector<ini::filed> values() const`                     | 获取所有的values, 类型为ini::filed                           |
| items         | `std::vector<value_type> items() const`                      | 获取所有的key-value键值对                                    |
| size          | `size_type size() const noexcept`                            | 返回有多少key - value键值对                                  |
| count         | `size_type count(const key_type &key) const`                 | 返回有多少指定key的key - value键值对                         |
| begin         | `iterator begin() noexcept`                                  | 返回起始迭代器                                               |
| end           | `iterator end() noexcept`                                    | 返回末尾迭代器                                               |
| set_comment   | `void set_comment(const std::string &str, char symbol = ';')` | 设置section的注释, 覆盖模式, 注释字符串允许换行`\n`          |
| add_comment   | `void add_comment(const std::string &str, char symbol = ';')` | 添加section的注释, 追加模式, 注释字符串允许换行`\n`          |
| clear_comment | `void clear_comment()`                                       | 清除section的注释                                            |
| comment       | `ini::comment &comment()`                                    | 获取section的注释内容(引用)                                  |

</details>

#### ini::inifile类API说明

<details>
  <summary>点击展开</summary>

| 函数名      | 函数签名                                                     | 功能描述                                                     |
| ----------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| operator[]  | `section &operator[](const std::string &section)`            | 返回ini::section引用, 不存在则插入空ini::section             |
| set         | `void set(const std::string &section, const std::string &key, T &&value)` | 设置section key-value                                        |
| contains    | `bool contains(std::string section) const`                   | 判断指定的section是否存在                                    |
| contains    | `bool contains(std::string section, std::string key) const`  | 判断指定section下指定的key是否存在                           |
| at          | `section &at(std::string section)`                           | 返回指定section的引用。如果不存在这样的元素，则会抛出 std::out_of_range 类型的异常 |
| swap        | `void swap(inifile &other) noexcept`                         | 交换函数                                                     |
| get         | `field get(std::string sec, std::string key, field default_value = field{}) const` | 返回指定section的指定key键的字段值, 若section或key不存在则返回默认值default_value |
| find        | `iterator find(const key_type &key)`                         | 查找指定section的迭代器, 不存在返回end迭代器                 |
| erase       | `iterator erase(iterator pos)`                               | 删除指定迭代器的section(包括其所有元素)                      |
| remove      | `bool remove(std::string sec)`                               | 删除指定的section(包括其所有元素)                            |
| empty       | `bool empty() const noexcept`                                | 判断是否没有section, 没有section空返回true                   |
| clear       | `void clear() noexcept`                                      | 清空所有的section                                            |
| size        | `size_type size() const noexcept`                            | 返回有多少section                                            |
| sections    | `std::vector<key_type> sections() const`                     | 获取ini文件的所有section                                     |
| count       | `size_type count(const key_type &key) const`                 | 返回有多少指定section-name的section                          |
| begin       | `iterator begin() noexcept`                                  | 返回起始迭代器                                               |
| end         | `iterator end() noexcept`                                    | 返回末尾迭代器                                               |
| read        | `void read(std::istream &is)`                                | 从istream中读取ini信息                                       |
| write       | `void write(std::ostream &os) const`                         | 向ostream中写入ini信息                                       |
| from_string | `void from_string(const std::string &str)`                   | 从string中读取ini信息                                        |
| to_string   | `std::string to_string() const`                              | 将inifile对象转为对应字符串                                  |
| load        | `bool load(const std::string &filename)`                     | 从ini文件中加载ini信息, 返回是否成功                         |
| save        | `bool save(const std::string &filename)`                     | 将ini信息保存到ini文件, 返回是否成功                         |

</details>

### 💡 贡献指南

欢迎提交 **Issue** 和 **Pull request** 来改进本项目！

### 🙌 致谢

感谢 **[Catch2](https://github.com/catchorg/Catch2)** 提供强大支持，助力本项目的单元测试！

感谢 **[Valgrind](http://valgrind.org/)** 在确保内存安全和防止内存泄漏方面的帮助！

### 📜 许可证

本项目采用[ **MIT** 许可证](./LICENSE)。

版权所有 © 2025–Present Abin。

