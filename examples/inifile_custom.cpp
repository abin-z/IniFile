#include <inifile/inifile.h>

/// @brief 自定义类型Person
struct Person
{
  int id = 0;
  int age = 0;
  std::string name;

  std::string to_string() const
  {
    std::ostringstream oss;
    oss << "Person{id=" << id << ", age=" << age << ", name=\"" << name << "\"}";
    return oss.str();
  }
};

/// @brief 自定义类型转换(转换Person)
template <>
struct INIFILE_TYPE_CONVERTER<Person>
{
  // 将Person对象 "编码"为 value字符串
  void encode(const Person &obj, std::string &value)
  {
    const char delimiter = ',';
    value = std::to_string(obj.id) + delimiter + std::to_string(obj.age) + delimiter + obj.name;
  }

  // 将value字符串 "解码"为 Person对象
  void decode(const std::string &value, Person &obj)
  {
    const char delimiter = ',';
    std::vector<std::string> info = ini::split(value, delimiter);
    if (info.size() >= 3)
    {
      obj.id = std::stoi(info[0]);
      obj.age = std::stoi(info[1]);
      obj.name = info[2];
    }
    else
    {
      // Exception or default or other, you can customize it
    }
  }
};

int main()
{
  ini::inifile inif;
  Person p = Person{123, 24, "abin"};

  // set person object
  inif["section"]["key"] = p;

  // get person object
  Person pp = inif["section"]["key"];

  std::cout << "Person pp: " << pp.to_string() << std::endl;
}