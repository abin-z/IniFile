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
  std::cout << "value = " <<  file["sec"]["strkey"].as<int>() << ", val = " << ds << std::endl;




}