#include <inifile/inifile.h>

constexpr char path[] = "basic.ini";
const char *str = R"(
  [section]
  key=value
)";

void save_func()
{
  ini::inifile inif;
  // Set value
  inif["section"]["key0"] = true;
  inif["section"]["key1"] = 3.141592;
  inif["section"]["key2"] = "value";

  // Add comments if necessary
  inif["section"].set_comment("This is a section comment.");                      // set section comment, Overwrite Mode
  inif["section"]["key1"].set_comment("This is a key-value pairs comment", '#');  // set key=value pairs comment

  bool isok = inif.save(path);
}

void load_func()
{
  ini::inifile inif;
  try
  {
    // Call the load method to load the ini file and return whether the loading was successful
    bool isok = inif.load(path);
    bool        b = inif["section"]["key0"];
    double      d = inif["section"]["key1"];
    std::string s = inif["section"]["key2"];

    // explicit type conversion
    bool        bb = inif["section"]["key0"].as<bool>();
    double      dd = inif["section"]["key1"].as<double>();
    std::string ss = inif["section"]["key2"].as<std::string>();
  }
  catch (const std::exception &e)
  {
    std::cout << e.what() << std::endl;
  }
}

void read_func()
{
  std::istringstream is(str);  // Other istreams are also possible
  ini::inifile inif;
  inif.read(is);
}

void write_func()
{
  std::ostringstream os;  // Other istreams are also possible
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
  std::cout << "inifile_basic finish." << std::endl;
}