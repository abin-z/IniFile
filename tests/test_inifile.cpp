#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <inifile/inifile.h>

TEST_CASE("basic test")
{
  REQUIRE(1 + 1 == 2);
#ifdef _WIN32
  // msvc下能通过: msvc下double和long double一样, 都是 (64-bit IEEE 754), 但是gcc和clang不一样
  CHECK(sizeof(double) == sizeof(long double));
  CHECK(std::numeric_limits<long double>::max() == std::numeric_limits<double>::max());
#endif
}

TEST_CASE("trim function", "[trim]")
{
  using namespace ini;

  SECTION("Trim removes leading and trailing spaces")
  {
    REQUIRE(trim("  hello  ") == "hello");
    REQUIRE(trim("\t hello \n") == "hello");
    REQUIRE(trim("  test string  ") == "test string");
  }

  SECTION("Trim handles various whitespace characters")
  {
    REQUIRE(trim("\n\nhello\t ") == "hello");
    REQUIRE(trim("  \r\n  text  \t") == "text");
  }

  SECTION("Trim handles empty and all-whitespace strings")
  {
    REQUIRE(trim("") == "");
    REQUIRE(trim("      ") == "");
    REQUIRE(trim("\t\n  \t") == "");
  }

  SECTION("Trim has no effect on already trimmed strings")
  {
    REQUIRE(trim("hello") == "hello");
    REQUIRE(trim("trimmed") == "trimmed");
  }

  SECTION("Trim handles strings with only one character")
  {
    REQUIRE(trim("a") == "a");
    REQUIRE(trim(" ") == "");
    REQUIRE(trim("\n") == "");
  }
}

TEST_CASE("split function", "[split]")
{
  using namespace ini;

  SECTION("Split normal string")
  {
    REQUIRE(split("a,b,c", ',') == std::vector<std::string>{"a", "b", "c"});
    REQUIRE(split("hello world example", ' ') == std::vector<std::string>{"hello", "world", "example"});
  }

  SECTION("Split with consecutive delimiters (skip_empty = false)")
  {
    REQUIRE(split("a,,b,c", ',') == std::vector<std::string>{"a", "", "b", "c"});
    REQUIRE(split("one:::two:three", ':') == std::vector<std::string>{"one", "", "", "two", "three"});
  }

  SECTION("Split with consecutive delimiters (skip_empty = true)")
  {
    REQUIRE(split("a,,b,c", ',', true) == std::vector<std::string>{"a", "b", "c"});
    REQUIRE(split("one:::two:three", ':', true) == std::vector<std::string>{"one", "two", "three"});
  }

  SECTION("Split handles edge cases")
  {
    REQUIRE(split("", ',') == std::vector<std::string>{""});
    REQUIRE(split(",", ',') == std::vector<std::string>{"", ""});
    REQUIRE(split(",", ',', true) == std::vector<std::string>{});   // 跳过空字符串
    REQUIRE(split(",,", ',', true) == std::vector<std::string>{});  // 全部跳过
    REQUIRE(split("singleword", ',') == std::vector<std::string>{"singleword"});
  }

  SECTION("Split when delimiter is at the start or end")
  {
    REQUIRE(split(",abc,def,", ',') == std::vector<std::string>{"", "abc", "def", ""});
    REQUIRE(split(",abc,def,", ',', true) == std::vector<std::string>{"abc", "def"});
  }

  SECTION("Split when there are no delimiters")
  {
    REQUIRE(split("singleword", ',') == std::vector<std::string>{"singleword"});
    REQUIRE(split("  trim this  ", ' ') == std::vector<std::string>{"", "", "trim", "this", "", ""});
    REQUIRE(split("  trim this  ", ' ', true) == std::vector<std::string>{"trim", "this"});
  }

  SECTION("Split with different delimiters")
  {
    REQUIRE(split("apple|banana|cherry", '|') == std::vector<std::string>{"apple", "banana", "cherry"});
    REQUIRE(split("key=value=pair", '=') == std::vector<std::string>{"key", "value", "pair"});
  }
}

TEST_CASE("type convert test01", "[inifile]")
{
  ini::inifile file;
  file["section"]["key"] = 1;
  CHECK(file["section"]["key"].as<bool>() == true);
  CHECK(file["section"]["key"].as<char>() == '1');
  CHECK(file["section"]["key"].as<signed char>() == '1');
  CHECK(file["section"]["key"].as<unsigned char>() == '1');
  CHECK(file["section"]["key"].as<short>() == 1);
  CHECK(file["section"]["key"].as<int>() == 1);
  CHECK(file["section"]["key"].as<long>() == 1);
  CHECK(file["section"]["key"].as<long long>() == 1);
  CHECK(file["section"]["key"].as<unsigned int>() == 1);
  CHECK(file["section"]["key"].as<unsigned long>() == 1);
  CHECK(file["section"]["key"].as<unsigned long long>() == 1);
  CHECK(file["section"]["key"].as<float>() == 1);
  CHECK(file["section"]["key"].as<double>() == 1);
  CHECK(file["section"]["key"].as<long double>() == 1);
  CHECK(file["section"]["key"].as<std::string>() == "1");
  CHECK(file["section"]["key"].as<const char *>() == std::to_string(1));
}

TEST_CASE("type convert test02", "[inifile]")
{
  ini::inifile file;
  file["section"]["key"] = 'A';
  char cc = file["section"]["key"];
  signed char sc = file["section"]["key"];
  unsigned char uc = file["section"]["key"];
  REQUIRE(cc == 'A');
  REQUIRE(sc == 'A');
  REQUIRE(uc == 'A');
}

TEMPLATE_TEST_CASE("Different types", "[template]",
                   char,
                   unsigned char,
                   signed char,
                   short,
                   int,
                   long,
                   long long,
                   unsigned short,
                   unsigned int,
                   unsigned long,
                   unsigned long long,
                   float,
                   double,
                   long double)
{
  TestType x = 1;
  ini::inifile file;
  file["section"]["key"] = x;
  TestType result = file["section"]["key"];
  REQUIRE(x == result);
}

TEMPLATE_TEST_CASE("Different types int", "[template]",
                   char,
                   unsigned char,
                   signed char,
                   short,
                   int,
                   long,
                   long long,
                   unsigned short,
                   unsigned int,
                   unsigned long,
                   unsigned long long)
{
  TestType x = 0;
  ini::inifile file;
  file["section"]["key"] = x;
  TestType result = file["section"]["key"];
  REQUIRE(x == result);

  TestType xx = 19;
  file["section"]["key"] = xx;
  TestType result_xx = file["section"]["key"];
  REQUIRE(xx == result_xx);
}

TEMPLATE_TEST_CASE("Different types float", "[template]",
                   float,
                   double,
                   long double)
{
  TestType x = 3.141592;
  ini::inifile file;
  file["section"]["key"] = x;
  TestType result = file["section"]["key"];
  REQUIRE(Approx(result).epsilon(10e-6) == x); // 允许误差
}

TEST_CASE("test out of range", "[inifile]")
{
  REQUIRE_THROWS_AS([]
                    {
                      ini::inifile file;
                      file["section"]["key"] = std::numeric_limits<unsigned int>::max();
                      uint32_t ui = file["section"]["key"];
                      int si = file["section"]["key"]; // out_of_range
                    }(),
                    std::out_of_range);

  REQUIRE_THROWS_AS([]
                    {
                      ini::inifile file;
                      file["section"]["key"] = std::numeric_limits<long double>::max();
                      long double v1 = file["section"]["key"];
                      float v2 = file["section"]["key"]; // out_of_range
                    }(),
                    std::out_of_range);

  REQUIRE_NOTHROW([]
                  {
                    ini::inifile file;
                    file["section"]["key"] = std::numeric_limits<unsigned int>::max();

                    uint32_t ui = file["section"]["key"];  // 读取 uint32_t
                    int64_t si1 = file["section"]["key"];  // 读取 int64_t
                    uint64_t si2 = file["section"]["key"]; // 读取 uint64_t
                  }());
}

TEST_CASE("member func test01", "[inifile]")
{
  ini::inifile file;
  file["section"]["key"] = "hello world";
  REQUIRE(file.contains("section") == true);
  REQUIRE(file.contains("section", "key") == true);
  REQUIRE(file.contains("section_no") == false);
  REQUIRE(file.contains("section_no", "key") == false);
  REQUIRE(file.contains("section", "key_no") == false);
  REQUIRE(file["section"].size() == 1);
  REQUIRE(file["section"]["key"].as<std::string>() == std::string("hello world"));
  REQUIRE_FALSE(file.contains("section") != true);
}

TEST_CASE("member func test02", "[inifile]")
{
  ini::inifile file;
  file["section"]["key"] = 3.14;
  REQUIRE(file.size() == 1);
  REQUIRE(file["section"].size() == 1);
  REQUIRE(file["section01"].size() == 0);
  REQUIRE(file.size() == 2);
  REQUIRE_THROWS_AS([&file]
                    { file.at("section_no"); }(), std::out_of_range);
  REQUIRE_THROWS_AS([&file]
                    { file.at("section").at("key_no"); }(), std::out_of_range);
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.at("section").at("key"); }());
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.at("section01"); }());
  REQUIRE_FALSE(file.contains("section01") != true);
  REQUIRE(file.contains("section_no") == false);
  REQUIRE(file.contains("section_no") == false);
  REQUIRE(file.at("section").at("key").as<std::string>() != "3.14");
}

TEST_CASE("member func test03", "[inifile]")
{
  ini::inifile file;
  file["section"]["key"] = 3.14;
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.get("section", "key_no"); }());
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.at("section").get("key_no"); }());

  REQUIRE_NOTHROW([&file]
                  { auto ret = file.get("section", "key_no", "default"); }());
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.at("section").get("key_no", 55); }());

  REQUIRE_THROWS_AS([&file]
                    { file.at("section_no"); }(), std::out_of_range);
  REQUIRE_THROWS_AS([&file]
                    { file.at("section_no").at("key"); }(), std::out_of_range);
  REQUIRE_THROWS_AS([&file]
                    { file.at("section").at("key_no"); }(), std::out_of_range);
}

TEST_CASE("member func test04", "[inifile]")
{
  ini::inifile file;
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.get("section", "key_no", "default"); }());
  REQUIRE_NOTHROW([&file]
                  { auto ret = file.get("section", "key_no", 55).as<float>(); }());

  REQUIRE_THROWS_AS([&file]
                    { auto ret = file.get("section", "key_no", "default").as<int>(); }(), std::invalid_argument);
}

TEST_CASE("member func test05", "[inifile]")
{
  ini::inifile file;
  REQUIRE_NOTHROW([&file]
                  { auto ret0 = file["section"].get("key_no");
                    auto ret1 = file["section"].get("key_no", "default"); }());

  REQUIRE_THROWS_AS([&file]
                    { auto ret = file.get("section", "key_no", "default").as<double>(); }(), std::invalid_argument);
}

TEST_CASE("member func test06", "[inifile]")
{
  REQUIRE_NOTHROW([]
                  { 
                    ini::field f;
                    ini::field f1(1);
                    ini::field f2(true);
                    ini::field f3(3.14);
                    ini::field f4('c');
                    ini::field f5("abc");
                    ini::field f6(3.14f);
                    ini::field f7(999999999ll);
                    ini::field f8 = "hello";
                    f8 = 3.14; }());
}

TEST_CASE("member func test07", "[inifile]")
{
  REQUIRE_NOTHROW([]
                  { 
                    // 其他类型 -> ini::field
                    ini::field f(true);
                    ini::field f1(10);
                    ini::field f2 = 3.14;
                    ini::field f3 = 'c';
                    ini::field f4 = "abc";
                      
                    // ini::field -> 其他类型
                    bool b = f;
                    int i = f1;
                    double d = f2;
                    char c = f3;
                    std::string s = f4; 

                    ini::inifile inif;
                    inif["section"]["key"] = true; // bool -> ini::field
                    
                    /// Get direct type(ini::field)
                    auto val = inif["section"]["key"]; // val type is ini::field
                    ini::field val2 = inif["section"]["key"]; 
                    
                    /// explicit type conversion
                    bool bb = inif["section"]["key"].as<bool>();
                      
                    /// automatic type conversion
                    bool bb2 = inif["section"]["key"]; }());
}

TEST_CASE("member func test08", "[inifile]")
{
  REQUIRE_NOTHROW([]
                  { 
                    ini::inifile inif;
                    inif["only_section"];
                    inif["section"]["only_key"];
                    inif[""][""];
                    inif[""]["key"];
                    inif["section0"][""];
                    inif.save("./test.ini"); }());
}

TEST_CASE("member func test09", "[inifile]")
{
  ini::inifile inif;
  inif["section"]["key"] = true;
  CHECK(inif.contains("section") == true);
  CHECK(inif.contains("section_no") == false);
  CHECK(inif.contains("section", "key") == true);
  CHECK(inif.contains("section_no", "key") == false);
  CHECK(inif.contains("section", "key_no") == false);
  CHECK(inif.contains("section_no", "key_no") == false);

  CHECK(inif.at("section").contains("key") == true);
  CHECK(inif.at("section").contains("key_no") == false);

  CHECK(inif["section"].contains("key") == true);
  CHECK(inif["section"].contains("key_no") == false);

  CHECK(inif["section_no"].contains("") == false);
  CHECK(inif["section_no"].contains("key_no") == false);

  inif[""]["num"] = 12345;
  CHECK(inif[""].contains("num") == true); // 允许key为空字符串
}

TEST_CASE("member func test10", "[inifile]")
{
  ini::inifile inif;
  CHECK(inif.empty() == true);
  CHECK(inif.size() == 0);
  CHECK(inif.count("section") == 0);
  CHECK((inif.find("section") == inif.end()) == true);

  inif["section"]["key"] = true;
  inif["section"]["key"] = false;
  CHECK(inif.empty() == false);
  CHECK(inif.size() == 1);
  CHECK(inif.count("section") == 1);
  bool ret = inif.find("section") != inif.end();
  CHECK(ret == true);
}

TEST_CASE("member func test11", "[inifile]")
{
  const char *path = "./test.ini";
  ini::inifile inif;
  inif.set("section", "key", 100);
  inif.set("section", "key1", 101);
  inif.set("section", "key2", 102);
  inif.set("section", "key3", 103);
  inif.set("section", "key4", 104);
  inif.set("section", "key5", 105);

  inif.at("section").clear_comment();
  inif.at("section").set_comment("section注释信息");
  inif.at("section").at("key1").set_comment("key-value注释信息1", '#');
  inif.save(path);

  ini::inifile inif2;
  inif2.load(path);
  inif2.at("section").at("key3").set_comment("key-value注释信息3");
  inif2.save(path);

  bool ret = inif2.find("section") != inif2.end();
  CHECK(ret == true);
}