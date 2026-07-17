/*
 *  Sample: case_insensitive_inifile
 *  --------------------------------
 *  Demonstrates usage of `ini::case_insensitive_inifile`:
 *    - Case-insensitive access to sections and keys
 *    - Value retrieval and string conversion
 *    - Comparison of modified vs original content
 *
 *  示例：case_insensitive_inifile
 *  ------------------------------
 *  演示 `ini::case_insensitive_inifile` 的用法：
 *    - Section 和 key 的大小写无关访问
 *    - 键值读取与字符串转换
 *    - 修改后与原始内容的对比
 */

#include <inifile/inifile.h>

int main()
{
  const char *str = R"(
    [Section]
    KEY=Value
    Flag=123
    hello=world
  )";

  ini::case_insensitive_inifile inif;  // Create a case-insensitive INI file object
  inif.from_string(str);               // Read INI data from string
  auto copy_inif = inif;               // Copy the ini file object

  // Test case-insensitive section and key access
  std::cout << R"(inif.contains("Section") = )" << inif.contains("Section") << '\n';
  std::cout << R"(inif.contains("SECTION") = )" << inif.contains("SECTION") << '\n';
  std::cout << R"(inif.contains("SeCtIoN") = )" << inif.contains("SeCtIoN") << '\n';

  std::cout << R"(inif.at("section").contains("key") = )" << inif.at("section").contains("key") << '\n';
  std::cout << R"(inif.at("section").contains("Key") = )" << inif.at("section").contains("Key") << '\n';
  std::cout << R"(inif.at("SECTION").contains("KEY") = )" << inif.at("SECTION").contains("KEY") << '\n';
  std::cout << R"(inif.at("SECTION").contains("flag") = )" << inif.at("SECTION").contains("flag") << '\n';
  std::cout << R"(inif.at("SECTION").contains("FLAG") = )" << inif.at("SECTION").contains("FLAG") << '\n';

  std::cout << "section-key: " << inif["section"]["key"].as<std::string>() << '\n';
  std::cout << "SECTION-KEY: " << inif["SECTION"]["KEY"].as<std::string>() << '\n';
  std::cout << "Section-Key: " << inif["Section"]["Key"].as<std::string>() << '\n';

  std::cout << "section-hello: " << inif["section"]["hello"].as<std::string>() << '\n';
  std::cout << "SECTION-HELLO: " << inif["SECTION"]["HELLO"].as<std::string>() << '\n';
  std::cout << "Section-Hello: " << inif["Section"]["Hello"].as<std::string>() << '\n';

  std::cout << "section-Flag: " << inif["section"]["Flag"].as<int>() << '\n';
  std::cout << "SECTION-FLAG: " << inif["SECTION"]["FLAG"].as<int>() << '\n';
  std::cout << "Section-flag: " << inif["Section"]["flag"].as<int>() << '\n';

  std::cout << "~~~~~~~~~~~~~~~~~ After the visit ini contents ~~~~~~~~~~~~~~~~~" << '\n';
  std::cout << inif.to_string() << '\n';

  std::cout << "~~~~~~~~~~~~~~~~~ original ini contents ~~~~~~~~~~~~~~~~~" << '\n';
  std::cout << copy_inif.to_string() << '\n';
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << '\n';
  return 0;
}