##  🛠️ 轻量级ini文件解析库，支持解析、修改、保存ini文件

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

### 🔧 使用示例

下面提供简单的使用案例, 更多详细的案例请查看`./examples/`文件夹下的案例

**创建并保存ini文件**

```cpp
#include "inifile.h"

int main()
{
  constexpr char path[] = "path/to/ini/file";
  ini::inifile inif;
  inif["section"]["key0"] = true;
  inif["section"]["key1"] = 3.14159;
  inif["section"]["key2"] = "value";
  // 调用save方法保存ini文件, 返回是否保存成功
  bool isok = inif.save(path);
}
```

**读取ini文件**

```cpp
#include "inifile.h"

int main()
{
  constexpr char path[] = "path/to/ini/file";
  ini::inifile inif;
  // 调用load方法加载ini文件, 返回是否加载成功
  bool isok = inif.load(path);
  bool b = inif["section"]["key0"];
  double d = inif["section"]["key1"];
  std::string b = inif["section"]["key2"];
}
```







### 📄 API 说明

**class 类型说明**

| 类型名称     | 描述                                                      |
| ------------ | --------------------------------------------------------- |
| ini::inifile | 对应整个ini数据, 包含了所有的section                      |
| ini::section | 对应整个section内容, 里面包含了本section所有的key-value值 |
| ini::field   | 对应ini文件中的 value 字段, 支持多种数据类型              |



### 💡 贡献指南

欢迎提交 **Issue** 和 **Pull request** 来改进本项目！

### 📜 许可证

本项目采用 **MIT** 许可证。
