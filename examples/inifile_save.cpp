#include <inifile/inifile.h>

int main()
{
  constexpr char path[] = "./config.ini"; // path to ini file

  ini::inifile inif;

  inif["section"]["string"] = "value";
  inif["section"]["int"] = 123;
  inif["section"]["float"] = 3.14f;
  inif["section"]["double"] = 3.141592;
  inif["section"]["char"] = 'c';
  inif["section"]["bool"] = true;

  inif["section1"]["string"].set("hello");
  inif["section1"]["int"].set(1);
  inif["section1"]["double"].set(3.14159);
  inif["section1"]["bool"].set(false);

  inif.set("section2", "int", 99);
  inif.set("section2", "bool", false);
  inif.set("section2", "double", 1.67);
  inif.set("section2", "string", "abcdef");

  inif["section3"].set("int", 100);
  inif["section3"].set("bool", true);
  inif["section3"].set("float", 0.99);
  inif["section3"].set("string", std::string("inifile"));

  inif["section4"].set({{"bool", true},
                        {"int", 123},
                        {"double", 999.888},
                        {"string", "ABC"},
                        {"char", 'm'}});

  bool isok = inif.save(path);
  if (isok)
  {
    std::cout << "Result: Saved ini file successfully" << std::endl;
  }
  else
  {
    std::cout << "Result: Failed to save ini file" << std::endl;
  }
}