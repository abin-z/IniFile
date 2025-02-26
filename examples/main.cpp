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
  char c = 'm';

  wchar_t wc = 'l';

  field = i;
  field = d;
  field = ld;
  field = s;
  field = b;
  field = c;
  field = i;
  // field = wc;

  std::string ss = field;
  double dd = field;
  int ii = field;
  // wchar_t wcc = field;

  std::cout << "ss = " << ss << ", dd = " << dd << ", ii = " << ii << std::endl; 






}