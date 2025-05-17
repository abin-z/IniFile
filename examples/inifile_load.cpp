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
    std::cout << "Result: Ini file loaded successfully" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << inif.to_string() << std::endl;
    std::cout << "--------------------------------------" << std::endl;
  }
  else
  {
    std::cout << "Result: Failed to load ini file" << std::endl;
  }
}