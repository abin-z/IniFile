#include <inifile/inifile.h>

int main()
{
  ini::inifile inif;

  inif["section1"]["string"] = "value";
  inif["section1"]["int"] = 123;
  inif["section1"]["float"] = 3.14f;
  inif["section1"]["double"] = 3.141592;
  inif["section1"]["char"] = 'c';
  inif["section1"]["bool"] = true;
  inif["section1"].set_comment("Comments on section1");

  inif.set("section2", "int", 99);
  inif.set("section2", "bool", false);
  inif.set("section2", "double", 1.67);
  inif.set("section2", "string", "abcdef");
  inif.at("section2").add_comment("Comments on section2");

  inif["section3"].set("int", 100);
  inif["section3"].set("bool", true);
  inif["section3"].set("float", 0.99);
  inif["section3"].set("string", std::string("inifile"));

  inif["section4"].set({{"bool", true},
                        {"int", 123},
                        {"double", 999.888},
                        {"string", "ABC"},
                        {"char", 'm'}});

  // Write ini data to stream
  std::ostringstream ss;
  inif.write(ss);
  std::string content = ss.str();

  //// ini data to string
  // std::string content = inif.to_string();

  std::cout << "~~~~~~~~~~~~~~~~~write inifile contents~~~~~~~~~~~~~~~~~" << std::endl;
  std::cout << content << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~write inifile contents~~~~~~~~~~~~~~~~~" << std::endl;
}