#include <iostream>

#include <inifile/inifile.h>

int main()
{
  std::cout << "hello world!" << std::endl;
  ini::field field;
  int i = 100;
  double d = 3.14;
  long double ld = 999999999.999;
  std::string s = "你好";
  bool b = true;
  const char c = 'm';
  const char arr[5] = {'g', 'o', 'o', 'd', '\0'};

  wchar_t wc = 'l';

  field = i;
  field = d;
  field = ld;
  field = s;
  field = b;
  field = c;
  field = i;
  // field = wc;
  field.set(123);
  field = arr;

  ini::field f2 = 100;

  std::string ss = field;
  double dd = field;
  int ii = field;
  const char * cpp = field;
  // wchar_t wcc = field;

  std::cout << "ss = " << ss << ", dd = " << dd << ", ii = " << ii << ", cpp = " << cpp 
  << ", as<double> = " << field.as<double>() << std::endl; 

  ini::inifile file;
  file["sec"]["strkey"] = 100;
  int val = file["sec"]["strkey"];
  // file["sec"].set("strkey", 2000);
  double ds = file["sec"].get("strkey");
  file["sec"].set("price", 99.9);
  file["sec"].set({{"hello", 0}, {"ggg", 20}, {"mmm", 999}});
  file["section"].set({
    {"key1", "value1"},
    {"key2", "value2"},
    {"key3", 999},
    {"key4", 3.14159},
    {"key5", true}
  });


  auto &section01 = file["section"];
  section01.remove("");
  for(const auto &pair : section01)
  {
    std::cout << pair.first << "=" << pair.second << std::endl;
  }

  float nom = section01.get("no", 98.9);
  std::cout << "nom = " << nom << std::endl;




  std::cout << "value = " << file["sec"]["strkey"].as<int>() << ", val = " << ds << std::endl;
}