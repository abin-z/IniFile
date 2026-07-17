#include <inifile/inifile.h>

#include <iostream>

int main()
{
  ini::inifile config;
  config["section1"]["key1"] = "value1";
  config["section1"]["key2"] = "value2";
  config["section1"]["key3"] = "value3";
  config["section1"]["key4"] = "value4";

  std::cout << "Hello, World!\n";
  std::cout << "INI file content:\n" << config.to_string();
  return 0;
}