/*
 *  Sample: load_and_print
 *  ----------------------
 *  Loads an INI file from disk and prints its content to stdout.
 *
 *  示例：load_and_print
 *  ---------------------
 *  从磁盘加载 INI 文件并将其内容输出到标准输出。
 */

#include <inifile/inifile.h>

int main()
{
  constexpr char path[] = "./config.ini";  // path to ini file

  ini::inifile inif;

  if (inif.load(path))
  {
    std::cout << "Result: Ini file loaded successfully\n";
    std::cout << "--------------------------------------\n";
    std::cout << inif.to_string() << '\n';
    std::cout << "--------------------------------------" << '\n';
  }
  else
  {
    std::cout << "Result: Failed to load ini file" << '\n';
  }
}