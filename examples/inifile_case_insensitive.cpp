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
  std::cout << "inif.contains(\"Section\") = " << inif.contains("Section") << std::endl;
  std::cout << "inif.contains(\"SECTION\") = " << inif.contains("SECTION") << std::endl;
  std::cout << "inif.contains(\"SeCtIoN\") = " << inif.contains("SeCtIoN") << std::endl;

  std::cout << "inif.at(\"section\").contains(\"key\") = " << inif.at("section").contains("key") << std::endl;
  std::cout << "inif.at(\"section\").contains(\"Key\") = " << inif.at("section").contains("Key") << std::endl;
  std::cout << "inif.at(\"SECTION\").contains(\"KEY\") = " << inif.at("SECTION").contains("KEY") << std::endl;
  std::cout << "inif.at(\"SECTION\").contains(\"flag\") = " << inif.at("SECTION").contains("flag") << std::endl;
  std::cout << "inif.at(\"SECTION\").contains(\"FLAG\") = " << inif.at("SECTION").contains("FLAG") << std::endl;

  std::cout << "section-key: " << inif["section"]["key"].as<std::string>() << std::endl;
  std::cout << "SECTION-KEY: " << inif["SECTION"]["KEY"].as<std::string>() << std::endl;
  std::cout << "Section-Key: " << inif["Section"]["Key"].as<std::string>() << std::endl;

  std::cout << "section-hello: " << inif["section"]["hello"].as<std::string>() << std::endl;
  std::cout << "SECTION-HELLO: " << inif["SECTION"]["HELLO"].as<std::string>() << std::endl;
  std::cout << "Section-Hello: " << inif["Section"]["Hello"].as<std::string>() << std::endl;

  std::cout << "section-Flag: " << inif["section"]["Flag"].as<int>() << std::endl;
  std::cout << "SECTION-FLAG: " << inif["SECTION"]["FLAG"].as<int>() << std::endl;
  std::cout << "Section-flag: " << inif["Section"]["flag"].as<int>() << std::endl;

  std::cout << "~~~~~~~~~~~~~~~~~ After the visit ini contents ~~~~~~~~~~~~~~~~~" << std::endl;
  std::cout << inif.to_string() << std::endl;

  std::cout << "~~~~~~~~~~~~~~~~~ original ini contents ~~~~~~~~~~~~~~~~~" << std::endl;
  std::cout << copy_inif.to_string() << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  return 0;
}