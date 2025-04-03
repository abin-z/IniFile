## 🌟 Lightweight INI File Parsing Library

[![iniparser](https://img.shields.io/badge/INI_Parser-8A2BE2)](https://github.com/abin-z/IniFile) ![headeronly](https://img.shields.io/badge/Header_Only-green) ![moderncpp](https://img.shields.io/badge/Modern_C%2B%2B-218c73) [![licenseMIT](https://img.shields.io/badge/License-MIT-green)](https://github.com/abin-z/IniFile/blob/main/LICENSE) [![version](https://img.shields.io/badge/version-0.9.3-green)](https://github.com/abin-z/IniFile/releases)

🌍 Languages/语言:  [English](README.md)  |  [简体中文](README.zh-CN.md)

### 📌 Project Overview

This is a lightweight, cross-platform, efficient and **header-only** INI configuration parsing library designed for C++ projects. It provides a concise, intuitive and elegant API, supports parsing, modifying and writing INI configuration information from files, `std::istream` or `std::string`, and has line-level comment retention function to ensure that the original comments are not lost, making configuration management easier and more efficient.

### 🚀 Features

- **Lightweight & Dependency-Free**: Only relies on the C++11 standard library, no additional dependencies required.
- **Easy Integration**: Header-only design, out of the box, simple enough.
- **Intuitive API**: Provides a clear and user-friendly interface to simplify INI file operations.
- **Comprehensive Support**: Allows reading, modifying, and writing INI data to files.
- **Multiple Data Sources**: Supports parsing INI data from files, `std::string` or `std::istream`, and writing to them.
- **Automatic Type Conversion**: Supports multiple data types and can automatically handle type conversion (elegant API interface).
- **Support comment function**: Support ini line comments (`;` or `#`), you can add line comments for `[section]` and `key=value` (does not support end-of-line comments).
- **Custom type conversion**: You can customize type conversion, inifile will automatically convert according to the definition you wrote (reduce duplication)

Ideal for C++ projects that require **parsing, editing, and storing** INI configuration files. The following is the basic ini format:

```ini
; comment
[section]
key = value
```

> Note: The library uses `std::string` to encapsulate filed values internally, which is very compatible with `UTF-8` encoding, but other encoding specifics may be different.

### 📦 Usage

**Header-Only Approach**

1. Copy the [`inifile.h`](./include/inifile/inifile.h) header file to your project folder.
2. Include it in your source code using `#include "inifile.h"`.

**CMake Approach**

1. Create an `inifile` folder in your project (name can be customized).

2. Copy all contents from the [`include`](./include/) folder of this project into the `inifile` folder.

3. Add the following line to your main `CMakeLists.txt` file:

   ```cmake
   add_subdirectory(inifile) # "inifile" is the folder name created in step 1
   ```

4. Include it in your source code using `#include <inifile/inifile.h>`.

### 🛠️ Usage Examples

Below are some simple usage examples. For more details, refer to the[`./examples/`](./examples/) folder.

#### Creating and Saving an INI File

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

#### Reading an INI File

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

#### Reading/Writing INI Data from `std::istream` or `std::ostream`

```cpp
#include "inifile.h"
int main()
{
  // Create istream object "is" ...
  ini::inifile inif;
  inif.read(is);
}
```

```cpp
#include "inifile.h"
int main()
{
  // Create ostream object "os" ...
  ini::inifile inif;
  inif.write(os);
}
```

#### Reading/Writing INI Data from `std::string`

```cpp
#include "inifile.h"
int main()
{
  // Create string object "s" ...
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

#### Set value
Explanation: If the section or key does not exist, The `operator[]` and `set` functions will **directly insert** the section key, and if the section key exists, **update** the field value.

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

#### Get value

Explanation: There are two things to keep in mind when getting values.

- It is important to know whether a given section key exists or not, and different calling functions will have different strategies for handling this;
  - Use `operator[]` to return **reference**. If the given section or key does not exist, will **insert** an empty field value and set the field to an empty string (Behavior similar to `std::map`'s`[]`).
  - Use the ` get() ` function to return the  **value**. If the given section or key does not exist,  field will **not** be inserted, but a default empty field value (default value can be specified) will be returned.
  - Use the ` at() ` function to return a **reference**, and if the given section or key does not exist, **throw an exception**: `std::out_of_range`
- Whether you can perform type automatic conversion, the above three functions return the `ini::field` wrapper object, if the object to other types should be noted:
  - Whether the type conversion is allowed, if the type conversion is not allowed, an **exception** is thrown: `std::invalid_argument`, (e.g. converting `"abc"` to `int`).
  - Whether the range of numeric type conversion overflows, throwing an **exception** if it is outside the range of the target type: `std::out_of_range`, (e.g. converting `INT_MAX` to `uint8_t`).

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

#### Comment function

This library supports setting line-level comments for `[section]` and `key=value` (end-of-line comments are not supported), and the comment symbols can be `;` and `#`; it can also retain the comment content from the data source.

```cpp
#include "inifile.h"
int main()
{
  ini::inifile inif;
  // Set value
  inif["section"]["key0"] = true;
  inif["section"]["key1"] = 3.141592;
  inif["section"]["key2"] = "value";

  // Add comments if necessary
  inif["section"].set_comment("This is a section comment.");                     // set section comment, Overwrite Mode
  inif["section"]["key1"].set_comment("This is a key-value pairs comment", '#'); // set key=value pairs comment

  inif["section"].clear_comment();                                     // clear section comments
  inif["section"].add_comment("section comment01");                    // add section comment, Append Mode
  inif["section"].add_comment("section comment02\nsection comment03"); // Multi-line comments are allowed, lines separated by `\n`
  
  bool isok = inif.save("config.ini");
}
```

The contents of `config.ini` should be:

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

#### About automatic type conversion

Automatic type conversion works on `ini::field` objects, allowing `ini::field` <=> `other type` to be converted to and from each other; but be careful:  **An exception will be thrown if the conversion fails**.

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

Supported types for automatic conversions:

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

#### Custom type conversion

> Q: Can user-defined types be automatically converted like the basic data types mentioned above?
>
> A: Yes, you can. You only need to follow the following rules to customize the type conversion so that inifile can automatically handle user-defined types.

You can provide a special template class for automatic type conversion for user-defined types, which allows the inifile library to automatically convert according to the rules you define, so that it can store custom classes in ini fields, which can greatly reduce code duplication. The following are custom rules and templates:

1. Use the `INIFILE_TYPE_CONVERTER` macro to **specialize** the custom type (Must have a default constructor);
2. **Define `encode` function**, which is used to define how to convert custom types into ini storage strings (the storage string cannot contain newlines);
3. **Define `decode` function**, which is used to define how to convert the ini storage string into a custom type;

```cpp
/// Specialized type conversion template
template <>
struct INIFILE_TYPE_CONVERTER<CustomClass>  // User-defined type `CustomClass`
{
  void encode(const CustomClass &obj, std::string &value)
  {
    // Convert the CustomClass object `obj` to ini storage string `value`
  }
  void decode(const std::string &value, CustomClass &obj)
  {
    // Convert the ini storage string `value` to a CustomClass object `obj`
  }
}
```

> In order to facilitate the writing of the `decode` function in step 3, this library provides the `ini::split()` and `ini::trim()` tool functions

**Example 1**: The following is an example of converting a user-defined class `Person` object to an ini field.   [Click to view details](./examples/inifile_custom.cpp)

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

**Example 2**: You can nest calls to `INIFILE_TYPE_CONVERTER<T>` to achieve automatic conversion of STL containers, which can achieve the following effects of directly assigning or getting values from containers. For the specific implementation, please [Click to view details](./examples/inifile_custom2.cpp)

```cpp
// Define vectors of different types
std::vector<int> vec1 = {1, 2, 3, 4, 5};
std::vector<double> vec2 = {1.1111, 2.2222, 3.3333, 4.4444, 5.5555};
std::vector<std::string> vec3 = {"aaa", "bbb", "ccc", "ddd", "eee"};

// Set different types of vectors in the INI file object
inif["section"]["key1"] = vec1;
inif["section"]["key2"] = vec2;
inif["section"]["key3"] = vec3;

// Get different vectors from INI file object
std::vector<int> v1 = inif["section"]["key1"];
std::vector<double> v2 = inif["section"]["key2"];
std::vector<std::string> v3 = inif["section"]["key3"];
```

#### Other utility functions

Provides a variety of other utility functions, including checking whether it is empty `empty()`, querying the total number `size()`, querying the number of keys `count()`, checking whether it contains elements `contains()`, finding elements `find()`, removing elements `remove()` and `erase()`, clearing all elements `clear()`, iterator access: `begin()`, `end()`, `cbegin()`, `cend()`, and supporting range base `for` loops. For details, please refer to the Common API Description.

Here is an iterator to access ini information:

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

### 📄 Common  API  Descriptions

#### class Description

| class name   | description                                                  |
| ------------ | ------------------------------------------------------------ |
| ini::inifile | Corresponds to the entire ini data, including all sections   |
| ini::section | corresponds to the content of the entire section, which contains all the key-value values of the section. |
| ini::field   | corresponds to the value field in the ini data, supports multiple data types, supports automatic type conversion |

#### ini::field API Description

The following functions will throw an exception if the type conversion fails or the value overflows:

| function name | function signature                                           | function description                                         |
| ------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| field         | `field(const T &other)`                                      | Constructs a field object, converting a T type to a field value. |
| set           | `void set(const T &value)`                                   | Set field value, convert T type to field value.              |
| operator=     | `field &operator=(const T &rhs)`                             | Set field value, convert T type to field value.              |
| operator T    | `operator T() const`                                         | Converting field types to T type                             |
| as            | `T as() const`                                               | Converting field types to T type                             |
| set_comment   | `void set_comment(const std::string &str, char symbol = ';')` | Set the key-value comment, overwrite mode                    |
| add_comment   | `void add_comment(const std::string &str, char symbol = ';')` | Add key-value comments, append mode                          |
| clear_comment | `void clear_comment()`                                       | Clear the comments of key-value                              |

#### ini::section API Description

| function name | function signature                                           | function description                                         |
| ------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| operator[]    | `field &operator[](const std::string &key)`                  | Return an ini::field reference, or insert an empty ini::field if it doesn't exist. |
| set           | `void set(std::string key, T &&value)`                       | Inserts or updates a field with the specified key            |
| contains      | `bool contains(std::string key) const`                       | Determine if the key exists                                  |
| at            | `field &at(std::string key)`                                 | Returns a reference to the field value of the element with the specified key. Throws std::out_of_range exception if element does not exist |
| get           | `field get(std::string key, field default_value = field{}) const` | Get the value (copy) of the key, if the key does not exist, then return the default value of default_value. |
| find          | `iterator find(const key_type &key)`                         | Find the iterator of the specified key value, if it does not exist, return the end iterator |
| erase         | `iterator erase(iterator pos)`                               | Delete the key-value pair of the specified iterator          |
| remove        | `bool remove(std::string key)`                               | Removes the specified key-value key pair, or does nothing if it does not exist. |
| empty         | `bool empty() const noexcept`                                | Determine whether the key-value pair is empty, and return true if it is empty |
| clear         | `void clear() noexcept`                                      | Clear all key-value pairs                                    |
| size          | `size_type size() const noexcept`                            | Returns how many key-value pairs there are.                  |
| count         | `size_type count(const key_type &key) const`                 | Returns the number of key-value pairs for the specified key. |
| begin         | `iterator begin() noexcept`                                  | Returns the begin iterator.                                  |
| end           | `iterator end() noexcept`                                    | Returns the end iterator.                                    |
| set_comment   | `void set_comment(const std::string &str, char symbol = ';')` | Set section comment, overwrite mode, comment string allows line breaks `\n` |
| add_comment   | `void add_comment(const std::string &str, char symbol = ';')` | Add section comments, append mode, comment string allows line breaks `\n` |
| clear_comment | `void clear_comment()`                                       | Clear section comments                                       |

#### ini::inifile API Description

| function name | function signature                                           | function description                                         |
| ------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| operator[]    | `section &operator[](const std::string &section)`            | Returns a reference to ini::section, or inserts an empty ini::section if it does not exist. |
| set           | `void set(const std::string &section, const std::string &key, T &&value)` | Set section key-value                                        |
| contains      | `bool contains(std::string section) const`                   | Determines if the specified section exists.                  |
| contains      | `bool contains(std::string section, std::string key) const`  | Determines if the specified key exists in the specified section. |
| at            | `section &at(std::string section)`                           | Returns a reference to the specified section. If no such element exists, an exception of type std::out_of_range is thrown |
| get           | `field get(std::string sec, std::string key, field default_value = field{}) const` | Returns the field value of the specified key for the specified section, or the default value default_value if section or key does not exist |
| find          | `iterator find(const key_type &key)`                         | Find the iterator of the specified section, if it does not exist, return the end iterator |
| erase         | `iterator erase(iterator pos)`                               | Deletes the section of the specified iterator (including all its elements) |
| remove        | `bool remove(std::string sec)`                               | Removes the specified section (including all its elements).  |
| empty         | `bool empty() const noexcept`                                | Determines whether there is no section, returns true if there is no section |
| clear         | `void clear() noexcept`                                      | Clear all sections                                           |
| size          | `size_type size() const noexcept`                            | Returns how many sections                                    |
| count         | `size_type count(const key_type &key) const`                 | Returns how many sections have the specified section-name    |
| begin         | `iterator begin() noexcept`                                  | Returns the begin iterator.                                  |
| end           | `iterator end() noexcept`                                    | Returns the end iterator.                                    |
| read          | `void read(std::istream &is)`                                | Read ini information from istream                            |
| write         | `void write(std::ostream &os) const`                         | Writes ini information to ostream                            |
| from_string   | `void from_string(const std::string &str)`                   | Read ini information from string                             |
| to_string     | `std::string to_string() const`                              | Converts the inifile object to the corresponding string      |
| load          | `bool load(const std::string &filename)`                     | Load ini information from ini file, return whether it was successful or not |
| save          | `bool save(const std::string &filename)`                     | Save ini information to an ini file, return whether it was successful or not |

### 💡 Contribution Guidelines

We welcome contributions! Feel free to submit **Issues** and **Pull Requests** to improve this project.

### 🙌 Acknowledgements

Thank you **[Catch2](https://github.com/catchorg/Catch2)** for providing strong support and assisting in the unit testing of this project!

### 📜 License

This project is licensed under the [**MIT** License](./LICENSE). Copyright © 2025–Present Abin.

