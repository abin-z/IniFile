/*
 *  Sample: read_from_stream
 *  ------------------------
 *  Reads INI data from a string stream and prints all sections and key-value pairs.
 *
 *  示例：read_from_stream
 *  ------------------------
 *  从字符串流读取 INI 数据，并打印所有 section 及其键值对内容。
 */

#include <inifile/inifile.h>

int main()
{
  const char *str = R"(
    [Section]
    key=value
    hello=world
    num=123
    [Nothing1]
    [Test]
    status=pass
    [Nothing2]
  )";

  // Read ini data from stream
  std::istringstream is(str);
  ini::inifile inif;
  inif.read(is);

  //// Read ini data from string
  // ini::inifile inif;
  // inif.from_string(str);

  std::cout << "~~~~~~~~~~~~~~~~~read ini contents~~~~~~~~~~~~~~~~~\n";
  std::cout << "inifile has " << inif.size() << " sections\n";
  for (const auto &sec_pair : inif)
  {
    const std::string &section_name = sec_pair.first;
    const ini::section &section = sec_pair.second;
    std::cout << "  section '" << section_name << "' has " << section.size() << " key-values.\n";

    for (const auto &kv : section)
    {
      const std::string &key = kv.first;
      const auto &value = kv.second;
      std::cout << "    kv: '" << key << "' = '" << value << "'\n";
    }
  }
  std::cout << "~~~~~~~~~~~~~~~~~read ini contents~~~~~~~~~~~~~~~~~\n";
}