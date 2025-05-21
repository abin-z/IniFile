
/*
 *  Sample: custom_type_conversion_demo
 *  -----------------------------------
 *  Demonstrates how to define and use custom type conversion
 *  for user-defined classes with ini::inifile via
 *  INIFILE_TYPE_CONVERTER specialization.
 *
 *  示例：自定义类型转换演示
 *  ---------------------
 *  演示如何为用户自定义类通过 INIFILE_TYPE_CONVERTER
 *  模板特化，实现与 ini::inifile 的读写转换支持。
 */

#include <inifile/inifile.h>

/// @brief User-defined classes
struct Person
{
  Person() = default;  // Must have a default constructor
  Person(int id, int age, const std::string &name) : id(id), age(age), name(name) {}

  int id = 0;
  int age = 0;
  std::string name;
};

void print_person(const Person &p)
{
  std::cout << "Person{id=" << p.id << ", age=" << p.age << ", name=\"" << p.name << "\"}" << std::endl;
}

/// @brief Custom type conversion (Use INIFILE_TYPE_CONVERTER to specialize Person)
template <>
struct INIFILE_TYPE_CONVERTER<Person>
{
  // "Encode" the Person object into a value string
  static void encode(const Person &obj, std::string &value)
  {
    const char delimiter = ',';
    // Format: id,age,name; Note: Please do not include line breaks in the value string
    value = std::to_string(obj.id) + delimiter + std::to_string(obj.age) + delimiter + obj.name;
  }

  // "Decode" the value string into a Person object
  static void decode(const std::string &value, Person &obj)
  {
    const char delimiter = ',';
    std::vector<std::string> info = ini::split(value, delimiter);
    if (info.size() >= 3)
    {
      obj.id = std::stoi(info[0]);  // Exception handling can be added
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
  Person p = Person{123456, 24, "abin"};

  inif["section"]["key"] = p;          // set person object
  Person pp = inif["section"]["key"];  // get person object

  print_person(pp);
}