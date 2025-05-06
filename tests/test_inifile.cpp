#define CATCH_CONFIG_MAIN
#include <inifile/inifile.h>

#include <array>
#include <catch.hpp>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <string>
#include <vector>

TEST_CASE("basic test")
{
  REQUIRE(1 + 1 == 2);
#ifdef _WIN32
  // msvc下能通过: msvc下double和long double一样, 都是 (64-bit IEEE 754), 但是gcc和clang不一样
  CHECK(sizeof(double) == sizeof(long double));
  CHECK(std::numeric_limits<long double>::max() == std::numeric_limits<double>::max());
#endif
}

TEST_CASE("Field basic functionality", "[field]")
{
  using namespace ini;
  SECTION("Default constructor")
  {
    field f;
    REQUIRE(f.as<std::string>() == "");
  }

  SECTION("Parameterized constructor with string")
  {
    field f("value");
    REQUIRE(f.as<std::string>() == "value");
  }

  SECTION("Move constructor")
  {
    field f1("value");
    field f2(std::move(f1));
    REQUIRE(f2.as<std::string>() == "value");
  }

  SECTION("Move assignment operator")
  {
    field f1("value");
    field f2;
    f2 = std::move(f1);
    REQUIRE(f2.as<std::string>() == "value");
  }

  SECTION("Copy constructor")
  {
    field f1("value");
    field f2(f1);
    REQUIRE(f2.as<std::string>() == "value");
  }

  SECTION("Copy assignment operator")
  {
    field f1("value");
    field f2;
    f2 = f1;
    REQUIRE(f2.as<std::string>() == "value");
  }

  SECTION("Template constructor with int")
  {
    field f(10);
    REQUIRE(f.as<int>() == 10);
  }

  SECTION("Template assignment with double")
  {
    field f("10.5");
    f = 20.5;
    REQUIRE(f.as<double>() == 20.5);
  }

  SECTION("Conversion to int")
  {
    field f("10");
    int val = f;
    REQUIRE(val == 10);
  }

  SECTION("Setting a value")
  {
    field f;
    f.set(42);
    REQUIRE(f.as<int>() == 42);
  }

  SECTION("Set comment")
  {
    field f("value");
    f.set_comment("This is a comment");
    REQUIRE(f.as<std::string>() == "value");
    // Check comment handling functionality
  }
}

TEST_CASE("basic_section default constructor initializes empty", "[section]")
{
  ini::section s;
  REQUIRE(s.empty());
  REQUIRE(s.size() == 0);
}

TEST_CASE("basic_section key-value assignment and access", "[section]")
{
  ini::section s;
  s.set("username", "admin");
  s.set("timeout", 30);

  REQUIRE(s.contains("username"));
  REQUIRE(s.contains("timeout"));
  REQUIRE(s["username"].as<std::string>() == "admin");
  REQUIRE(s["timeout"].as<std::string>() == "30");
}

TEST_CASE("basic_section at and get behavior", "[section]")
{
  ini::section s;
  s.set("port", 8080);

  REQUIRE(s.at("port").as<std::string>() == "8080");
  REQUIRE_THROWS_AS(s.at("not_exist"), std::out_of_range);

  auto fallback = s.get("not_exist", ini::field("fallback"));
  REQUIRE(fallback.as<std::string>() == "fallback");
}

TEST_CASE("basic_section copy constructor deep copies", "[section]")
{
  ini::section s1;
  s1.set("ip", "127.0.0.1");
  s1.set_comment("network config");

  ini::section s2(s1);  // copy

  REQUIRE(s2.contains("ip"));
  REQUIRE(s2["ip"].as<std::string>() == "127.0.0.1");

  // 改变 s1 不影响 s2
  s1.set("ip", "changed");
  REQUIRE(s2["ip"].as<std::string>() == "127.0.0.1");
}

TEST_CASE("basic_section copy assignment is deep and exception-safe", "[section]")
{
  ini::section s1;
  s1.set("key", "value");
  s1.add_comment("some comment");

  ini::section s2;
  s2 = s1;  // copy assignment

  REQUIRE(s2.contains("key"));
  REQUIRE(s2["key"].as<std::string>() == "value");
  REQUIRE(s2.keys().size() == 1);
}

TEST_CASE("basic_section move constructor transfers ownership", "[section]")
{
  ini::section s1;
  s1.set("a", 1);
  s1.add_comment("moved");

  ini::section s2(std::move(s1));  // move

  REQUIRE(s2.contains("a"));
  REQUIRE(s2["a"].as<std::string>() == "1");

  REQUIRE_NOTHROW(s1.size());  // 合法但状态未指定
}

TEST_CASE("basic_section move assignment transfers ownership", "[section]")
{
  ini::section s1;
  s1.set("b", 2);
  s1.set_comment("m");

  ini::section s2;
  s2.set("temp", "value");

  s2 = std::move(s1);

  REQUIRE(s2.contains("b"));
  REQUIRE(s2["b"].as<std::string>() == "2");

  REQUIRE(!s2.contains("temp"));  // 被替换掉了
}

TEST_CASE("basic_section self copy assignment safe", "[section]")
{
  ini::section s;
  s.set("x", "xvalue");
  s = s;

  REQUIRE(s.contains("x"));
  REQUIRE(s["x"].as<std::string>() == "xvalue");
}

TEST_CASE("basic_section self move assignment safe", "[section]")
{
  ini::section s;
  s.set("y", "yvalue");
  s = std::move(s);

  REQUIRE(s.contains("y"));
  REQUIRE(s["y"].as<std::string>() == "yvalue");
}

TEST_CASE("basic_section remove, clear and erase", "[section]")
{
  ini::section s;
  s.set("a", 1);
  s.set("b", 2);

  REQUIRE(s.remove("a"));
  REQUIRE_FALSE(s.contains("a"));

  REQUIRE(s.erase("b") == 1);
  REQUIRE(s.empty());

  s.set("x", 42);
  s.clear();
  REQUIRE(s.empty());
}

TEST_CASE("basic_section comment operations", "[section]")
{
  ini::section s;
  s.set_comment("section comment");
  s.add_comment("line2\nline3");

  ini::field f;
  f.add_comment("field comment");
  s.set("k", f);
}

TEST_CASE("basic_section iterator and keys", "[section]")
{
  ini::section s;
  s.set("one", 1);
  s.set("two", 2);

  std::vector<std::string> expected_keys = {"one", "two"};
  auto keys = s.keys();

  for (const auto &key : expected_keys)
  {
    REQUIRE(std::find(keys.begin(), keys.end(), key) != keys.end());
  }

  size_t count = 0;
  for (const auto &pair : s)
  {
    ++count;
    REQUIRE((pair.first == "one" || pair.first == "two"));
  }
  REQUIRE(count == 2);
}

TEST_CASE("Basic Section functionality", "[basic_section]")
{
  using namespace ini;
  ini::section section;

  SECTION("Set and get key-value pair")
  {
    section.set("key1", 42);
    REQUIRE(section.at("key1").as<int>() == 42);
  }

  SECTION("Check if key exists")
  {
    section.set("key2", 100);
    REQUIRE(section.contains("key2"));
    REQUIRE_FALSE(section.contains("key3"));
  }

  SECTION("Get with default value")
  {
    section.set("key3", "Hello");
    field default_field;
    REQUIRE(section.get("key3", default_field).as<std::string>() == "Hello");
    REQUIRE(section.get("key4", default_field).as<std::string>() == "");
  }

  SECTION("Remove key-value pair")
  {
    section.set("key4", "Test");
    REQUIRE(section.remove("key4"));
    REQUIRE_FALSE(section.contains("key4"));
  }

  SECTION("Get all keys")
  {
    section.set("key5", 55);
    section.set("key6", 66);
    std::vector<std::string> keys = section.keys();
    REQUIRE(keys.size() == 2);
    REQUIRE(std::find(keys.begin(), keys.end(), "key5") != keys.end());
    REQUIRE(std::find(keys.begin(), keys.end(), "key6") != keys.end());
  }

  SECTION("Set and get comment")
  {
    section.set_comment("Section comment");
    section.add_comment("Additional comment");
    // Assuming we have a way to check comments:
    // REQUIRE(section.comments().size() == 2);
  }

  SECTION("Copy constructor")
  {
    section.set("key7", 123);
    basic_section<> copied_section(section);
    REQUIRE(copied_section.at("key7").as<int>() == 123);
  }

  SECTION("Copy assignment operator")
  {
    section.set("key8", "Copy assignment");
    basic_section<> copied_section;
    copied_section = section;
    REQUIRE(copied_section.at("key8").as<std::string>() == "Copy assignment");
  }
}

TEST_CASE("basic_inifile copy constructor and copy assignment")
{
  ini::inifile inifile1;

  // Set some sections and fields
  inifile1.set("section1", "key1", "value1");
  inifile1.set("section2", "key2", "value2");

  // Use copy constructor
  ini::inifile inifile2 = inifile1;
  REQUIRE(inifile2.contains("section1"));
  REQUIRE(inifile2.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile2.contains("section2"));
  REQUIRE(inifile2.get("section2", "key2").as<std::string>() == "value2");

  // Use copy assignment
  ini::inifile inifile3;
  inifile3 = inifile1;
  REQUIRE(inifile3.contains("section1"));
  REQUIRE(inifile3.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile3.contains("section2"));
  REQUIRE(inifile3.get("section2", "key2").as<std::string>() == "value2");
}

TEST_CASE("basic_inifile move constructor and move assignment")
{
  ini::inifile inifile1;

  // Set some sections and fields
  inifile1.set("section1", "key1", "value1");
  inifile1.set("section2", "key2", "value2");

  // Move constructor
  ini::inifile inifile2 = std::move(inifile1);
  REQUIRE(inifile2.contains("section1"));
  REQUIRE(inifile2.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile2.contains("section2"));
  REQUIRE(inifile2.get("section2", "key2").as<std::string>() == "value2");
  REQUIRE(inifile1.empty());  // Ensure inifile1 is empty after move

  // Move assignment
  ini::inifile inifile3;
  inifile3 = std::move(inifile2);
  REQUIRE(inifile3.contains("section1"));
  REQUIRE(inifile3.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile3.contains("section2"));
  REQUIRE(inifile3.get("section2", "key2").as<std::string>() == "value2");
  REQUIRE(inifile2.empty());  // Ensure inifile2 is empty after move
}

TEST_CASE("basic_inifile self-assignment")
{
  ini::inifile inifile;

  // Set some sections and fields
  inifile.set("section1", "key1", "value1");
  inifile.set("section2", "key2", "value2");

  // Self-assignment for copy assignment operator
  inifile = inifile;
  REQUIRE(inifile.contains("section1"));
  REQUIRE(inifile.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile.contains("section2"));
  REQUIRE(inifile.get("section2", "key2").as<std::string>() == "value2");

  // Self-assignment for move assignment (should be safe)
  inifile = std::move(inifile);
  REQUIRE(inifile.contains("section1"));
  REQUIRE(inifile.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile.contains("section2"));
  REQUIRE(inifile.get("section2", "key2").as<std::string>() == "value2");
}

TEST_CASE("basic_inifile contains and at methods")
{
  ini::inifile inifile;

  // Set some sections and fields
  inifile.set("section1", "key1", "value1");

  // Testing contains() method
  REQUIRE(inifile.contains("section1"));
  REQUIRE(inifile.contains("section1", "key1"));
  REQUIRE(!inifile.contains("section2"));
  REQUIRE(!inifile.contains("section1", "key2"));

  // Testing at() method
  REQUIRE(inifile.at("section1").contains("key1"));
  REQUIRE(inifile.at("section1").at("key1").as<std::string>() == "value1");

  // Testing exception when section does not exist
  REQUIRE_THROWS_AS(inifile.at("nonexistent_section"), std::out_of_range);
}

TEST_CASE("basic_inifile edge cases: empty section, empty key")
{
  ini::inifile inifile;

  // Test empty section name
  inifile.set("", "key1", "value1");
  REQUIRE(inifile.contains(""));
  REQUIRE(inifile.get("", "key1").as<std::string>() == "value1");

  // Test empty key name
  inifile.set("section1", "", "value1");
  REQUIRE(inifile.contains("section1"));
  REQUIRE(inifile.get("section1", "").as<std::string>() == "value1");
}

TEST_CASE("basic_inifile load and save methods")
{
  ini::inifile inifile;

  // Set some sections and fields
  inifile.set("section1", "key1", "value1");
  inifile.set("section2", "key2", "value2");

  // Save to a file
  std::string filename = "test01.ini";
  REQUIRE(inifile.save(filename));

  // Load from the file
  ini::inifile inifile_loaded;
  REQUIRE(inifile_loaded.load(filename));
  REQUIRE(inifile_loaded.contains("section1"));
  REQUIRE(inifile_loaded.get("section1", "key1").as<std::string>() == "value1");
  REQUIRE(inifile_loaded.contains("section2"));
  REQUIRE(inifile_loaded.get("section2", "key2").as<std::string>() == "value2");
}

TEST_CASE("basic_inifile sections")
{
  ini::inifile inifile;

  // Set some sections and fields
  inifile.set("section1", "key1", "value1");
  inifile.set("section2", "key2", "value2");

  // Get all sections
  auto sections = inifile.sections();
  REQUIRE(sections.size() == 2);
  REQUIRE(std::find(sections.begin(), sections.end(), "section1") != sections.end());
  REQUIRE(std::find(sections.begin(), sections.end(), "section2") != sections.end());
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

  SECTION("Split with Unicode-like delimiter")
  {
    REQUIRE(split("红-绿-蓝", '-') == std::vector<std::string>{"红", "绿", "蓝"});
    REQUIRE(split("路径#文件#类型", '#') == std::vector<std::string>{"路径", "文件", "类型"});
  }
}

TEST_CASE("split function with string delimiter", "[split][string-delim]")
{
  using namespace ini;

  SECTION("Split with string delimiter - basic cases")
  {
    REQUIRE(split("a::b::c", "::") == std::vector<std::string>{"a", "b", "c"});
    REQUIRE(split("one--two--three", "--") == std::vector<std::string>{"one", "two", "three"});
  }

  SECTION("Split with string delimiter - consecutive delimiters")
  {
    REQUIRE(split("a::::b::c", "::") == std::vector<std::string>{"a", "", "b", "c"});
    REQUIRE(split("x##y####z", "##") == std::vector<std::string>{"x", "y", "", "z"});
  }

  SECTION("Split with string delimiter - skip_empty = true")
  {
    REQUIRE(split("a::::b::c", "::", true) == std::vector<std::string>{"a", "b", "c"});
    REQUIRE(split("x##y####z", "##", true) == std::vector<std::string>{"x", "y", "z"});
  }

  SECTION("Split with string delimiter - edge cases")
  {
    REQUIRE(split("", "::") == std::vector<std::string>{""});
    REQUIRE(split("::", "::") == std::vector<std::string>{"", ""});
    REQUIRE(split("::", "::", true) == std::vector<std::string>{});
    REQUIRE(split("no-delimiter", "::") == std::vector<std::string>{"no-delimiter"});
  }

  SECTION("Split with string delimiter at start or end")
  {
    REQUIRE(split("::a::b::", "::") == std::vector<std::string>{"", "a", "b", ""});
    REQUIRE(split("::a::b::", "::", true) == std::vector<std::string>{"a", "b"});
  }

  SECTION("Split with non-overlapping multi-char delimiters")
  {
    REQUIRE(split("a<>b<>c<>d", "<>") == std::vector<std::string>{"a", "b", "c", "d"});
    REQUIRE(split("123==456==789", "==") == std::vector<std::string>{"123", "456", "789"});
  }

  SECTION("Split with overlapping delimiter characters")
  {
    REQUIRE(split("aaaa", "aa") == std::vector<std::string>{"", "", ""});
    REQUIRE(split("aaaa", "aa", true) == std::vector<std::string>{});
  }

  SECTION("Split with long delimiter and longer content")
  {
    std::string delim = "==SPLIT==";
    std::string input = "part1==SPLIT==part2==SPLIT==part3";
    REQUIRE(split(input, delim) == std::vector<std::string>{"part1", "part2", "part3"});
  }

  SECTION("Split with long delimiter, empty parts")
  {
    std::string delim = "--DELIM--";
    std::string input = "--DELIM--a--DELIM----DELIM--b--DELIM--";
    REQUIRE(split(input, delim) == std::vector<std::string>{"", "a", "", "b", ""});
    REQUIRE(split(input, delim, true) == std::vector<std::string>{"a", "b"});
  }

  SECTION("Split string of only delimiters")
  {
    REQUIRE(split("::", "::") == std::vector<std::string>{"", ""});
    REQUIRE(split("::::", "::") == std::vector<std::string>{"", "", ""});
    REQUIRE(split("::::", "::", true) == std::vector<std::string>{});
  }

  SECTION("Split with Unicode-like delimiter")
  {
    REQUIRE(split("红-绿-蓝", "-") == std::vector<std::string>{"红", "绿", "蓝"});
    REQUIRE(split("路径##文件##类型", "##") == std::vector<std::string>{"路径", "文件", "类型"});
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

TEMPLATE_TEST_CASE("Different types", "[template]", char, unsigned char, signed char, short, int, long, long long,
                   unsigned short, unsigned int, unsigned long, unsigned long long, float, double, long double)
{
  TestType x = 1;
  ini::inifile file;
  file["section"]["key"] = x;
  TestType result = file["section"]["key"];
  REQUIRE(x == result);
}

TEMPLATE_TEST_CASE("Different types int", "[template]", char, unsigned char, signed char, short, int, long, long long,
                   unsigned short, unsigned int, unsigned long, unsigned long long)
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

TEMPLATE_TEST_CASE("Different types float", "[template]", float, double, long double)
{
  TestType x = 3.141592;
  ini::inifile file;
  file["section"]["key"] = x;
  TestType result = file["section"]["key"];
  REQUIRE(Approx(result).epsilon(10e-6) == x);  // 允许误差
}

TEST_CASE("test out of range", "[inifile]")
{
  REQUIRE_THROWS_AS(
    [] {
      ini::inifile file;
      file["section"]["key"] = std::numeric_limits<unsigned int>::max();
      uint32_t ui = file["section"]["key"];
      int si = file["section"]["key"];  // out_of_range
    }(),
    std::out_of_range);

  REQUIRE_THROWS_AS(
    [] {
      ini::inifile file;
      file["section"]["key"] = std::numeric_limits<long double>::max();
      long double v1 = file["section"]["key"];
      float v2 = file["section"]["key"];  // out_of_range
    }(),
    std::out_of_range);

  REQUIRE_NOTHROW([] {
    ini::inifile file;
    file["section"]["key"] = std::numeric_limits<unsigned int>::max();

    uint32_t ui = file["section"]["key"];   // 读取 uint32_t
    int64_t si1 = file["section"]["key"];   // 读取 int64_t
    uint64_t si2 = file["section"]["key"];  // 读取 uint64_t
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
  REQUIRE_THROWS_AS([&file] { file.at("section_no"); }(), std::out_of_range);
  REQUIRE_THROWS_AS([&file] { file.at("section").at("key_no"); }(), std::out_of_range);
  REQUIRE_NOTHROW([&file] { auto ret = file.at("section").at("key"); }());
  REQUIRE_NOTHROW([&file] { auto ret = file.at("section01"); }());
  REQUIRE_FALSE(file.contains("section01") != true);
  REQUIRE(file.contains("section_no") == false);
  REQUIRE(file.contains("section_no") == false);
  REQUIRE(file.at("section").at("key").as<std::string>() != "3.14");
}

TEST_CASE("member func test03", "[inifile]")
{
  ini::inifile file;
  file["section"]["key"] = 3.14;
  REQUIRE_NOTHROW([&file] { auto ret = file.get("section", "key_no"); }());
  REQUIRE_NOTHROW([&file] { auto ret = file.at("section").get("key_no"); }());

  REQUIRE_NOTHROW([&file] { auto ret = file.get("section", "key_no", "default"); }());
  REQUIRE_NOTHROW([&file] { auto ret = file.at("section").get("key_no", 55); }());

  REQUIRE_THROWS_AS([&file] { file.at("section_no"); }(), std::out_of_range);
  REQUIRE_THROWS_AS([&file] { file.at("section_no").at("key"); }(), std::out_of_range);
  REQUIRE_THROWS_AS([&file] { file.at("section").at("key_no"); }(), std::out_of_range);
}

TEST_CASE("member func test04", "[inifile]")
{
  ini::inifile file;
  REQUIRE_NOTHROW([&file] { auto ret = file.get("section", "key_no", "default"); }());
  REQUIRE_NOTHROW([&file] { auto ret = file.get("section", "key_no", 55).as<float>(); }());

  REQUIRE_THROWS_AS([&file] { auto ret = file.get("section", "key_no", "default").as<int>(); }(),
                    std::invalid_argument);
}

TEST_CASE("member func test05", "[inifile]")
{
  ini::inifile file;
  REQUIRE_NOTHROW([&file] {
    auto ret0 = file["section"].get("key_no");
    auto ret1 = file["section"].get("key_no", "default");
  }());

  REQUIRE_THROWS_AS([&file] { auto ret = file.get("section", "key_no", "default").as<double>(); }(),
                    std::invalid_argument);
}

TEST_CASE("member func test06", "[inifile]")
{
  REQUIRE_NOTHROW([] {
    ini::field f;
    ini::field f1(1);
    ini::field f2(true);
    ini::field f3(3.14);
    ini::field f4('c');
    ini::field f5("abc");
    ini::field f6(3.14f);
    ini::field f7(999999999ll);
    ini::field f8 = "hello";
    f8 = 3.14;
  }());
}

TEST_CASE("member func test07", "[inifile]")
{
  REQUIRE_NOTHROW([] {
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
    inif["section"]["key"] = true;  // bool -> ini::field

    /// Get direct type(ini::field)
    auto val = inif["section"]["key"];  // val type is ini::field
    ini::field val2 = inif["section"]["key"];

    /// explicit type conversion
    bool bb = inif["section"]["key"].as<bool>();

    /// automatic type conversion
    bool bb2 = inif["section"]["key"];
  }());
}

TEST_CASE("member func test08", "[inifile]")
{
  REQUIRE_NOTHROW([] {
    ini::inifile inif;
    inif["only_section"];
    inif["section"]["only_key"];
    inif[""][""];
    inif[""]["key"];
    inif["section0"][""];
    inif.save("./test.ini");
  }());
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
  CHECK(inif[""].contains("num") == true);  // 允许key为空字符串
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

TEST_CASE("sections and keys test01", "[inifile][sections][keys]")
{
  ini::inifile inif;

  // 添加多个 section 和 key-value 对
  inif["Network"]["host"] = "127.0.0.1";
  inif["Network"]["port"] = "8080";

  inif["Database"]["user"] = "admin";
  inif["Database"]["password"] = "secret";

  inif["Logging"]["level"] = "debug";

  // 测试 sections()
  auto sections = inif.sections();
  CHECK(sections.size() == 3);
  CHECK(std::find(sections.begin(), sections.end(), "Network") != sections.end());
  CHECK(std::find(sections.begin(), sections.end(), "Database") != sections.end());
  CHECK(std::find(sections.begin(), sections.end(), "Logging") != sections.end());

  // 测试 keys("Network")
  auto network_keys = inif["Network"].keys();  // 假设 section 类型实现了 keys()
  CHECK(network_keys.size() == 2);
  CHECK(std::find(network_keys.begin(), network_keys.end(), "host") != network_keys.end());
  CHECK(std::find(network_keys.begin(), network_keys.end(), "port") != network_keys.end());

  // 测试 keys("Logging")
  auto logging_keys = inif["Logging"].keys();
  CHECK(logging_keys.size() == 1);
  CHECK(logging_keys[0] == "level");

  // 空 section 测试
  inif["EmptySection"];  // 添加但不设置 key
  auto empty_keys = inif["EmptySection"].keys();
  CHECK(empty_keys.empty());
}

TEST_CASE("case insensitive test01", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = "Value";

  // Test case-insensitive section and key access
  CHECK(inif.contains("section") == true);
  CHECK(inif.contains("SECTION") == true);
  CHECK(inif.contains("Section") == true);
  CHECK(inif.contains("section", "key") == true);
  CHECK(inif.contains("SECTION", "KEY") == true);
  CHECK(inif.contains("Section", "Key") == true);

  // Test retrieval of values
  CHECK(inif["section"]["key"].as<std::string>() == "Value");
  CHECK(inif["SECTION"]["KEY"].as<std::string>() == "Value");
  CHECK(inif["Section"]["Key"].as<std::string>() == "Value");

  // Test modification of values
  inif["section"]["key"] = "NewValue";
  CHECK(inif["SECTION"]["KEY"].as<std::string>() == "NewValue");
}

TEST_CASE("case insensitive test02", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = 42;

  // Test case-insensitive numeric value retrieval
  CHECK(inif["section"]["key"].as<int>() == 42);
  CHECK(inif["SECTION"]["KEY"].as<int>() == 42);
  CHECK(inif["Section"]["Key"].as<int>() == 42);

  // Test case-insensitive existence checks
  CHECK(inif.contains("section", "key") == true);
  CHECK(inif.contains("SECTION", "KEY") == true);
  CHECK(inif.contains("Section", "Key") == true);

  // Test case-insensitive non-existent keys
  CHECK(inif.contains("section", "nonexistent") == false);
  CHECK(inif.contains("SECTION", "NONEXISTENT") == false);
}

TEST_CASE("case insensitive test03", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = "Value";

  // Test case-insensitive find
  auto it = inif.find("section");
  bool isfind = it != inif.end();
  CHECK(isfind == true);
  CHECK(it->first == "Section");
  CHECK(it->second.contains("key") == true);

  auto key_it = it->second.find("key");
  bool isfind_key = key_it != it->second.end();
  CHECK(isfind_key == true);
  CHECK(key_it->first == "Key");
  CHECK(key_it->second.as<std::string>() == "Value");

  // Test case-insensitive count
  CHECK(inif.count("section") == 1);
  CHECK(inif.count("SECTION") == 1);
}

TEST_CASE("case insensitive test04", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = "Value";

  // Test case-insensitive clear and size
  CHECK(inif.size() == 1);
  inif.clear();
  CHECK(inif.size() == 0);
  CHECK(inif.contains("section") == false);
}

TEST_CASE("case insensitive test05", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = "Value";

  // Test case-insensitive default value retrieval
  CHECK(inif.get("section", "key", "Default").as<std::string>() == "Value");
  CHECK(inif.get("SECTION", "KEY", "Default").as<std::string>() == "Value");
  CHECK(inif.get("section", "nonexistent", "Default").as<std::string>() == "Default");
}

TEST_CASE("case insensitive test06", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = "Value";

  // Test case-insensitive removal
  CHECK(inif.contains("section", "key") == true);
  inif["section"].erase("key");
  CHECK(inif.contains("section", "key") == false);

  CHECK(inif.contains("section") == true);
  inif.erase("section");
  CHECK(inif.contains("section") == false);
}

TEST_CASE("case insensitive test07", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  inif["Section"]["Key"] = "Value";
  inif["AnotherSection"]["AnotherKey"] = "AnotherValue";

  // Test case-insensitive iteration
  for (const auto &section : inif)
  {
    CHECK((section.first == "Section" || section.first == "AnotherSection"));
    for (const auto &key : section.second)
    {
      if (section.first == "Section")
      {
        CHECK(key.first == "Key");
        CHECK(key.second.as<std::string>() == "Value");
      }
      else if (section.first == "AnotherSection")
      {
        CHECK(key.first == "AnotherKey");
        CHECK(key.second.as<std::string>() == "AnotherValue");
      }
    }
  }
}

TEST_CASE("case insensitive test08", "[inifile][case_insensitive]")
{
  ini::case_insensitive_inifile inif;
  // 添加一些中文的测试
  inif["中文节"]["中文键"] = "中文值";

  // 测试中文的大小写不敏感性（实际上中文没有大小写，但测试是否受影响）
  CHECK(inif.contains("中文节") == true);
  CHECK(inif.contains("中文节", "中文键") == true);
  CHECK(inif["中文节"]["中文键"].as<std::string>() == "中文值");

  // 修改中文值
  inif["中文节"]["中文键"] = "新的中文值";
  CHECK(inif["中文节"]["中文键"].as<std::string>() == "新的中文值");

  // 测试不存在的中文键
  CHECK(inif.contains("中文节", "不存在的键") == false);

  // 测试中文默认值
  CHECK(inif.get("中文节", "不存在的键", "默认值").as<std::string>() == "默认值");
}

struct Point
{
  int x, y;
  friend std::ostream &operator<<(std::ostream &os, const Point &p)
  {
    return os << "(" << p.x << "," << p.y << ")";
  }
};

TEST_CASE("join function", "[join]")
{
  using namespace ini;
  using namespace std;

  SECTION("Join vector of int")
  {
    vector<int> vec = {1, 2, 3};
    REQUIRE(join(vec, ",") == "1,2,3");
    REQUIRE(join(vec, '-') == "1-2-3");
  }

  SECTION("Join list of float")
  {
    list<float> lst = {1.1f, 2.2f, 3.3f};
    REQUIRE(join(lst, ", ") == "1.1, 2.2, 3.3");
  }

  SECTION("Join set of double")
  {
    set<double> s = {3.14, 2.71};
    REQUIRE(join(s, " | ") == "2.71 | 3.14");  // 注意 set 是有序的
  }

  SECTION("Join vector of strings")
  {
    vector<string> words = {"hello", "world", "!"};
    REQUIRE(join(words, " ") == "hello world !");
    REQUIRE(join(words, '_') == "hello_world_!");
  }

  SECTION("Join array of integers")
  {
    array<int, 4> arr = {10, 20, 30, 40};
    REQUIRE(join(arr, ",") == "10,20,30,40");
  }

  SECTION("Join deque of strings")
  {
    deque<string> dq = {"first", "second"};
    REQUIRE(join(dq, ":") == "first:second");
  }

  SECTION("Join vector of single element")
  {
    vector<int> one = {42};
    REQUIRE(join(one, ',') == "42");
  }

  SECTION("Join empty container")
  {
    vector<string> empty;
    REQUIRE(join(empty, ",") == "");
  }

  SECTION("Join vector of Chinese strings")
  {
    vector<string> chinese = {"你好", "世界", "！"};
    REQUIRE(join(chinese, "-") == "你好-世界-！");
  }

  SECTION("Join vector of mixed-length strings")
  {
    vector<string> mixed = {"short", "", "longer text", "123"};
    REQUIRE(join(mixed, "|") == "short||longer text|123");
  }

  SECTION("Join set of chars")
  {
    set<char> chars = {'a', 'b', 'c'};
    REQUIRE(join(chars, ',') == "a,b,c");
  }

  SECTION("Join array of doubles")
  {
    array<double, 3> arr = {0.1, 2.5, 3.14159};
    REQUIRE(join(arr, ";") == "0.1;2.5;3.14159");
  }

  SECTION("Join vector of bool")
  {
    vector<bool> flags = {true, false, true};
    REQUIRE(join(flags, ',') == "1,0,1");  // vector<bool> 特殊处理为位
  }

  SECTION("Join forward_list of integers")
  {
    forward_list<int> fl = {5, 10, 15};
    REQUIRE(join(fl, "->") == "5->10->15");
  }

  SECTION("Join multiset of strings")
  {
    multiset<string> ms = {"apple", "banana", "apple"};
    REQUIRE(join(ms, ",") == "apple,apple,banana");  // multiset 自动排序 + 重复
  }

  // SECTION("Join vector of nullptrs") Error in join function: Container elements cannot be of pointer type
  // {
  //   vector<const char*> ptrs = {"test", "text", nullptr};
  //   std::ostringstream expected;
  //   expected << ptrs[0] << "," << ptrs[1] << "," << ptrs[2];  // 输出地址或null
  //   REQUIRE(join(ptrs, ',') == expected.str());
  // }

  SECTION("Join vector of user-defined type")
  {
    vector<Point> points = {{1, 2}, {3, 4}, {5, 6}};
    REQUIRE(join(points, ';') == "(1,2);(3,4);(5,6)");
  }

  SECTION("Join with empty string separator")
  {
    vector<int> nums = {1, 2, 3};
    REQUIRE(join(nums, "") == "123");
  }

  SECTION("Join of single string element with empty separator")
  {
    vector<string> s = {"abc"};
    REQUIRE(join(s, "") == "abc");
  }

  SECTION("Join list of large numbers")
  {
    list<long long> big = {
      1000000000LL,          // 1_000_000_000
      9223372036854775807LL  // 9_223_372_036_854_775_807
    };
    REQUIRE(join(big, ",") == "1000000000,9223372036854775807");
  }
}

TEST_CASE("member func test12 - massive insertions and edge cases", "[inifile][stress]")
{
  ini::inifile inif;

  // 1. 插入 100 个 section，每个 section 包含 50 个键值对
  for (int i = 0; i < 100; ++i)
  {
    std::string section = "section_" + std::to_string(i);
    for (int j = 0; j < 50; ++j)
    {
      std::string key = "key_" + std::to_string(j);
      inif[section][key] = i * 100 + j;
    }
  }

  // 2. 检查部分 section 和 key 的存在性
  CHECK(inif.contains("section_0", "key_0"));
  CHECK(inif.contains("section_99", "key_49"));
  CHECK_FALSE(inif.contains("section_100"));            // 不存在的 section
  CHECK_FALSE(inif.contains("section_10", "key_100"));  // 不存在的 key

  // 3. 边界键名和值测试
  inif[""][""] = "";  // 空 section + 空 key
  inif[""]["key_only"] = "value";
  inif["special"]["空格 key"] = "空格 value";
  inif["special"]["中文key"] = "中文值";
  inif["special"]["newline\nkey"] = "value\nwith\nnewlines";
  inif["special"]["=equals="] = "==value==";

  // 4. 类型覆盖测试（先设置为 bool，再覆盖为 string）
  inif["overload"]["data"] = true;
  CHECK(bool(inif["overload"]["data"]) == true);
  inif["overload"]["data"] = "now string";
  CHECK(inif["overload"]["data"].as<std::string>() == "now string");

  // 5. 注释设置和清除
  inif["commented"]["item1"] = 123;
  inif["commented"].set_comment("这是 section 的注释");
  inif["commented"]["item1"].set_comment("item1 的注释");

  inif["commented"].clear_comment();
  inif["commented"]["item1"].clear_comment();

  // 6. 检查 size/count/empty/find
  CHECK(inif.size() >= 100);
  CHECK(inif.count("section_42") == 1);
  CHECK(inif["section_42"].size() == 50);
  CHECK(inif[""][""].as<std::string>() == "");

  // 7. 保存并加载验证（保存后再 load 并比对）
  const char *path = "./test_massive.ini";
  REQUIRE_NOTHROW(inif.save(path));

  ini::inifile loaded;
  REQUIRE_NOTHROW(loaded.load(path));

  CHECK(loaded.size() == inif.size());
  CHECK(loaded["section_1"]["key_1"].as<int>() == 101);
  CHECK(loaded["special"]["中文key"].as<std::string>() == "中文值");
  CHECK(loaded[""]["key_only"].as<std::string>() == "value");
  CHECK(loaded.contains("section_99", "key_49"));
}

TEST_CASE("member func test13 - extreme stress and edge cases", "[inifile][extreme]")
{
  ini::inifile inif;

  // 1. 插入 1000 个 section，每个插入 100 个 key-value（含特殊值）
  for (int i = 0; i < 1000; ++i)
  {
    std::string section = "sec_" + std::to_string(i);
    for (int j = 0; j < 100; ++j)
    {
      std::string key = "key_" + std::to_string(j);
      std::string value = "val_" + std::to_string(i) + "_" + std::to_string(j);
      inif[section][key] = value;

      // 多次覆盖
      inif[section][key] = value + "_updated";
      inif[section][key] = value + "_final";
    }
  }

  // 2. 空 section 和空 key/value
  inif[""][""] = "";
  inif[""]["empty_key"] = "";
  inif[""]["空值"] = "    ";
  // inif[""]["#special_key"] = "#this is not a comment";  // 不允许#开头的 key, 会被当作注释

  // 3. 含控制字符、特殊符号、换行、回车
  inif["strange"]["中文"] = "支持中文值";
  inif["strange"]["emoji🚀"] = "值🌟";

  // 3.1 合法但极端的 Key 和 Value（不使用换行）
  inif["strange"]["with_space"] = "value with space";
  inif["strange"]["with_special_!@#$%^&*()"] = "~!@#￥%……&*（）——";

  // 3.2 中文、emoji：仅在 UTF-8 环境下允许
  inif["strange"]["中文key"] = "中文值";
  inif["strange"]["emoji_key_🚀"] = "emoji_value_🌟";

  // 3.3 将不可见字符放入 value 中以测试 parser（需转义或预期失败）
  std::string encoded_ctrl_value = "line\\nbreak\\tand\\x07bell";  // 以编码字符串替代原始控制符
  inif["strange"]["encoded_control"] = encoded_ctrl_value;

  // 4. 超长 key/value
  std::string long_key(1024, 'K');
  std::string long_val(8192, 'V');
  inif["long"]["long_key"] = long_val;
  inif["long"][long_key] = "短值";

  // 5. 数值边界情况
  inif["numbers"]["int_max"] = std::numeric_limits<int>::max();
  inif["numbers"]["int_min"] = std::numeric_limits<int>::min();
  inif["numbers"]["double_max"] = std::numeric_limits<double>::max();
  inif["numbers"]["double_min"] = std::numeric_limits<double>::min();
  inif["numbers"]["neg_zero"] = -0.0;
  inif["numbers"]["inf"] = std::numeric_limits<double>::infinity();
  inif["numbers"]["nan"] = std::numeric_limits<double>::quiet_NaN();

  // 6. 重复 key，重复 section
  for (int i = 0; i < 10; ++i)
  {
    inif["dup_section"]["dup_key"] = i;
  }
  CHECK(inif["dup_section"]["dup_key"].as<int>() == 9);  // 最后一个覆盖

  // 7. 注释多次覆盖
  auto &sec = inif["annotated"];
  sec.set_comment("初始注释");
  sec.set_comment("覆盖注释");
  sec.clear_comment();
  sec.set_comment("最终注释");

  // 8. 保存并再次加载，验证一致性
  const char *path = "./test_extreme.ini";
  inif.save(path);

  ini::inifile reloaded;
  reloaded.load(path);

  // 校验部分加载后数据一致
  CHECK(reloaded["sec_999"]["key_99"].as<std::string>() == "val_999_99_final");
  CHECK(reloaded[""][""].as<std::string>() == "");
  // CHECK(reloaded[""]["#special_key"].as<std::string>() == "#this is not a comment");
  CHECK(reloaded["long"]["long_key"].as<std::string>() == long_val);
  CHECK(reloaded["long"][long_key].as<std::string>() == "短值");
  CHECK(reloaded["strange"]["emoji🚀"].as<std::string>() == "值🌟");
  CHECK(reloaded["strange"]["with_special_!@#$%^&*()"].as<std::string>() == "~!@#￥%……&*（）——");  // 极端
  CHECK(reloaded["numbers"]["int_max"].as<int>() == std::numeric_limits<int>::max());

  // 不一定能比较 NaN，但可以判断不是 inf
  auto nan_val = reloaded["numbers"]["nan"].as<double>();
  CHECK(nan_val != nan_val);  // NaN 特性：不等于自身

  // 9. 更加密集的校验（不需要全校验，但随机抽查）
  CHECK(reloaded.contains("sec_0", "key_0"));
  CHECK(reloaded.contains("sec_500", "key_50"));
  CHECK(reloaded["sec_123"]["key_45"].as<std::string>() == "val_123_45_final");
  CHECK(reloaded["sec_0"]["key_0"].as<std::string>() == "val_0_0_final");

  // 10. 空字符串和空 section 检查
  CHECK(reloaded.contains("", ""));
  CHECK(reloaded.contains("", "empty_key"));
  CHECK(reloaded.contains("", "空值"));
  CHECK(reloaded[""]["空值"].as<std::string>() == "");  // 被trim了

  // 11. 中文、emoji 内容完整性验证
  CHECK(reloaded["strange"]["中文"].as<std::string>() == "支持中文值");
  CHECK(reloaded["strange"]["emoji_key_🚀"].as<std::string>() == "emoji_value_🌟");
  CHECK(reloaded["strange"]["中文key"].as<std::string>() == "中文值");

  // 12. encoded 控制字符字符串正确保留
  CHECK(reloaded["strange"]["encoded_control"].as<std::string>() == encoded_ctrl_value);

  // 13. 注释是否保留（若支持注释加载）
  // CHECK(reloaded["annotated"].comment() == "最终注释");  // 如果实现支持 comment()

  // 14. 数值内容
  CHECK(reloaded["numbers"]["double_max"].as<double>() == std::numeric_limits<double>::max());
  CHECK(reloaded["numbers"]["double_min"].as<double>() == std::numeric_limits<double>::min());
  CHECK(std::signbit(reloaded["numbers"]["neg_zero"].as<double>()) == true);  // 确保是 -0.0
  CHECK(std::isinf(reloaded["numbers"]["inf"].as<double>()) == true); // 不通过
  CHECK(std::isnan(reloaded["numbers"]["nan"].as<double>()) == true);

  // 15. 重复 section 的最终值覆盖验证
  CHECK(reloaded["dup_section"]["dup_key"].as<std::string>() == "9");

  // 16. 超长 key/value 检查
  CHECK(reloaded["long"].contains("long_key"));
  CHECK(reloaded["long"].contains(long_key));
  CHECK(reloaded["long"]["long_key"].as<std::string>().size() == 8192);
  CHECK(reloaded["long"][long_key].as<std::string>() == "短值");

  // 17. section 和 key 总量检查（应有1000+1+1+1+1+1 = 1005个section）
  CHECK(reloaded.size() >= 1000);
  CHECK(reloaded.count("sec_0") == 1);
  CHECK(reloaded["sec_0"].size() == 100);

  // 18. 极端符号内容
  CHECK(reloaded["strange"]["with_special_!@#$%^&*()"].as<std::string>() == "~!@#￥%……&*（）——");
  CHECK(reloaded["strange"]["with_space"].as<std::string>() == "value with space");
}
