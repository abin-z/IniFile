## 🛠️ Lightweight INI File Parsing Library - Supports Parsing, Modifying, and Saving INI Files

### 📌 Project Overview

This is a lightweight, efficient, and header-only INI configuration file parsing library designed for C++ projects. It provides a simple and intuitive API that supports fast parsing, modification, and writing of INI files, making configuration management easier.

### 🚀 Features

- **Lightweight & Dependency-Free**: Only relies on the C++11 standard library, no additional dependencies required.
- **Easy Integration**: Header-only design, ready to use out of the box.
- **Intuitive API**: Provides a clear and user-friendly interface to simplify INI file operations.
- **Comprehensive Support**: Allows reading, modifying, and writing INI data to files.
- **Multiple Data Sources**: Supports parsing INI data from `std::string` or `std::istream` and writing back to them.
- **Automatic Type Conversion**: Supports multiple data types with seamless type conversion.

Ideal for C++ projects that require **parsing, editing, and storing** INI configuration files.

### 📦 Usage

**Header-Only Approach**

1. Copy the [`inifile.h`](https://chatgpt.com/c/include/inifile/inifile.h) header file to your project folder.
2. Include it in your source code using `#include "inifile.h"`.

**CMake Approach**

1. Create an `inifile` folder in your project (name can be customized).

2. Copy all contents from the [`include`](https://chatgpt.com/c/include/) folder of this project into the `inifile` folder.

3. Add the following line to your main `CMakeLists.txt` file:

   ```cmake
   add_subdirectory(inifile) # "inifile" is the folder name created in step 1
   ```

4. Include it in your source code using `#include <inifile/inifile.h>`.

### 🔧 Usage Examples

Below are some simple usage examples. For more details, refer to the [`./examples/`](https://chatgpt.com/c/examples/) folder.

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
#include "inifile.h"
int main()
{
  ini::inifile inif;
  inif["section"]["key"] = "value";
  std::string s = inif.to_string();
}
```

### 💡 Contribution Guidelines

We welcome contributions! Feel free to submit **Issues** and **Pull Requests** to improve this project.

### 📜 License

This project is licensed under the [**MIT** License](https://chatgpt.com/c/LICENSE).