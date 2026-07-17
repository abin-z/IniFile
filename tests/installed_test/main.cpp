#include <inifile/inifile.h>

#include <string>

int main()
{
  ini::inifile ini;
  ini.set("section", "key", "value");
  return ini["section"]["key"].as<std::string>() == "value" ? 0 : 1;
}