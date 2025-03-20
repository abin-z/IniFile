#include <inifile/inifile.h>

constexpr char path[] = "basic.ini";
const char *str = R"(
  [section]
  key=value
)";

void save_func()
{
  ini::inifile inif;
  inif["section"]["key0"] = true;
  inif["section"]["key1"] = 3.14159;
  inif["section"]["key2"] = "value";
  // Call the save method to save the ini file and return whether the save was successful
  bool isok = inif.save(path);
}

void load_func()
{
  ini::inifile inif;
  // Call the load method to load the ini file and return whether the loading was successful
  bool isok = inif.load(path);
  bool b = inif["section"]["key0"];
  double d = inif["section"]["key1"];
  std::string s = inif["section"]["key2"];
}

void read_func()
{
  std::istringstream is(str); // Other istreams are also possible
  ini::inifile inif;
  inif.read(is);
}

void write_func()
{
  std::ostringstream os; // Other istreams are also possible
  ini::inifile inif;
  inif["section"]["key"] = "value";
  inif.write(os);
}

void to_string_func()
{
  ini::inifile inif;
  inif["section"]["key"] = "value";
  std::string s = inif.to_string();
}

void from_string_func()
{
  std::string s(str);
  ini::inifile inif;
  inif.from_string(s);
}

int main()
{
  save_func();
  load_func();

  read_func();
  write_func();

  to_string_func();
  from_string_func();
}