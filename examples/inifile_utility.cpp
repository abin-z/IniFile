#include <inifile/inifile.h>

#include <iostream>
#include <list>
#include <string>
#include <vector>

int main()
{
  // --- trim ---
  std::string s = "  \tHello World\n ";
  std::cout << "[trim] \"" << ini::trim(s) << "\"\n";

  // --- split by char ---
  std::string csv = "a,b,,c,";
  std::vector<std::string> parts = ini::split(csv, ',');
  std::vector<std::string> parts_no_empty = ini::split(csv, ',', true);

  std::cout << "[split ,] all: " << ini::join(parts, '|') << "\n";
  std::cout << "[split ,] skip_empty: " << ini::join(parts_no_empty, '|') << "\n";

  // --- split by string ---
  std::string path = "C:\\Users\\Admin\\file.ini";
  std::vector<std::string> path_parts = ini::split(path, "\\");
  std::cout << "[split \\] " << ini::join(path_parts, " > ") << "\n";

  // --- join with vector ---
  std::vector<std::string> fruits = {"apple", "banana", "cherry"};
  std::cout << "[join vector] " << ini::join(fruits, ", ") << "\n";

  // --- join with list ---
  std::list<int> numbers = {1, 2, 3};
  std::cout << "[join list] " << ini::join(numbers, " + ") << "\n";

  return 0;
}
