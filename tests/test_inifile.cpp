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

TEST_CASE("dynamic allocation stress test - memory leak check", "[inifile][dynamic]")
{
  // 动态分配 inifile 实例
  ini::inifile *inif = new ini::inifile();

  // 动态创建 100 个 section，每个 section 插入多个键值
  for (int s = 0; s < 100; ++s)
  {
    std::string section_name = "section_" + std::to_string(s);
    ini::section &sec = (*inif)[section_name];

    // 插入各种形式的 key-value，部分使用 field API
    for (int k = 0; k < 50; ++k)
    {
      std::string key = "key_" + std::to_string(k);
      std::string value = "value_" + std::to_string(s) + "_" + std::to_string(k);

      ini::field &fld = sec[key];
      fld = value;

      // 设置注释并修改值
      fld.set_comment("comment for " + key);
      fld = value + "_modified";
    }

    // 测试 section 的注释处理
    sec.set_comment("section comment");
  }

  // 添加空 section、空 key、空值、特殊字符
  (*inif)[""][""] = "";
  (*inif)[""]["\t\n\r"] = "  \t ";
  (*inif)["specialchars"]["💡🚀"] = "emoji✅";
  (*inif)["specialchars"]["!@#$%^&*()"] = "<symbols>";

  // 设置重复覆盖，field 的多次赋值
  auto &fld = (*inif)["repeat"]["key"];
  fld = "first";
  fld = "second";
  fld = "final";

  // 保存再读取（内存行为测试重点在前面，此处为完整性验证）
  const char *path = "./test_dynamic_memory.ini";
  inif->save(path);

  // 可选：删除原始对象后重新加载看是否崩溃
  delete inif;
  inif = nullptr;

  ini::inifile *reloaded = new ini::inifile();
  reloaded->load(path);

  CHECK(reloaded->contains("section_0", "key_0"));
  CHECK((*reloaded)["repeat"]["key"].as<std::string>() == "final");
  CHECK((*reloaded)["specialchars"]["💡🚀"].as<std::string>() == "emoji✅");

  // 释放内存，观察是否泄漏（结合 valgrind/VS 工具）
  delete reloaded;
}

TEST_CASE("dynamic allocation stress test - memory leak check new", "[inifile][dynamic][new]")
{
  ini::inifile *inif = new ini::inifile();
  ini::comment *cmt = new ini::comment();
  ini::field *fld = new ini::field();
  ini::section *sec = new ini::section();

  delete inif;
  delete cmt;
  delete fld;
  delete sec;
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

TEST_CASE("section keys, values, and items test", "[section][keys][values][items]")
{
  ini::inifile inif;

  // 设置字段
  inif["General"]["version"] = "1.2.3";
  inif["General"]["author"] = "Abin";
  inif["General"]["license"] = "MIT";

  const auto &section = inif["General"];

  // 测试 keys()
  auto keys = section.keys();
  CHECK(keys.size() == 3);
  CHECK(std::find(keys.begin(), keys.end(), "version") != keys.end());
  CHECK(std::find(keys.begin(), keys.end(), "author") != keys.end());
  CHECK(std::find(keys.begin(), keys.end(), "license") != keys.end());

  // 测试 values()
  auto values = section.values();
  CHECK(values.size() == 3);

  bool has_version_value = false, has_author_value = false, has_license_value = false;
  for (const auto &val : values)
  {
    if (val.as<std::string>() == "1.2.3")
      has_version_value = true;
    else if (val.as<std::string>() == "Abin")
      has_author_value = true;
    else if (val.as<std::string>() == "MIT")
      has_license_value = true;
  }
  CHECK(has_version_value);
  CHECK(has_author_value);
  CHECK(has_license_value);

  // 测试 items()
  auto items = section.items();
  CHECK(items.size() == 3);

  bool has_version = false, has_author = false, has_license = false;
  for (const auto &item : items)
  {
    if (item.first == "version" && item.second.as<std::string>() == "1.2.3")
      has_version = true;
    else if (item.first == "author" && item.second.as<std::string>() == "Abin")
      has_author = true;
    else if (item.first == "license" && item.second.as<std::string>() == "MIT")
      has_license = true;
  }
  CHECK(has_version);
  CHECK(has_author);
  CHECK(has_license);

  // 空 section 测试
  inif["Empty"];
  auto empty_keys = inif["Empty"].keys();
  auto empty_values = inif["Empty"].values();
  auto empty_items = inif["Empty"].items();

  CHECK(empty_keys.empty());
  CHECK(empty_values.empty());
  CHECK(empty_items.empty());
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
  inif["special"]["negative_int"] = -2147483648LL;

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

  // 14. 数值内容
  CHECK(reloaded["numbers"]["double_max"].as<double>() == std::numeric_limits<double>::max());
  CHECK(reloaded["numbers"]["double_min"].as<double>() == std::numeric_limits<double>::min());
  CHECK(std::signbit(reloaded["numbers"]["neg_zero"].as<double>()) == true);  // 确保是 -0.0
  CHECK(std::isinf(reloaded["numbers"]["inf"].as<double>()) == true);
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

TEST_CASE("Floating Point Values in iniFile", "[inifile][floating_point][boundary]")
{
  ini::inifile inif;

  // 1. 正常的浮点数
  inif["normal"]["pi"] = 3.141592653589793;
  inif["normal"]["e"] = 2.718281828459045;
  CHECK(inif["normal"]["pi"].as<double>() == Approx(3.141592653589793).epsilon(1e-15));
  CHECK(inif["normal"]["e"].as<double>() == Approx(2.718281828459045).epsilon(1e-15));

  // 2. 特殊值：inf, -inf, nan, -nan
  inif["special"]["inf"] = std::numeric_limits<double>::infinity();
  inif["special"]["-inf"] = -std::numeric_limits<double>::infinity();
  inif["special"]["nan"] = std::numeric_limits<double>::quiet_NaN();
  inif["special"]["-nan"] = -std::numeric_limits<double>::quiet_NaN();
  CHECK(inif["special"]["inf"].as<double>() == std::numeric_limits<double>::infinity());
  CHECK(inif["special"]["-inf"].as<double>() == -std::numeric_limits<double>::infinity());
  CHECK(std::isnan(inif["special"]["nan"].as<double>()));
  CHECK(std::isnan(inif["special"]["-nan"].as<double>()));

  // 3. 最大/最小浮点值
  inif["boundary"]["double_max"] = std::numeric_limits<double>::max();
  inif["boundary"]["double_min"] = std::numeric_limits<double>::min();
  inif["boundary"]["float_max"] = std::numeric_limits<float>::max();
  inif["boundary"]["float_min"] = std::numeric_limits<float>::min();
  CHECK(inif["boundary"]["double_max"].as<double>() == std::numeric_limits<double>::max());
  CHECK(inif["boundary"]["double_min"].as<double>() == std::numeric_limits<double>::min());
  CHECK(inif["boundary"]["float_max"].as<float>() == std::numeric_limits<float>::max());
  CHECK(inif["boundary"]["float_min"].as<float>() == std::numeric_limits<float>::min());

  // 4. 负零（-0.0）
  inif["special"]["neg_zero"] = -0.0;
  CHECK(std::signbit(inif["special"]["neg_zero"].as<double>()) == true);  // 确保是 -0.

  // 6. 浮点数的大数值，验证字符串转换的精度
  inif["large"]["big_number"] = 12345678901234567890.1234567890;
  CHECK(inif["large"]["big_number"].as<double>() == Approx(12345678901234567890.1234567890).epsilon(1e-10));

  // 7. 浮点数写入和读取的一致性验证
  double input_value = 3.14159;
  inif["test"]["pi_value"] = input_value;
  double output_value = inif["test"]["pi_value"].as<double>();
  CHECK(output_value == Approx(input_value).epsilon(1e-5));

  // 8. 长度较长的浮点数值，验证能否正确处理
  inif["long"]["long_value"] = 1.2345678901234567e+308;  // double最大值
  CHECK(inif["long"]["long_value"].as<double>() == Approx(1.2345678901234567e+308));

  // 9. 浮点数的空字符串转换，预期抛出异常
  try
  {
    inif["empty"]["empty_value"] = "";
    // 尝试解析空字符串，应该抛出异常
    inif["empty"]["empty_value"].as<double>();
    // 如果没有抛出异常，则测试失败
    CHECK(false);  // 强制测试失败
  }
  catch (const std::invalid_argument &e)
  {
    // 检查异常消息是否正确
    CHECK(std::string(e.what()) == "[inifile] error: Cannot convert empty string to floating-point: \"\"");
  }
  catch (...)
  {
    // 捕获其他类型的异常，测试失败
    CHECK(false);
  }

  // 10. 各种浮点数格式和数值的转换
  inif["formats"]["small"] = 0.000000123456789;
  inif["formats"]["scientific"] = 1.23e10;
  inif["formats"]["percentage"] = 45.67;
  CHECK(inif["formats"]["small"].as<double>() == Approx(0.000000123456789).epsilon(1e-18));
  CHECK(inif["formats"]["scientific"].as<double>() == Approx(1.23e10).epsilon(1e-5));
  CHECK(inif["formats"]["percentage"].as<double>() == Approx(45.67).epsilon(1e-2));

  // 11. 负数和零的校验
  inif["numbers"]["negative_number"] = -123.456;
  inif["numbers"]["zero_value"] = 0.0;
  CHECK(inif["numbers"]["negative_number"].as<double>() == -123.456);
  CHECK(inif["numbers"]["zero_value"].as<double>() == 0.0);

  // 12. 重复键的覆盖，检查最后一个赋值生效
  inif["dup"]["value"] = 3.14;
  inif["dup"]["value"] = 2.71;
  CHECK(inif["dup"]["value"].as<double>() == Approx(2.71).epsilon(1e-5));

  // 13. 注释的处理
  inif["comments"]["with_comment"] = 3.14159;
  inif["comments"].set_comment("This is Pi");

  // 14. 文件保存和加载，验证一致性
  const char *path = "./test_floating_point.ini";
  inif.save(path);

  ini::inifile reloaded;
  reloaded.load(path);

  // 检查重载后的数据一致性
  CHECK(reloaded["normal"]["pi"].as<double>() == Approx(3.141592653589793).epsilon(1e-15));
  CHECK(reloaded["normal"]["e"].as<double>() == Approx(2.718281828459045).epsilon(1e-15));
  CHECK(reloaded["special"]["inf"].as<double>() == std::numeric_limits<double>::infinity());
  CHECK(reloaded["special"]["-inf"].as<double>() == -std::numeric_limits<double>::infinity());
  CHECK(std::isnan(reloaded["special"]["nan"].as<double>()));
  CHECK(std::isnan(reloaded["special"]["-nan"].as<double>()));
  CHECK(reloaded["boundary"]["double_max"].as<double>() == std::numeric_limits<double>::max());
  CHECK(reloaded["boundary"]["double_min"].as<double>() == std::numeric_limits<double>::min());
  CHECK(reloaded["boundary"]["float_max"].as<float>() == std::numeric_limits<float>::max());
  CHECK(reloaded["boundary"]["float_min"].as<float>() == std::numeric_limits<float>::min());
  CHECK(std::signbit(reloaded["special"]["neg_zero"].as<double>()) == true);  // 确保是 -0.0
  CHECK(reloaded["large"]["big_number"].as<double>() == Approx(12345678901234567890.1234567890).epsilon(1e-10));
  CHECK(reloaded["formats"]["small"].as<double>() == Approx(0.000000123456789).epsilon(1e-18));
  CHECK(reloaded["formats"]["scientific"].as<double>() == Approx(1.23e10).epsilon(1e-5));
  CHECK(reloaded["formats"]["percentage"].as<double>() == Approx(45.67).epsilon(1e-2));
  CHECK(reloaded["numbers"]["negative_number"].as<double>() == -123.456);
  CHECK(reloaded["numbers"]["zero_value"].as<double>() == 0.0);
  CHECK(reloaded["dup"]["value"].as<double>() == Approx(2.71).epsilon(1e-5));
}

TEST_CASE("Integer Values in iniFile", "[inifile][integer][boundary]")
{
  ini::inifile inif;

  // 1. 正常的整数
  inif["normal"]["positive_int"] = 123456789;
  inif["normal"]["negative_int"] = -987654321;
  CHECK(inif["normal"]["positive_int"].as<int>() == 123456789);
  CHECK(inif["normal"]["negative_int"].as<int>() == -987654321);

  // 2. 负零（-0）验证
  inif["special"]["neg_zero"] = -0;
  CHECK(inif["special"]["neg_zero"].as<int>() == 0);  // -0 转换为 0

  // 3. 最大/最小整数值
  inif["boundary"]["int_max"] = std::numeric_limits<int>::max();
  inif["boundary"]["int_min"] = std::numeric_limits<int>::min();
  inif["boundary"]["long_max"] = std::numeric_limits<long>::max();
  inif["boundary"]["long_min"] = std::numeric_limits<long>::min();
  CHECK(inif["boundary"]["int_max"].as<int>() == std::numeric_limits<int>::max());
  CHECK(inif["boundary"]["int_min"].as<int>() == std::numeric_limits<int>::min());
  CHECK(inif["boundary"]["long_max"].as<long>() == std::numeric_limits<long>::max());
  CHECK(inif["boundary"]["long_min"].as<long>() == std::numeric_limits<long>::min());

  // 4. 边界测试：非常大的整数
  inif["large"]["big_number"] = 1234567890123456789LL;  // long long
  CHECK(inif["large"]["big_number"].as<long long>() == 1234567890123456789LL);

  // 5. 空字符串转换，预期抛出异常
  try
  {
    inif["empty"]["empty_value"] = "";
    // 尝试解析空字符串，应该抛出异常
    inif["empty"]["empty_value"].as<int>();
    CHECK(false);  // 强制测试失败
  }
  catch (const std::invalid_argument &e)
  {
    // 检查异常消息是否正确
    CHECK(std::string(e.what()) == "[inifile] error: Cannot convert empty string to integer: \"\"");
  }
  catch (...)
  {
    // 捕获其他类型的异常，测试失败
    CHECK(false);
  }

  // 6. 非法整数格式（字母或特殊字符）
  try
  {
    inif["invalid"]["invalid_value"] = "abc123";
    inif["invalid"]["invalid_value"].as<int>();
    CHECK(false);  // 强制测试失败
  }
  catch (const std::invalid_argument &e)
  {
    // 检查异常消息是否正确
    CHECK(std::string(e.what()) == "[inifile] error: Invalid integer format: \"abc123\"");
  }

  // 7. 重复键的覆盖，检查最后一个赋值生效
  inif["dup"]["value"] = 42;
  inif["dup"]["value"] = 99;
  CHECK(inif["dup"]["value"].as<int>() == 99);

  // 8. 负数整数值验证
  inif["numbers"]["negative_number"] = -123456;
  inif["numbers"]["zero_value"] = 0;
  CHECK(inif["numbers"]["negative_number"].as<int>() == -123456);
  CHECK(inif["numbers"]["zero_value"].as<int>() == 0);

  // 9. 整数写入和读取的一致性验证
  int input_value = 98765;
  inif["test"]["int_value"] = input_value;
  int output_value = inif["test"]["int_value"].as<int>();
  CHECK(output_value == input_value);

  // 10. 负整数测试
  inif["special"]["negative_int"] = -2147483648LL;
  CHECK(inif["special"]["negative_int"].as<int>() == -2147483648);

  // 11. 数字格式验证：科学计数法
  inif["formats"]["scientific"] = 1e6;  // 科学计数法
  CHECK(inif["formats"]["scientific"].as<int>() == 1000000);

  // 12. 边界条件：大于 int 最大值的 long 转换
  try
  {
    inif["too_large"]["big_value"] = "2147483648";  // int 最大值加1
    inif["too_large"]["big_value"].as<int>();
    CHECK(false);  // 强制测试失败
  }
  catch (const std::out_of_range &e)
  {
    // 检查溢出异常
    CHECK((std::string(e.what()).find("out of range") != std::string::npos));
  }

  // 13. 文件保存和加载，验证一致性
  const char *path = "./test_integer_values.ini";
  inif.save(path);

  ini::inifile reloaded;
  reloaded.load(path);

  // 检查重载后的数据一致性
  CHECK(reloaded["normal"]["positive_int"].as<int>() == 123456789);
  CHECK(reloaded["normal"]["negative_int"].as<int>() == -987654321);
  CHECK(reloaded["boundary"]["int_max"].as<int>() == std::numeric_limits<int>::max());
  CHECK(reloaded["boundary"]["int_min"].as<int>() == std::numeric_limits<int>::min());
  CHECK(reloaded["large"]["big_number"].as<long long>() == 1234567890123456789LL);
  CHECK(reloaded["dup"]["value"].as<int>() == 99);
  CHECK(reloaded["numbers"]["negative_number"].as<int>() == -123456);
  CHECK(reloaded["numbers"]["zero_value"].as<int>() == 0);
}

TEST_CASE("2 Integer Values in iniFile", "[inifile][integer][boundary]")
{
  ini::inifile inif;

  SECTION("Standard signed integers")
  {
    inif["signed"]["int"] = int(-12345);
    inif["signed"]["short"] = short(-1234);
    inif["signed"]["long"] = long(-123456789L);
    inif["signed"]["long_long"] = (long long)(-123456789012345LL);
    inif["signed"]["int8"] = int8_t(-128);
    inif["signed"]["int16"] = int16_t(-32768);
    inif["signed"]["int32"] = int32_t(-2147483647);
    inif["signed"]["int64"] = int64_t(-9223372036854775807LL);

    CHECK(inif["signed"]["int"].as<int>() == -12345);
    CHECK(inif["signed"]["short"].as<short>() == -1234);
    CHECK(inif["signed"]["long"].as<long>() == -123456789L);
    CHECK(inif["signed"]["long_long"].as<long long>() == -123456789012345LL);
    CHECK(inif["signed"]["int8"].as<int8_t>() == -128);
    CHECK(inif["signed"]["int16"].as<int16_t>() == -32768);
    CHECK(inif["signed"]["int32"].as<int32_t>() == -2147483647);
    CHECK(inif["signed"]["int64"].as<int64_t>() == -9223372036854775807LL);
  }

  SECTION("Standard unsigned integers")
  {
    inif["unsigned"]["uint"] = unsigned(12345);
    inif["unsigned"]["ushort"] = (unsigned short)(1234);
    inif["unsigned"]["ulong"] = (unsigned long)(123456789UL);
    inif["unsigned"]["ulong_long"] = (unsigned long long)(123456789012345ULL);
    inif["unsigned"]["uint8"] = uint8_t(255);
    inif["unsigned"]["uint16"] = uint16_t(65535);
    inif["unsigned"]["uint32"] = uint32_t(4294967295U);
    inif["unsigned"]["uint64"] = uint64_t(18446744073709551615ULL);

    CHECK(inif["unsigned"]["uint"].as<unsigned>() == 12345);
    CHECK(inif["unsigned"]["ushort"].as<unsigned short>() == 1234);
    CHECK(inif["unsigned"]["ulong"].as<unsigned long>() == 123456789UL);
    CHECK(inif["unsigned"]["ulong_long"].as<unsigned long long>() == 123456789012345ULL);
    CHECK(inif["unsigned"]["uint8"].as<uint8_t>() == 255);
    CHECK(inif["unsigned"]["uint16"].as<uint16_t>() == 65535);
    CHECK(inif["unsigned"]["uint32"].as<uint32_t>() == 4294967295U);
    CHECK(inif["unsigned"]["uint64"].as<uint64_t>() == 18446744073709551615ULL);
  }

  SECTION("Edge cases and errors")
  {
    // 空字符串
    inif["error"]["empty"] = "";
    CHECK_THROWS_AS(inif["error"]["empty"].as<int>(), std::invalid_argument);

    // 非法格式
    inif["error"]["not_a_number"] = "abc123";
    CHECK_THROWS_AS(inif["error"]["not_a_number"].as<int>(), std::invalid_argument);

    // 溢出
    inif["error"]["too_big"] = "999999999999999999999999";
    CHECK_THROWS_AS(inif["error"]["too_big"].as<int64_t>(), std::out_of_range);
  }

  SECTION("Overwriting value")
  {
    inif["overwrite"]["x"] = 123;
    inif["overwrite"]["x"] = 456;
    CHECK(inif["overwrite"]["x"].as<int>() == 456);
  }

  SECTION("Write and reload file")
  {
    inif["persist"]["a"] = 42;
    inif["persist"]["b"] = -99;

    const char *path = "./test_integer_values.ini";
    inif.save(path);

    ini::inifile reloaded;
    reloaded.load(path);

    CHECK(reloaded["persist"]["a"].as<int>() == 42);
    CHECK(reloaded["persist"]["b"].as<int>() == -99);
  }
}

TEMPLATE_TEST_CASE("Integer value conversion in ini::inifile", "[inifile][convert][int]", int, short, long, long long,
                   unsigned int, unsigned short, unsigned long, unsigned long long, int8_t, int16_t, int32_t, int64_t,
                   uint8_t, uint16_t, uint32_t, uint64_t)
{
  using T = TestType;
  ini::inifile inif;

  const T min_value = std::numeric_limits<T>::lowest();
  const T max_value = std::numeric_limits<T>::max();
  const T zero = 0;
  const T pos = T(123);
  const T neg = std::is_signed<T>::value ? T(-456) : T(0);  // for unsigned, use 0

  inif["val"]["zero"] = zero;
  inif["val"]["pos"] = pos;
  inif["val"]["min"] = min_value;
  inif["val"]["max"] = max_value;
  if (std::is_signed<T>::value)
  {
    inif["val"]["neg"] = neg;
  }

  CHECK(inif["val"]["zero"].as<T>() == zero);
  CHECK(inif["val"]["pos"].as<T>() == pos);
  CHECK(inif["val"]["min"].as<T>() == min_value);
  CHECK(inif["val"]["max"].as<T>() == max_value);
  if (std::is_signed<T>::value)
  {
    CHECK(inif["val"]["neg"].as<T>() == neg);
  }
}

TEMPLATE_TEST_CASE("Integer edge ±1 conversion", "[inifile][convert][int][edge]", int, short, long, long long,
                   unsigned int, unsigned short, unsigned long, unsigned long long, int8_t, int16_t, int32_t, int64_t,
                   uint8_t, uint16_t, uint32_t, uint64_t)
{
  using T = TestType;
  ini::inifile inif;

  const T max_minus_1 = std::numeric_limits<T>::max() - T(1);
  const T min_plus_1 = std::numeric_limits<T>::lowest() + T(1);

  inif["val"]["max-1"] = max_minus_1;
  CHECK(inif["val"]["max-1"].as<T>() == max_minus_1);

  if (std::is_signed<T>::value)
  {
    inif["val"]["min+1"] = min_plus_1;
    CHECK(inif["val"]["min+1"].as<T>() == min_plus_1);
  }
}

TEMPLATE_TEST_CASE("Integer bit-boundary test", "[inifile][convert][bit][int]", int, short, long, long long,
                   unsigned int, unsigned short, unsigned long, unsigned long long, int8_t, int16_t, int32_t, int64_t,
                   uint8_t, uint16_t, uint32_t, uint64_t)
{
  using T = TestType;
  ini::inifile inif;

  constexpr int num_bits = sizeof(T) * 8;
  const T high_bit = T(1) << (num_bits - 1);  // highest bit set
  const T low_bit = 1;

  inif["val"]["low_bit"] = low_bit;
  CHECK(inif["val"]["low_bit"].as<T>() == low_bit);

  inif["val"]["high_bit"] = high_bit;
  CHECK(inif["val"]["high_bit"].as<T>() == high_bit);
}

TEMPLATE_TEST_CASE("Full-range integer conversion with min/lowest/max", "[inifile][convert][int][limits]", int, short,
                   long, long long, unsigned int, unsigned short, unsigned long, unsigned long long, int8_t, int16_t,
                   int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t)
{
  using T = TestType;
  ini::inifile inif;

  const T zero = 0;
  const T pos = T(123);
  const T neg = std::is_signed<T>::value ? T(-456) : T(0);

  const T min_val = std::numeric_limits<T>::min();        // 最小正数（有符号时为 1）
  const T lowest_val = std::numeric_limits<T>::lowest();  // 真正的最小值（如 -128）
  const T max_val = std::numeric_limits<T>::max();

  inif["val"]["zero"] = zero;
  inif["val"]["pos"] = pos;
  inif["val"]["min"] = min_val;
  inif["val"]["lowest"] = lowest_val;
  inif["val"]["max"] = max_val;
  if (std::is_signed<T>::value) inif["val"]["neg"] = neg;

  CHECK(inif["val"]["zero"].as<T>() == zero);
  CHECK(inif["val"]["pos"].as<T>() == pos);
  CHECK(inif["val"]["min"].as<T>() == min_val);
  CHECK(inif["val"]["lowest"].as<T>() == lowest_val);
  CHECK(inif["val"]["max"].as<T>() == max_val);
  if (std::is_signed<T>::value) CHECK(inif["val"]["neg"].as<T>() == neg);
}

TEST_CASE("basic_inifile move and assignment robustness", "[inifile][move][self-assignment]")
{
  ini::inifile original;
  original.set("section", "key", "value");

  SECTION("Self-assignment")
  {
    original = original;  // 正常的自赋值，不应崩溃或修改数据
    REQUIRE(original.contains("section"));
    REQUIRE(original.get("section", "key").as<std::string>() == "value");
  }

  SECTION("Self move-assignment (should be safe)")
  {
    ini::inifile &ref = original;
    ref = std::move(ref);  // 不推荐但应安全处理
    REQUIRE(ref.contains("section"));
    REQUIRE(ref.get("section", "key").as<std::string>() == "value");
  }

  SECTION("Move constructor clears original")
  {
    ini::inifile moved = std::move(original);
    REQUIRE(moved.contains("section"));
    REQUIRE(moved.get("section", "key").as<std::string>() == "value");

    // 检查原对象是否为空
    REQUIRE(original.empty());  // 如果你实现了 clear/move 后置空
  }

  SECTION("Move assignment clears original")
  {
    ini::inifile src;
    src.set("foo", "bar", "baz");

    ini::inifile target;
    target.set("x", "y", "z");

    target = std::move(src);

    REQUIRE(target.contains("foo"));
    REQUIRE(target.get("foo", "bar").as<std::string>() == "baz");

    // 原始 src 应为空
    REQUIRE(src.empty());
  }

  SECTION("Move into empty inifile")
  {
    ini::inifile empty;
    ini::inifile data;
    data.set("s", "k", "v");

    empty = std::move(data);
    REQUIRE(empty.contains("s"));
    REQUIRE(empty.get("s", "k").as<std::string>() == "v");
    REQUIRE(data.empty());
  }

  SECTION("Move from empty inifile")
  {
    ini::inifile empty;
    ini::inifile full;
    full.set("s", "k", "v");

    full = std::move(empty);
    REQUIRE(full.empty());  // 原内容应被清空
    REQUIRE(empty.empty());
  }
}

TEST_CASE("ini::section move and assignment robustness", "[section][move][self-assignment]")
{
  ini::section sec;
  sec.set("key1", "value1");

  SECTION("Self-assignment")
  {
    sec = sec;
    REQUIRE(sec.contains("key1"));
    REQUIRE(sec.get("key1").as<std::string>() == "value1");
  }

  SECTION("Self move-assignment")
  {
    ini::section &ref = sec;
    ref = std::move(ref);
    REQUIRE(ref.contains("key1"));
    REQUIRE(ref.get("key1").as<std::string>() == "value1");
  }

  SECTION("Move constructor clears source")
  {
    ini::section moved = std::move(sec);
    REQUIRE(moved.contains("key1"));
    REQUIRE(moved.get("key1").as<std::string>() == "value1");

    REQUIRE(sec.empty());
  }

  SECTION("Move assignment clears source")
  {
    ini::section src;
    src.set("k", "v");

    ini::section target;
    target.set("a", "b");

    target = std::move(src);
    REQUIRE(target.contains("k"));
    REQUIRE(target.get("k").as<std::string>() == "v");
    REQUIRE(src.empty());
  }
}

TEST_CASE("ini::field move and assignment robustness", "[field][move][self-assignment]")
{
  ini::field f1("some_value");

  SECTION("Self-assignment")
  {
    f1 = f1;
    REQUIRE(f1.as<std::string>() == "some_value");
  }

  SECTION("Self move-assignment")
  {
    ini::field &ref = f1;
    ref = std::move(ref);
    REQUIRE(ref.as<std::string>() == "some_value");
  }

  SECTION("Move constructor clears source")
  {
    ini::field f2 = std::move(f1);
    REQUIRE(f2.as<std::string>() == "some_value");

    // moved-from field 应该为空或默认状态
    REQUIRE(f1.empty());  // 假设 empty() 存在，或者检查 f1.as<std::string>() == ""
  }

  SECTION("Move assignment clears source")
  {
    ini::field src("abc");
    ini::field dst("xyz");

    dst = std::move(src);
    REQUIRE(dst.as<std::string>() == "abc");
    REQUIRE(src.empty());  // 如果实现清空
  }
}

TEST_CASE("IniFile Save/Load Consistency - All Integer Types")
{
  ini::inifile ini;
  ini["int"]["int_min"] = std::numeric_limits<int>::min();
  ini["int"]["int_max"] = std::numeric_limits<int>::max();
  ini["short"]["short_min"] = std::numeric_limits<short>::min();
  ini["short"]["short_max"] = std::numeric_limits<short>::max();
  ini["long"]["long_min"] = std::numeric_limits<long>::min();
  ini["long"]["long_max"] = std::numeric_limits<long>::max();
  ini["longlong"]["ll_min"] = std::numeric_limits<long long>::min();
  ini["longlong"]["ll_max"] = std::numeric_limits<long long>::max();

  ini["uint"]["uint_max"] = std::numeric_limits<unsigned int>::max();
  ini["ushort"]["ushort_max"] = std::numeric_limits<unsigned short>::max();
  ini["ulong"]["ulong_max"] = std::numeric_limits<unsigned long>::max();
  ini["ulonglong"]["ull_max"] = std::numeric_limits<unsigned long long>::max();

  // 有符号整数
  ini["i8"]["lowest"] = std::numeric_limits<int8_t>::lowest();
  ini["i8"]["max"] = std::numeric_limits<int8_t>::max();

  ini["i16"]["lowest"] = std::numeric_limits<int16_t>::lowest();
  ini["i16"]["max"] = std::numeric_limits<int16_t>::max();

  ini["i32"]["lowest"] = std::numeric_limits<int32_t>::lowest();
  ini["i32"]["max"] = std::numeric_limits<int32_t>::max();

  ini["i64"]["lowest"] = std::numeric_limits<int64_t>::lowest();
  ini["i64"]["max"] = std::numeric_limits<int64_t>::max();

  // 无符号整数
  ini["u8"]["lowest"] = std::numeric_limits<uint8_t>::lowest();  // = 0
  ini["u8"]["max"] = std::numeric_limits<uint8_t>::max();

  ini["u16"]["lowest"] = std::numeric_limits<uint16_t>::lowest();
  ini["u16"]["max"] = std::numeric_limits<uint16_t>::max();

  ini["u32"]["lowest"] = std::numeric_limits<uint32_t>::lowest();
  ini["u32"]["max"] = std::numeric_limits<uint32_t>::max();

  ini["u64"]["lowest"] = std::numeric_limits<uint64_t>::lowest();
  ini["u64"]["max"] = std::numeric_limits<uint64_t>::max();

  ini.save("test_integers.ini");

  ini::inifile loaded;
  loaded.load("test_integers.ini");

  CHECK(loaded["int"]["int_min"].as<int>() == std::numeric_limits<int>::min());
  CHECK(loaded["int"]["int_max"].as<int>() == std::numeric_limits<int>::max());
  CHECK(loaded["short"]["short_min"].as<short>() == std::numeric_limits<short>::min());
  CHECK(loaded["short"]["short_max"].as<short>() == std::numeric_limits<short>::max());
  CHECK(loaded["long"]["long_min"].as<long>() == std::numeric_limits<long>::min());
  CHECK(loaded["long"]["long_max"].as<long>() == std::numeric_limits<long>::max());
  CHECK(loaded["longlong"]["ll_min"].as<long long>() == std::numeric_limits<long long>::min());
  CHECK(loaded["longlong"]["ll_max"].as<long long>() == std::numeric_limits<long long>::max());

  CHECK(loaded["uint"]["uint_max"].as<unsigned int>() == std::numeric_limits<unsigned int>::max());
  CHECK(loaded["ushort"]["ushort_max"].as<unsigned short>() == std::numeric_limits<unsigned short>::max());
  CHECK(loaded["ulong"]["ulong_max"].as<unsigned long>() == std::numeric_limits<unsigned long>::max());
  CHECK(loaded["ulonglong"]["ull_max"].as<unsigned long long>() == std::numeric_limits<unsigned long long>::max());

  CHECK(loaded["i8"]["lowest"].as<int8_t>() == std::numeric_limits<int8_t>::lowest());
  CHECK(loaded["i8"]["max"].as<int8_t>() == std::numeric_limits<int8_t>::max());

  CHECK(loaded["i16"]["lowest"].as<int16_t>() == std::numeric_limits<int16_t>::lowest());
  CHECK(loaded["i16"]["max"].as<int16_t>() == std::numeric_limits<int16_t>::max());

  CHECK(loaded["i32"]["lowest"].as<int32_t>() == std::numeric_limits<int32_t>::lowest());
  CHECK(loaded["i32"]["max"].as<int32_t>() == std::numeric_limits<int32_t>::max());

  CHECK(loaded["i64"]["lowest"].as<int64_t>() == std::numeric_limits<int64_t>::lowest());
  CHECK(loaded["i64"]["max"].as<int64_t>() == std::numeric_limits<int64_t>::max());

  CHECK(loaded["u8"]["lowest"].as<uint8_t>() == std::numeric_limits<uint8_t>::lowest());
  CHECK(loaded["u8"]["max"].as<uint8_t>() == std::numeric_limits<uint8_t>::max());

  CHECK(loaded["u16"]["lowest"].as<uint16_t>() == std::numeric_limits<uint16_t>::lowest());
  CHECK(loaded["u16"]["max"].as<uint16_t>() == std::numeric_limits<uint16_t>::max());

  CHECK(loaded["u32"]["lowest"].as<uint32_t>() == std::numeric_limits<uint32_t>::lowest());
  CHECK(loaded["u32"]["max"].as<uint32_t>() == std::numeric_limits<uint32_t>::max());

  CHECK(loaded["u64"]["lowest"].as<uint64_t>() == std::numeric_limits<uint64_t>::lowest());
  CHECK(loaded["u64"]["max"].as<uint64_t>() == std::numeric_limits<uint64_t>::max());
}

TEST_CASE("IniFile Save/Load Consistency - All Floating Types")
{
  ini::inifile ini;
  ini["float"]["lowest"] = std::numeric_limits<float>::lowest();  // -FLT_MAX
  ini["float"]["min"] = std::numeric_limits<float>::min();        // 最小正数
  ini["float"]["max"] = std::numeric_limits<float>::max();

  ini["double"]["lowest"] = std::numeric_limits<double>::lowest();
  ini["double"]["min"] = std::numeric_limits<double>::min();
  ini["double"]["max"] = std::numeric_limits<double>::max();

  ini["long double"]["lowest"] = std::numeric_limits<long double>::lowest();
  ini["long double"]["min"] = std::numeric_limits<long double>::min();
  ini["long double"]["max"] = std::numeric_limits<long double>::max();

  ini["special"]["pos_inf"] = std::numeric_limits<double>::infinity();
  ini["special"]["neg_inf"] = -std::numeric_limits<double>::infinity();
  ini["special"]["nan"] = std::numeric_limits<double>::quiet_NaN();

  ini.save("test_floats.ini");

  ini::inifile loaded;
  loaded.load("test_floats.ini");

  CHECK(loaded["float"]["lowest"].as<float>() == std::numeric_limits<float>::lowest());
  CHECK(loaded["float"]["min"].as<float>() == std::numeric_limits<float>::min());
  CHECK(loaded["float"]["max"].as<float>() == std::numeric_limits<float>::max());

  CHECK(loaded["double"]["lowest"].as<double>() == std::numeric_limits<double>::lowest());
  CHECK(loaded["double"]["min"].as<double>() == std::numeric_limits<double>::min());
  CHECK(loaded["double"]["max"].as<double>() == std::numeric_limits<double>::max());

  CHECK(loaded["long double"]["lowest"].as<long double>() == std::numeric_limits<long double>::lowest());
  CHECK(loaded["long double"]["min"].as<long double>() == std::numeric_limits<long double>::min());
  CHECK(loaded["long double"]["max"].as<long double>() == std::numeric_limits<long double>::max());

  CHECK(std::isinf(loaded["special"]["pos_inf"].as<double>()));
  CHECK(loaded["special"]["pos_inf"].as<double>() > 0);
  CHECK(std::isinf(loaded["special"]["neg_inf"].as<double>()));
  CHECK(loaded["special"]["neg_inf"].as<double>() < 0);
  CHECK(std::isnan(loaded["special"]["nan"].as<double>()));
}

TEST_CASE("IniFile Save/Load Consistency - Special Floating-Point Values")
{
  ini::inifile ini;

  // 使用 std::numeric_limits<T>::quiet_NaN() 更标准
  ini["float"]["nan"] = std::numeric_limits<float>::quiet_NaN();
  ini["float"]["inf"] = std::numeric_limits<float>::infinity();
  ini["float"]["-inf"] = -std::numeric_limits<float>::infinity();

  ini["double"]["nan"] = std::numeric_limits<double>::quiet_NaN();
  ini["double"]["inf"] = std::numeric_limits<double>::infinity();
  ini["double"]["-inf"] = -std::numeric_limits<double>::infinity();

  ini["long double"]["nan"] = std::numeric_limits<long double>::quiet_NaN();
  ini["long double"]["inf"] = std::numeric_limits<long double>::infinity();
  ini["long double"]["-inf"] = -std::numeric_limits<long double>::infinity();

  ini.save("test_special_floats.ini");

  ini::inifile loaded;
  loaded.load("test_special_floats.ini");

  // float
  CHECK((std::isnan(loaded["float"]["nan"].as<float>())));
  CHECK((std::isinf(loaded["float"]["inf"].as<float>()) && loaded["float"]["inf"].as<float>() > 0));
  CHECK((std::isinf(loaded["float"]["-inf"].as<float>()) && loaded["float"]["-inf"].as<float>() < 0));

  // double
  CHECK((std::isnan(loaded["double"]["nan"].as<double>())));
  CHECK((std::isinf(loaded["double"]["inf"].as<double>()) && loaded["double"]["inf"].as<double>() > 0));
  CHECK((std::isinf(loaded["double"]["-inf"].as<double>()) && loaded["double"]["-inf"].as<double>() < 0));

  // long double
  CHECK((std::isnan(loaded["long double"]["nan"].as<long double>())));
  CHECK(
    (std::isinf(loaded["long double"]["inf"].as<long double>()) && loaded["long double"]["inf"].as<long double>() > 0));
  CHECK((std::isinf(loaded["long double"]["-inf"].as<long double>()) &&
         loaded["long double"]["-inf"].as<long double>() < 0));
}

TEST_CASE("IniFile Save/Load Consistency - bool, std::string, const char*, char types")
{
  ini::inifile ini;

  // 测试 bool 类型的保存和加载
  ini["bool"]["true"] = true;
  ini["bool"]["false"] = false;

  // 测试 std::string 类型的保存和加载
  ini["string"]["hello"] = "Hello, world!";
  ini["string"]["empty"] = "";

  // 测试 const char* 类型的保存和加载
  ini["cstr"]["sample"] = "This is a test!";
  ini["cstr"]["empty"] = "";

  // 测试 char 类型的保存和加载
  ini["char"]["char_val"] = 'A';

  // 测试 signed char 类型的保存和加载
  ini["signed_char"]["signed_char_val"] = static_cast<signed char>(-128);

  // 测试 unsigned char 类型的保存和加载
  ini["unsigned_char"]["unsigned_char_val"] = static_cast<unsigned char>(255);

  ini.save("test_all_types.ini");

  ini::inifile loaded;
  loaded.load("test_all_types.ini");

  // 验证 bool 类型
  CHECK(loaded["bool"]["true"].as<bool>() == true);
  CHECK(loaded["bool"]["false"].as<bool>() == false);

  // 验证 std::string 类型
  CHECK(loaded["string"]["hello"].as<std::string>() == "Hello, world!");
  CHECK(loaded["string"]["empty"].as<std::string>() == "");

  // 验证 const char* 类型
  CHECK(loaded["cstr"]["sample"].as<const char *>() == std::string("This is a test!"));
  CHECK(loaded["cstr"]["empty"].as<const char *>() == std::string(""));

  // 验证 char 类型
  CHECK(loaded["char"]["char_val"].as<char>() == 'A');

  // 验证 signed char 类型
  CHECK(loaded["signed_char"]["signed_char_val"].as<signed char>() == static_cast<signed char>(-128));

  // 验证 unsigned char 类型
  CHECK(loaded["unsigned_char"]["unsigned_char_val"].as<unsigned char>() == static_cast<unsigned char>(255));
}

TEST_CASE("field: swap exchanges values and comments correctly", "[field][swap]")
{
  ini::field f1("value1");
  f1.set_comment("comment1");

  ini::field f2("value2");
  f2.set_comment("comment2");

  // 交换前检查初始值
  REQUIRE(f1.as<std::string>() == "value1");
  REQUIRE(f2.as<std::string>() == "value2");

  // 交换
  using std::swap;
  swap(f1, f2);

  // 交换后检查
  REQUIRE(f1.as<std::string>() == "value2");
  REQUIRE(f2.as<std::string>() == "value1");
}

TEST_CASE("field: member swap exchanges values and comments correctly", "[field][member_swap]")
{
  ini::field f1("value1");
  f1.set_comment("comment1");

  ini::field f2("value2");
  f2.set_comment("comment2");

  // 交换前检查初始值
  REQUIRE(f1.as<std::string>() == "value1");
  REQUIRE(f2.as<std::string>() == "value2");

  // 使用成员函数 swap
  f1.swap(f2);

  // 交换后检查
  REQUIRE(f1.as<std::string>() == "value2");
  REQUIRE(f2.as<std::string>() == "value1");
}

TEST_CASE("basic_section: member swap exchanges data and comments correctly", "[basic_section][member_swap]")
{
  // 创建第一个 section，包含数据和注释
  ini::section s1;
  s1.set("key1", "value1");
  s1.set("key2", "value2");
  s1.set_comment("This is section 1\nSecond line of comment", '#');

  // 创建第二个 section，包含不同的数据和注释
  ini::section s2;
  s2.set("keyA", "valueA");
  s2.set("keyB", "valueB");
  s2.set_comment("This is section 2\nAnother comment", ';');

  // 交换前检查 s1 和 s2 的状态
  REQUIRE(s1.size() == 2);
  REQUIRE(s1.get("key1").as<std::string>() == "value1");
  REQUIRE(s1.get("key2").as<std::string>() == "value2");

  REQUIRE(s2.size() == 2);
  REQUIRE(s2.get("keyA").as<std::string>() == "valueA");
  REQUIRE(s2.get("keyB").as<std::string>() == "valueB");

  // 使用成员函数 swap 交换 s1 和 s2
  s1.swap(s2);

  // 交换后检查 s1 和 s2 的状态
  REQUIRE(s1.size() == 2);
  REQUIRE(s1.get("keyA").as<std::string>() == "valueA");
  REQUIRE(s1.get("keyB").as<std::string>() == "valueB");

  REQUIRE(s2.size() == 2);
  REQUIRE(s2.get("key1").as<std::string>() == "value1");
  REQUIRE(s2.get("key2").as<std::string>() == "value2");
}

TEST_CASE("basic_section: non-member swap exchanges data and comments correctly", "[basic_section][non_member_swap]")
{
  // 创建第一个 section，包含数据和注释
  ini::section s1;
  s1.set("key1", "value1");
  s1.set("key2", "value2");
  s1.set_comment("Comment for section 1", '#');

  // 创建第二个 section，包含不同的数据和注释
  ini::section s2;
  s2.set("keyA", "valueA");
  s2.set("keyB", "valueB");
  s2.set_comment("Comment for section 2", ';');

  // 交换前检查 s1 和 s2 的状态
  REQUIRE(s1.size() == 2);
  REQUIRE(s1.get("key1").as<std::string>() == "value1");
  REQUIRE(s1.get("key2").as<std::string>() == "value2");

  REQUIRE(s2.size() == 2);
  REQUIRE(s2.get("keyA").as<std::string>() == "valueA");
  REQUIRE(s2.get("keyB").as<std::string>() == "valueB");

  // 使用非成员函数 swap 交换 s1 和 s2
  swap(s1, s2);

  // 交换后检查 s1 和 s2 的状态
  REQUIRE(s1.size() == 2);
  REQUIRE(s1.get("keyA").as<std::string>() == "valueA");
  REQUIRE(s1.get("keyB").as<std::string>() == "valueB");

  REQUIRE(s2.size() == 2);
  REQUIRE(s2.get("key1").as<std::string>() == "value1");
  REQUIRE(s2.get("key2").as<std::string>() == "value2");
}

TEST_CASE("basic_section: swap with empty section", "[basic_section][swap_empty]")
{
  // 创建一个空的 section 和一个包含数据的 section
  ini::section s1;
  ini::section s2;
  s2.set("key1", "value1");
  s2.set("key2", "value2");

  // 交换前检查 s1 和 s2 的状态
  REQUIRE(s1.size() == 0);  // 空 section
  REQUIRE(s2.size() == 2);  // 有数据的 section

  // 使用成员函数 swap 交换 s1 和 s2
  s1.swap(s2);

  // 交换后检查 s1 和 s2 的状态
  REQUIRE(s1.size() == 2);  // 交换后 s1 包含数据
  REQUIRE(s2.size() == 0);  // 交换后 s2 变为空
}

TEST_CASE("basic_section: swap with self", "[basic_section][swap_self]")
{
  // 创建一个包含数据的 section
  ini::section s;
  s.set("key1", "value1");
  s.set("key2", "value2");

  // 交换前检查 s 的状态
  REQUIRE(s.size() == 2);
  REQUIRE(s.get("key1").as<std::string>() == "value1");
  REQUIRE(s.get("key2").as<std::string>() == "value2");

  // 使用成员函数 swap 交换 s 自身
  s.swap(s);

  // 交换后检查 s 的状态
  REQUIRE(s.size() == 2);
  REQUIRE(s.get("key1").as<std::string>() == "value1");
  REQUIRE(s.get("key2").as<std::string>() == "value2");
}

TEST_CASE("Test swap functionality of basic_inifile", "[swap]")
{
  // 1. 基本交换测试：添加一些数据到 ini1 和 ini2 中
  ini::inifile ini1;
  ini::inifile ini2;

  ini1["section1"]["key1"] = "value1";
  ini1["section1"]["key2"] = "value2";
  ini2["section2"]["key3"] = "value3";

  REQUIRE(ini1["section1"]["key1"].as<std::string>() == "value1");
  REQUIRE(ini1["section1"]["key2"].as<std::string>() == "value2");
  REQUIRE(ini2["section2"]["key3"].as<std::string>() == "value3");

  // 交换 ini1 和 ini2
  swap(ini1, ini2);

  // 检查交换后的结果
  REQUIRE(ini1["section2"]["key3"].as<std::string>() == "value3");
  REQUIRE(ini2["section1"]["key1"].as<std::string>() == "value1");
  REQUIRE(ini2["section1"]["key2"].as<std::string>() == "value2");

  // 确保大小一致
  REQUIRE(ini1.size() == 1);
  REQUIRE(ini2.size() == 1);

  // 2. 空对象交换：交换两个空的 basic_inifile 对象
  ini::inifile empty1;
  ini::inifile empty2;

  swap(empty1, empty2);
  REQUIRE(empty1.empty() == true);
  REQUIRE(empty2.empty() == true);

  // 3. 不同大小对象交换：一个空对象与一个非空对象交换
  ini::inifile non_empty;
  non_empty["section1"]["key1"] = "value1";

  swap(empty1, non_empty);

  // 确保交换后，empty1 拥有 non_empty 的内容，non_empty 成为空
  REQUIRE(empty1["section1"]["key1"].as<std::string>() == "value1");
  REQUIRE(non_empty.empty() == true);
  REQUIRE(empty1.size() == 1);
  REQUIRE(non_empty.size() == 0);

  // 4. 多次交换：交换两次，确保交换回原样
  ini::inifile ini3;
  ini3["section1"]["key1"] = "value1";
  ini::inifile ini4;
  ini4["section2"]["key2"] = "value2";

  swap(ini3, ini4);  // 初次交换
  swap(ini3, ini4);  // 再次交换，应该恢复到初始状态

  REQUIRE(ini3["section1"]["key1"].as<std::string>() == "value1");
  REQUIRE(ini4["section2"]["key2"].as<std::string>() == "value2");
  REQUIRE(ini3.size() == 1);
  REQUIRE(ini4.size() == 1);

  // 5. 检查交换后的内容一致性
  ini::inifile ini5;
  ini5["section1"]["key1"] = "value1";
  ini5["section1"]["key2"] = "value2";

  ini::inifile ini6;
  ini6["section2"]["key3"] = "value3";

  // 交换后，检查数据是否一致
  swap(ini5, ini6);
  REQUIRE(ini5["section2"]["key3"].as<std::string>() == "value3");
  REQUIRE(ini6["section1"]["key1"].as<std::string>() == "value1");
  REQUIRE(ini6["section1"]["key2"].as<std::string>() == "value2");
}

////////////////////////////////////////////////// ini::comment //////////////////////////////
TEST_CASE("comment default constructor", "[comment]")
{
  ini::comment c;
  REQUIRE(c.empty() == true);
}

TEST_CASE("comment string constructor", "[comment]")
{
  ini::comment c("This is a comment");
  REQUIRE_FALSE(c.empty());
  REQUIRE(c.to_vector() == std::vector<std::string>{"; This is a comment"});
}

TEST_CASE("comment vector constructor", "[comment]")
{
  std::vector<std::string> vec = {"First comment", "Second comment"};
  ini::comment c(vec);
  REQUIRE_FALSE(c.empty());
  REQUIRE(c.to_vector() == std::vector<std::string>{"; First comment", "; Second comment"});
}

TEST_CASE("comment initializer_list constructor", "[comment]")
{
  ini::comment c({"Comment 1", "Comment 2"});
  REQUIRE_FALSE(c.empty());
  REQUIRE(c.to_vector() == std::vector<std::string>{"; Comment 1", "; Comment 2"});
}

TEST_CASE("comment add", "[comment]")
{
  ini::comment c;
  c.add("This is a comment");
  REQUIRE_FALSE(c.empty());
  REQUIRE(c.to_vector() == std::vector<std::string>{"; This is a comment"});

  c.add("Another comment");
  REQUIRE(c.to_vector() == std::vector<std::string>{"; This is a comment", "; Another comment"});
}

TEST_CASE("comment add comment", "[comment]")
{
  ini::comment c1({"Comment 1"});
  ini::comment c2({"Comment 2"});

  c1.add(c2);
  REQUIRE(c1.to_vector() == std::vector<std::string>{"; Comment 1", "; Comment 2"});
}

TEST_CASE("comment add move comment", "[comment]")
{
  ini::comment c1({"Comment 1"});
  ini::comment c2("Comment 2");

  c1.add(std::move(c2));
  REQUIRE(c1.to_vector() == std::vector<std::string>{"; Comment 1", "; Comment 2"});
  REQUIRE(c2.empty() == true);  // c2 should be empty after move
}

TEST_CASE("comment set", "[comment]")
{
  ini::comment c;
  c.set("This is a new comment");
  REQUIRE(c.to_vector() == std::vector<std::string>{"; This is a new comment"});

  c.set("Another comment");
  REQUIRE(c.to_vector() == std::vector<std::string>{"; Another comment"});

  c.set("");  // Reset to empty
  REQUIRE(c.empty() == true);
}

TEST_CASE("comment set comment", "[comment]")
{
  ini::comment c1({"Comment 1"});
  ini::comment c2({"New comment"});

  c1.set(c2);
  REQUIRE(c1.to_vector() == std::vector<std::string>{"; New comment"});
}

TEST_CASE("comment set move comment", "[comment]")
{
  ini::comment c1({"Comment 1"});
  ini::comment c2({"New comment"});

  c1.set(std::move(c2));
  REQUIRE(c1.to_vector() == std::vector<std::string>{"; New comment"});
  REQUIRE(c2.empty() == true);  // c2 should be empty after move
}

TEST_CASE("comment empty", "[comment]")
{
  ini::comment c;
  REQUIRE(c.empty() == true);

  c.add("Some comment");
  REQUIRE(c.empty() == false);

  c.clear();
  REQUIRE(c.empty() == true);
}

TEST_CASE("comment copy constructor", "[comment]")
{
  ini::comment c1({"Comment 1", "Comment 2"});
  ini::comment c2 = c1;  // Copy constructor

  REQUIRE(c2.to_vector() == c1.to_vector());
}

TEST_CASE("comment move constructor", "[comment]")
{
  ini::comment c1({"Comment 1", "Comment 2"});
  ini::comment c2 = std::move(c1);  // Move constructor

  REQUIRE(c2.to_vector() == std::vector<std::string>{"; Comment 1", "; Comment 2"});
  REQUIRE(c1.empty() == true);  // c1 should be empty after move
}

TEST_CASE("comment copy assignment", "[comment]")
{
  ini::comment c1({"Comment 1", "Comment 2"});
  ini::comment c2;

  c2 = c1;  // Copy assignment
  REQUIRE(c2.to_vector() == c1.to_vector());
}

TEST_CASE("comment move assignment", "[comment]")
{
  ini::comment c1({"Comment 1", "Comment 2"});
  ini::comment c2;

  c2 = std::move(c1);  // Move assignment
  REQUIRE(c2.to_vector() == std::vector<std::string>{"; Comment 1", "; Comment 2"});
  REQUIRE(c1.empty() == true);  // c1 should be empty after move
}

TEST_CASE("comment self-assignment", "[comment]")
{
  ini::comment c({"Comment 1"});

  c = c;  // Self-assignment
  REQUIRE(c.to_vector() == std::vector<std::string>{"; Comment 1"});
}

TEST_CASE("comment equality operator", "[comment]")
{
  ini::comment c1({"Comment 1"});
  ini::comment c2({"Comment 1"});
  ini::comment c3({"Comment 2"});

  REQUIRE(c1 == c2);  // Same content
  REQUIRE(c1 != c3);  // Different content
}

TEST_CASE("comment clear", "[comment]")
{
  ini::comment c({"Comment 1", "Comment 2"});
  c.clear();
  REQUIRE(c.empty() == true);
}

TEST_CASE("comment constructors", "[comment]")
{
  SECTION("default constructor")
  {
    ini::comment c;
    REQUIRE(c.empty());
  }

  SECTION("construct from string")
  {
    ini::comment c("Comment line");
    REQUIRE_FALSE(c.empty());
    REQUIRE(c.to_vector().size() == 1);
  }

  SECTION("construct from vector")
  {
    std::vector<std::string> vec = {"Line1", "Line2"};
    ini::comment c(vec);
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line1", "; Line2"});
  }

  SECTION("construct from initializer_list")
  {
    ini::comment c({"Line1", "Line2"});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line1", "; Line2"});
  }
}

TEST_CASE("comment copy and move", "[comment]")
{
  ini::comment original({"Line1", "Line2"});

  SECTION("copy constructor")
  {
    ini::comment copy(original);
    REQUIRE(copy == original);
  }

  SECTION("move constructor")
  {
    ini::comment moved(std::move(original));
    REQUIRE(moved.to_vector() == std::vector<std::string>{"; Line1", "; Line2"});
  }

  SECTION("copy assignment")
  {
    ini::comment c;
    c = original;
    REQUIRE(c == original);
  }

  SECTION("move assignment")
  {
    ini::comment c;
    ini::comment temp({"LineX"});
    c = std::move(temp);
    REQUIRE(c.to_vector() == std::vector<std::string>{"; LineX"});
  }

  SECTION("self copy assignment")
  {
    ini::comment self({"Same"});
    self = self;  // 自赋值测试
    REQUIRE(self.to_vector() == std::vector<std::string>{"; Same"});
  }

  SECTION("self move assignment")
  {
    ini::comment self({"Same"});
    self = std::move(self);  // 自移动赋值
    REQUIRE(self.to_vector() == std::vector<std::string>{"; Same"});
  }
}

TEST_CASE("comment set functions", "[comment]")
{
  ini::comment c;

  SECTION("set from string")
  {
    c.set("Hello\nWorld");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Hello", "; World"});
  }

  SECTION("set from another comment (copy)")
  {
    ini::comment other({"A", "B"});
    c.set(other);
    REQUIRE(c == other);
  }

  SECTION("set from another comment (move)")
  {
    ini::comment other({"X", "Y"});
    c.set(std::move(other));
    REQUIRE(c.to_vector() == std::vector<std::string>{"; X", "; Y"});
  }

  SECTION("set from initializer list")
  {
    c.set({"C", "D"});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; C", "; D"});
  }

  SECTION("set empty string clears comments")
  {
    c.set("Line");
    c.set("");  // 设置空字符串后应该清空
    REQUIRE(c.empty());
  }
}

TEST_CASE("comment add functions", "[comment]")
{
  ini::comment c;

  SECTION("add string")
  {
    c.add("First\nSecond");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; First", "; Second"});
  }

  SECTION("add another comment (copy)")
  {
    ini::comment other({"A", "B"});
    c.add(other);
    REQUIRE(c.to_vector() == std::vector<std::string>{"; A", "; B"});
  }

  SECTION("add another comment (move)")
  {
    ini::comment other({"X", "Y"});
    c.add(std::move(other));
    REQUIRE(c.to_vector() == std::vector<std::string>{"; X", "; Y"});
  }

  SECTION("add empty comment has no effect")
  {
    ini::comment empty;
    c.add(empty);
    REQUIRE(c.empty());
  }
}

TEST_CASE("comment clear and empty", "[comment]")
{
  ini::comment c({"Comment 1", "Comment 2"});
  REQUIRE_FALSE(c.empty());
  c.clear();
  REQUIRE(c.empty());
}

TEST_CASE("comment to_vector returns correct content", "[comment]")
{
  ini::comment c({"Line1", "Line2"});
  auto vec = c.to_vector();
  REQUIRE(vec == std::vector<std::string>{"; Line1", "; Line2"});
}

TEST_CASE("comment equality and inequality", "[comment]")
{
  ini::comment c1({"Comment 1"});
  ini::comment c2({"Comment 1"});
  ini::comment c3({"Comment 2"});

  REQUIRE(c1 == c2);  // Same content
  REQUIRE(c1 != c3);  // Different content
}

TEST_CASE("comment swap", "[comment]")
{
  ini::comment c1({"A"});
  ini::comment c2({"B", "C"});
  c1.swap(c2);
  REQUIRE(c1.to_vector() == std::vector<std::string>{"; B", "; C"});
  REQUIRE(c2.to_vector() == std::vector<std::string>{"; A"});

  swap(c1, c2);  // 测试友元swap
  REQUIRE(c1.to_vector() == std::vector<std::string>{"; A"});
  REQUIRE(c2.to_vector() == std::vector<std::string>{"; B", "; C"});
}

TEST_CASE("comment with custom symbol", "[comment]")
{
  SECTION("construct with '#' symbol")
  {
    ini::comment c("Line1\nLine2", '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# Line1", "# Line2"});
  }

  SECTION("add with '#' symbol")
  {
    ini::comment c;
    c.add("Appended1\nAppended2", '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# Appended1", "# Appended2"});
  }

  SECTION("set with '#' symbol")
  {
    ini::comment c;
    c.set("Set1\nSet2", '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# Set1", "# Set2"});
  }

  SECTION("initializer list with '#' symbol")
  {
    ini::comment c({"IL1", "IL2"}, '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# IL1", "# IL2"});
  }

  SECTION("set initializer list with '#' symbol")
  {
    ini::comment c;
    c.set({"X", "Y"}, '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# X", "# Y"});
  }
}

TEST_CASE("comment multi-line string handling", "[comment]")
{
  ini::comment c;

  SECTION("add multi-line string")
  {
    c.add("Line1\nLine2\nLine3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line1", "; Line2", "; Line3"});
  }

  SECTION("set multi-line string")
  {
    c.set("One\nTwo\nThree");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; One", "; Two", "; Three"});
  }

  SECTION("empty lines are handled correctly")
  {
    c.set("First\n\nThird");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; First", "; Third"});
  }

  SECTION("whitespace-only lines are trimmed")
  {
    c.set("  One  \n   \n  Three ");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; One", "; Three"});
  }

  SECTION("leading/trailing whitespace preserved after symbol")
  {
    c.set("  Hello\nWorld  ");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Hello", "; World"});
  }
}

TEST_CASE("comment content contains symbol", "[comment]")
{
  SECTION("content contains ;, but symbol is default ;")
  {
    ini::comment c({";Already commented", "  Normal line"});
    REQUIRE(c.to_vector() == std::vector<std::string>{";Already commented", "; Normal line"});
  }

  SECTION("content contains #, but symbol is default ;")
  {
    ini::comment c({"#Hash style", "  \rAnother line  \r\n"});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; #Hash style", "; Another line"});
  }

  SECTION("content contains ;, with symbol = '#'")
  {
    ini::comment c({";Semicolon line", "Line2"}, '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# ;Semicolon line", "# Line2"});
  }

  SECTION("content contains #, with symbol = '#'")
  {
    ini::comment c({"#Already commented", "LineB"}, '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"#Already commented", "# LineB"});
  }

  SECTION("lines with only comment symbols")
  {
    ini::comment c({";", "#"}, ';');
    REQUIRE(c.to_vector() == std::vector<std::string>{";", "; #"});
  }

  SECTION("lines with only empty comment symbols")
  {
    ini::comment c({";", "", "#"}, ';');
    REQUIRE(c.to_vector() == std::vector<std::string>{";", "; #"});
  }
}

TEST_CASE("comment handles leading and trailing whitespace", "[comment]")
{
  SECTION("leading and trailing spaces are trimmed")
  {
    ini::comment c({"   leading and trailing   "});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; leading and trailing"});
  }

  SECTION("leading and trailing tabs are trimmed")
  {
    ini::comment c({" \t\tcomment with tabs\t\t "});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; comment with tabs"});
  }

  SECTION("mixed spaces and tabs")
  {
    ini::comment c({" \t mixed \t spaces \t "});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; mixed \t spaces"});
  }

  SECTION("empty line becomes a single comment symbol")
  {
    ini::comment c({""});
    REQUIRE(c.to_vector() == std::vector<std::string>{});
  }

  SECTION("line with only whitespace becomes a single comment symbol")
  {
    ini::comment c({"      \t  \t  "});
    REQUIRE(c.to_vector() == std::vector<std::string>{});
  }

  SECTION("multiple lines with mixed whitespace and content")
  {
    ini::comment c({" \t first line ", "", "    ", "\t second line \t"});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; first line", "; second line"});
  }

  SECTION("comment symbol is '#' and whitespace trimming applies")
  {
    ini::comment c({" \t value \t ", "  "}, '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# value"});
  }

  SECTION("preserves embedded spaces inside content")
  {
    ini::comment c({"text   with   spaces"});
    REQUIRE(c.to_vector() == std::vector<std::string>{"; text   with   spaces"});
  }
}

TEST_CASE("set function", "[comment]")
{
  SECTION("set with non-empty string")
  {
    ini::comment c;
    c.set("First line");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; First line"});
  }

  SECTION("set with empty string")
  {
    ini::comment c;
    c.set("");  // 清空注释
    REQUIRE(c.empty() == true);
  }

  SECTION("set with whitespace string")
  {
    ini::comment c;
    c.set("     \t  \t  ");  // 仅空白字符
    REQUIRE(c.empty() == true);
  }

  SECTION("set with multiple lines")
  {
    ini::comment c;
    c.set("Line 1\nLine 2\nLine 3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2", "; Line 3"});
  }

  SECTION("set with whitespace between lines")
  {
    ini::comment c;
    c.set("Line 1\n     \t  \t  \nLine 2");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2"});
  }

  SECTION("set with custom symbol")
  {
    ini::comment c;
    c.set("Custom comment", '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# Custom comment"});
  }
}

TEST_CASE("add function", "[comment]")
{
  SECTION("add non-empty string")
  {
    ini::comment c;
    c.add("First line");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; First line"});
  }

  SECTION("add empty string")
  {
    ini::comment c;
    c.add("");  // 不添加任何注释
    REQUIRE(c.empty() == true);
  }

  SECTION("add whitespace string")
  {
    ini::comment c;
    c.add("     \t  \t  ");  // 仅空白字符
    REQUIRE(c.empty() == true);
  }

  SECTION("add multiple lines")
  {
    ini::comment c;
    c.add("Line 1\nLine 2\nLine 3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2", "; Line 3"});
  }

  SECTION("add multiple lines with whitespace")
  {
    ini::comment c;
    c.add("Line 1\n     \t  \t  \nLine 2");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2"});
  }

  SECTION("add with custom symbol")
  {
    ini::comment c;
    c.add("Custom comment", '#');
    REQUIRE(c.to_vector() == std::vector<std::string>{"# Custom comment"});
  }

  SECTION("add to existing comments")
  {
    ini::comment c({"First comment", "Second comment"});
    c.add("Third comment");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; First comment", "; Second comment", "; Third comment"});
  }

  SECTION("add with whitespace between lines")
  {
    ini::comment c({"First comment", "Second comment"});
    c.add("   \t  \t ");  // 仅空白字符
    c.add("Third comment");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; First comment", "; Second comment", "; Third comment"});
  }
}

TEST_CASE("set function with whitespace between lines", "[comment]")
{
  SECTION("set with multiple lines containing whitespace between them")
  {
    ini::comment c;
    c.set("Line 1\n \t \nLine 2\n\n   \t");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2"});
  }

  SECTION("set with newlines and spaces between")
  {
    ini::comment c;
    c.set("Line 1\n \nLine 2\n   \n  \nLine 3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2", "; Line 3"});
  }

  SECTION("set with only whitespace between lines")
  {
    ini::comment c;
    c.set("Line 1\n \t\n\n \t\nLine 2");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2"});
  }

  SECTION("set with multiple types of whitespace")
  {
    ini::comment c;
    c.set("Line 1\n\t \n  Line 2\n \t\nLine 3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2", "; Line 3"});
  }
}

TEST_CASE("add function with whitespace between lines", "[comment]")
{
  SECTION("add with multiple lines containing whitespace between them")
  {
    ini::comment c;
    c.add("Line 1\n \t \nLine 2\n\n   \t");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2"});
  }

  SECTION("add with newlines and spaces between")
  {
    ini::comment c;
    c.add("Line 1\n \nLine 2\n   \n  \nLine 3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2", "; Line 3"});
  }

  SECTION("add with only whitespace between lines")
  {
    ini::comment c;
    c.add("Line 1\n \t\n\n \t\nLine 2");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2"});
  }

  SECTION("add with multiple types of whitespace")
  {
    ini::comment c;
    c.add("Line 1\n\t \n  Line 2\n \t\nLine 3");
    REQUIRE(c.to_vector() == std::vector<std::string>{"; Line 1", "; Line 2", "; Line 3"});
  }
}

TEST_CASE("comment::add with string", "[comment]")
{
  ini::comment c;
  c.add("line1\nline2", ';');
  auto vec = c.to_vector();

  REQUIRE(vec.size() == 2);
  REQUIRE(vec[0] == "; line1");
  REQUIRE(vec[1] == "; line2");
}

TEST_CASE("comment::add with initializer_list", "[comment]")
{
  ini::comment c;
  c.add({"foo", "bar"}, '#');
  auto vec = c.to_vector();

  REQUIRE(vec.size() == 2);
  REQUIRE(vec[0] == "# foo");
  REQUIRE(vec[1] == "# bar");
}

TEST_CASE("comment::add comment by copy and move", "[comment]")
{
  ini::comment a;
  a.add({"x", "y"});

  SECTION("copy add")
  {
    ini::comment b;
    b.add(a);
    REQUIRE(b.view().size() == 2);
    REQUIRE(b.to_vector() == a.to_vector());
  }

  SECTION("move add")
  {
    ini::comment b;
    b.add(std::move(a));
    REQUIRE(b.to_vector().size() == 2);
    REQUIRE(a.empty());
  }
}

TEST_CASE("comment::set and clear", "[comment]")
{
  ini::comment c;
  c.set("set\ncomment", '#');
  REQUIRE(c.to_vector()[0] == "# set");
  REQUIRE(c.to_vector()[1] == "# comment");

  c.clear();
  REQUIRE(c.empty());

  c.set({"one", "two"}, ';');
  REQUIRE(c.to_vector()[0] == "; one");
  REQUIRE(c.to_vector()[1] == "; two");
}

TEST_CASE("comment::set by comment object", "[comment]")
{
  ini::comment c1;
  c1.set("c1-line");

  ini::comment c2;
  c2.set(c1);
  REQUIRE(c2.to_vector() == c1.to_vector());

  ini::comment c3;
  c3.set(std::move(c2));
  REQUIRE(c3.to_vector()[0] == "; c1-line");
  REQUIRE(c2.empty());
}

TEST_CASE("comment::view and to_vector", "[comment]")
{
  ini::comment c;
  c.set("v1\nv2");

  const auto &ref = c.view();
  REQUIRE(ref.size() == 2);
  REQUIRE(ref[0] == "; v1");

  auto copy = c.to_vector();
  REQUIRE(copy == ref);
}

TEST_CASE("comment::equality", "[comment]")
{
  ini::comment a;
  a.set("a");

  ini::comment b;
  b.set("a");

  ini::comment c;
  c.set("c");

  REQUIRE(a == b);
  REQUIRE(a != c);
}

TEST_CASE("comment iterators", "[comment]")
{
  ini::comment c;
  c.add({"first", "second"});

  size_t count = 0;
  for (const auto &line : c)
  {
    REQUIRE_FALSE(line.empty());
    ++count;
  }
  REQUIRE(count == 2);

  count = 0;
  for (auto it = c.rbegin(); it != c.rend(); ++it)
  {
    REQUIRE_FALSE(it->empty());
    ++count;
  }
  REQUIRE(count == 2);
}

TEST_CASE("comment::view returns correct reference", "[comment][view]")
{
  ini::comment c;
  c.set("first\nsecond");

  const auto &view1 = c.view();

  REQUIRE(view1.size() == 2);
  REQUIRE(view1[0] == "; first");
  REQUIRE(view1[1] == "; second");
  REQUIRE(view1 == c.to_vector());

  // 修改原始数据再观察 view 的变化
  c.add("third");
  const auto &view2 = c.view();

  REQUIRE(view2.size() == 3);
  REQUIRE(view2[2] == "; third");

  // 清空后 view 应为空引用容器
  c.clear();
  const auto view3 = c.view();
  REQUIRE(view3.empty());
  // REQUIRE(view3.data() != nullptr);  // 不建议断言 data() != nullptr, 兼容性不好
}

TEST_CASE("ini::field comment operations", "[ini][field][comment]")
{
  using ini::comment;
  using ini::field;

  SECTION("default constructed field has empty comment")
  {
    field f;
    REQUIRE(f.comment().view().empty());
  }

  SECTION("set comment by string")
  {
    field f;
    f.set_comment("this is a comment");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "; this is a comment");
  }

  SECTION("set comment by string with custom symbol")
  {
    field f;
    f.set_comment("commented", '#');
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "# commented");
  }

  SECTION("set comment by multiline string")
  {
    field f;
    f.set_comment("line1\nline2\nline3");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == "; line1");
    REQUIRE(v[1] == "; line2");
    REQUIRE(v[2] == "; line3");
  }

  SECTION("set comment by initializer list")
  {
    field f;
    f.set_comment({"a", "b", "c"});
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == "; a");
    REQUIRE(v[1] == "; b");
    REQUIRE(v[2] == "; c");
  }

  SECTION("add comment by string")
  {
    field f;
    f.add_comment("first");
    f.add_comment("second");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 2);
    REQUIRE(v[0] == "; first");
    REQUIRE(v[1] == "; second");
  }

  SECTION("add comment from another comment (copy)")
  {
    field f1, f2;
    f1.set_comment("origin");
    f2.add_comment(f1.comment());
    const auto &v = f2.comment().view();
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "; origin");
  }

  SECTION("add comment from another comment (move)")
  {
    field f1, f2;
    f1.set_comment("moved");
    f2.add_comment(std::move(f1.comment()));
    const auto &v = f2.comment().view();
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "; moved");
    // moved-from object should now be empty
    REQUIRE(f1.comment().view().empty());
  }

  SECTION("clear comment")
  {
    field f;
    f.set_comment("erase me");
    REQUIRE_FALSE(f.comment().view().empty());
    f.clear_comment();
    REQUIRE(f.comment().view().empty());
  }

  SECTION("comment access is consistent")
  {
    field f;
    auto &c1 = f.comment();
    auto &c2 = f.comment();
    c1.set("line1");
    REQUIRE(c2.view().size() == 1);
    REQUIRE(c2.view()[0] == "; line1");
  }
}

TEST_CASE("ini::field comment edge cases", "[ini][field][comment][edge]")
{
  using ini::comment;
  using ini::field;

  SECTION("set comment with empty string")
  {
    field f;
    f.set_comment("");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 0);  // 空字符串也会生成一行注释
  }

  SECTION("set comment with only newlines")
  {
    field f;
    f.set_comment("\n\n");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 0);
  }

  SECTION("set comment with mix of LF and CRLF")
  {
    field f;
    f.set_comment("line1\r\nline2\nline3\r\n");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == "; line1");
    REQUIRE(v[1] == "; line2");
    REQUIRE(v[2] == "; line3");
  }

  SECTION("use invalid comment symbol should fallback or reject")
  {
    field f;
    // 如果实现限制了只能是 ';' 或 '#'
    // 应该忽略非法符号或默认使用 ';'
    f.set_comment("hello", '*');  // 不支持的符号
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 1);
    // 实际行为依你的实现而定，以下假设 fallback 到 `;`
    REQUIRE(v[0] == "; hello");
  }

  SECTION("repeated set_comment should replace previous one")
  {
    field f;
    f.set_comment("first");
    f.set_comment("second");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "; second");
  }

  SECTION("repeated add_comment should append")
  {
    field f;
    f.add_comment("first");
    f.add_comment("second");
    f.add_comment("third");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == "; first");
    REQUIRE(v[1] == "; second");
    REQUIRE(v[2] == "; third");
  }

  SECTION("add_comment with initializer list mixes correctly")
  {
    field f;
    f.set_comment("base");
    f.add_comment({"a", "b"}, '#');
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == "; base");
    REQUIRE(v[1] == "# a");
    REQUIRE(v[2] == "# b");
  }

  SECTION("clear then reuse comment")
  {
    field f;
    f.set_comment("start");
    f.clear_comment();
    REQUIRE(f.comment().view().empty());

    f.set_comment("after clear");
    const auto &v = f.comment().view();
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "; after clear");
  }
}

TEST_CASE("Section comment basic operations", "[section][comment]")
{
  using ini::comment;
  ini::section sec;

  SECTION("Initial comment should be empty")
  {
    REQUIRE(sec.comment().empty());
  }

  SECTION("Set single-line comment")
  {
    comment cmt;
    cmt.add("; This is a section comment");
    sec.set_comment(cmt);
    REQUIRE_FALSE(sec.comment().empty());
    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view().front() == "; This is a section comment");
  }

  SECTION("Set multi-line comment")
  {
    comment cmt;
    cmt.add("; line1");
    cmt.add("; line2");
    cmt.add("; line3");
    sec.set_comment(cmt);
    const auto &cmts = sec.comment();
    REQUIRE(cmts.view().size() == 3);
    REQUIRE(cmts.view()[1] == "; line2");
  }

  SECTION("Clear comment")
  {
    comment cmt;
    cmt.add("; to be cleared");
    sec.set_comment(cmt);
    sec.clear_comment();
    REQUIRE(sec.comment().empty());
  }

  SECTION("Overwrite existing comment")
  {
    comment first;
    first.add("; first");
    sec.set_comment(first);
    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; first");

    comment second;
    second.add("; overwritten");
    second.add("; second line");
    sec.set_comment(second);
    REQUIRE(sec.comment().view().size() == 2);
    REQUIRE(sec.comment().view()[0] == "; overwritten");
  }

  SECTION("Set empty comment explicitly")
  {
    comment empty;
    sec.set_comment(empty);
    REQUIRE(sec.comment().empty());
  }

  SECTION("Move comment into section")
  {
    comment movable;
    movable.add("; move this");
    movable.add("; and this");
    sec.set_comment(std::move(movable));
    REQUIRE(sec.comment().view().size() == 2);
    // 原 comment 已被移出，状态未定义，但容器实现应可容忍空 vector
  }

  SECTION("Copy comment object preserves content")
  {
    comment cmt;
    cmt.add("; copy me");
    ini::section sec2;
    sec2.set_comment(cmt);

    REQUIRE(sec2.comment().view().size() == 1);
    REQUIRE(sec2.comment().view()[0] == "; copy me");
    REQUIRE(cmt.view().size() == 1);  // 原 comment 未被破坏
  }
}

TEST_CASE("Section comment does not affect field comments", "[section][comment]")
{
  using ini::comment;
  ini::section sec;
  comment sec_comment;
  sec_comment.add("; section level");
  sec.set_comment(sec_comment);

  sec["key"] = "value";
  REQUIRE(sec["key"].comment().empty());

  comment field_comment;
  field_comment.add("# field level");  // 默认是 ';'
  sec["key"].set_comment(field_comment);

  REQUIRE(sec["key"].comment().view().size() == 1);
  REQUIRE(sec["key"].comment().view()[0] == "; # field level");

  sec["key"].clear_comment();
  REQUIRE(sec["key"].comment().view().empty());
  sec["key"].add_comment("; added field comment");
  REQUIRE(sec["key"].comment().view()[0] == "; added field comment");

  // 清除 section 注释不影响 field
  sec.clear_comment();
  REQUIRE(sec.comment().empty());
  REQUIRE_FALSE(sec["key"].comment().empty());
}

TEST_CASE("Section comment robustness under swap and assignment", "[section][comment][swap]")
{
  using ini::comment;
  using ini::section;

  SECTION("Swap swaps comments")
  {
    section sec1, sec2;

    sec1.add_comment("; sec1 comment");
    sec2.add_comment("; sec2 comment");

    std::swap(sec1, sec2);

    REQUIRE(sec1.comment().view().size() == 1);
    REQUIRE(sec1.comment().view()[0] == "; sec2 comment");

    REQUIRE(sec2.comment().view().size() == 1);
    REQUIRE(sec2.comment().view()[0] == "; sec1 comment");
  }

  SECTION("Self swap does not corrupt comment")
  {
    section sec;
    sec.add_comment("; self-swap test");
    std::swap(sec, sec);

    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; self-swap test");
  }

  SECTION("Copy assignment preserves comment")
  {
    section src, dst;
    src.add_comment("; source comment");
    dst = src;

    REQUIRE(dst.comment().view().size() == 1);
    REQUIRE(dst.comment().view()[0] == "; source comment");
    REQUIRE(src.comment().view()[0] == "; source comment");
  }

  SECTION("Self copy assignment is safe")
  {
    section sec;
    sec.add_comment("; self-assign test");
    sec = sec;

    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; self-assign test");
  }

  SECTION("Move assignment preserves comment")
  {
    section src;
    src.add_comment("; moved comment");
    section dst;
    dst = std::move(src);

    REQUIRE(dst.comment().view().size() == 1);
    REQUIRE(dst.comment().view()[0] == "; moved comment");
  }

  SECTION("Self move assignment is safe")
  {
    section sec;
    sec.add_comment("; self-move test");
    sec = std::move(sec);  // technically UB if not guarded internally

    // 要求实现中做了自移动保护（例如 if (this != &other)）
    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; self-move test");
  }
}

TEST_CASE("Field and Section swap, self-swap, assignment", "[section][comment][swap]")
{
  using ini::comment;
  using ini::section;

  SECTION("Field swap swaps comments")
  {
    section sec1, sec2;

    sec1["key1"] = "value1";
    sec2["key2"] = "value2";

    sec1["key1"].add_comment("; key1 comment");
    sec2["key2"].add_comment("; key2 comment");

    std::swap(sec1["key1"], sec2["key2"]);

    REQUIRE(sec1["key1"].comment().view().size() == 1);
    REQUIRE(sec1["key1"].comment().view()[0] == "; key2 comment");

    REQUIRE(sec2["key2"].comment().view().size() == 1);
    REQUIRE(sec2["key2"].comment().view()[0] == "; key1 comment");
  }

  SECTION("Self swap for field does not corrupt comment")
  {
    section sec;

    sec["key"].add_comment("; self-swap key comment");
    std::swap(sec["key"], sec["key"]);

    REQUIRE(sec["key"].comment().view().size() == 1);
    REQUIRE(sec["key"].comment().view()[0] == "; self-swap key comment");
  }

  SECTION("Self swap does not corrupt section comment")
  {
    section sec;
    sec.add_comment("; self-swap section comment");

    std::swap(sec, sec);

    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; self-swap section comment");
  }

  SECTION("Self assignment for field does not corrupt comment")
  {
    section sec;
    sec["key"].add_comment("; self-assign field comment");
    sec["key"] = sec["key"];

    REQUIRE(sec["key"].comment().view().size() == 1);
    REQUIRE(sec["key"].comment().view()[0] == "; self-assign field comment");
  }

  SECTION("Self assignment for section does not corrupt comment")
  {
    section sec;
    sec.add_comment("; self-assign section comment");
    sec = sec;

    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; self-assign section comment");
  }

  SECTION("Move assignment for field preserves comment")
  {
    section sec1, sec2;

    sec1["key1"] = "value1";
    sec1["key1"].add_comment("; moved field comment");

    sec2 = std::move(sec1);

    REQUIRE(sec1.size() == 0);  // sec1 should be empty after move
    REQUIRE(sec2.size() == 1);
    REQUIRE(sec2.at("key1").as<std::string>() == "value1");

    REQUIRE(sec2.at("key1").comment().view().size() == 1);
    REQUIRE(sec2.at("key1").comment().view()[0] == "; moved field comment");
  }

  SECTION("Move assignment for section preserves comment")
  {
    section sec1, sec2;

    sec1.add_comment("; moved section comment");
    sec2 = std::move(sec1);

    REQUIRE(sec2.comment().view().size() == 1);
    REQUIRE(sec2.comment().view()[0] == "; moved section comment");
  }

  SECTION("Self move assignment for field is safe")
  {
    section sec;
    sec["key"].add_comment("; self-move key comment");
    sec["key"] = std::move(sec["key"]);

    REQUIRE(sec["key"].comment().view().size() == 1);
    REQUIRE(sec["key"].comment().view()[0] == "; self-move key comment");
  }

  SECTION("Self move assignment for section is safe")
  {
    section sec;
    sec.add_comment("; self-move section comment");
    sec = std::move(sec);

    REQUIRE(sec.comment().view().size() == 1);
    REQUIRE(sec.comment().view()[0] == "; self-move section comment");
  }
}

TEST_CASE("test move assignment of section", "[basic_section]")
{
  ini::section sec1;
  sec1["key1"] = ini::field("value1");

  // Move sec1 to sec2
  ini::section sec2;
  sec2 = std::move(sec1);  // 触发移动赋值

  REQUIRE(sec2["key1"].as<std::string>() == "value1");
  REQUIRE(sec1["key1"].as<std::string>().empty());  // sec1 已经被清空
}

TEST_CASE("test move constructor of section", "[basic_section]")
{
  ini::section sec1;
  sec1["key1"] = ini::field("value1");

  // Move construct sec2 from sec1
  ini::section sec2(std::move(sec1));

  REQUIRE(sec2["key1"].as<std::string>() == "value1");
  REQUIRE(sec1["key1"].as<std::string>().empty());  // sec1 已经被清空
}

TEST_CASE("Test comments in basic_inifile", "[ini][comments]")
{
  ini::inifile inifile;

  SECTION("Add comment to section")
  {
    // 添加一个 section，并设置一个注释
    inifile["section1"].add_comment("; This is a section comment");

    // 检查 section 的注释
    REQUIRE(inifile["section1"].comment().view().size() == 1);
    REQUIRE(inifile["section1"].comment().view()[0] == "; This is a section comment");
  }

  SECTION("Add comment to field")
  {
    // 在 section 中添加一个字段并附加注释
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; This is a field comment");

    // 检查字段的注释
    REQUIRE(inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(inifile["section1"]["key1"].comment().view()[0] == "; This is a field comment");
  }

  SECTION("Move assignment transfers field comment")
  {
    ini::inifile new_inifile;

    // 设置一个字段并附加注释
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; This is a field comment");

    // 移动到新对象
    new_inifile = std::move(inifile);

    // 检查新对象的注释是否正确转移
    REQUIRE(new_inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(new_inifile["section1"]["key1"].comment().view()[0] == "; This is a field comment");
  }

  SECTION("Comments are cleared after move assignment")
  {
    ini::inifile new_inifile;

    // 设置一个字段并附加注释
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; This is a field comment");

    // 移动到新对象
    new_inifile = std::move(inifile);

    // 检查原对象中的注释是否已清空
    REQUIRE(inifile["section1"]["key1"].comment().view().empty());
  }

  SECTION("Write and read ini file with comments")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 将 ini 数据保存到字符串
    std::string ini_data = inifile.to_string();

    // 清空 inifile，并从字符串加载数据
    ini::inifile loaded_inifile;
    loaded_inifile.from_string(ini_data);

    // 检查注释是否正确加载
    REQUIRE(loaded_inifile["section1"].comment().view().size() == 1);
    REQUIRE(loaded_inifile["section1"].comment().view()[0] == "; Section comment");
    REQUIRE(loaded_inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(loaded_inifile["section1"]["key1"].comment().view()[0] == "; Field comment");
  }

  SECTION("Remove section and check if comments are removed")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 删除 section
    inifile.remove("section1");

    // 检查 section 是否被移除，注释应被清除
    REQUIRE_THROWS_AS(inifile.at("section1"), std::out_of_range);
  }

  SECTION("Write and read ini file with comments")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 将 ini 数据保存到字符串
    std::string ini_data = inifile.to_string();

    // 清空 inifile，并从字符串加载数据
    ini::inifile loaded_inifile;
    loaded_inifile.from_string(ini_data);

    // 检查注释是否正确加载
    REQUIRE(loaded_inifile["section1"].comment().view().size() == 1);
    REQUIRE(loaded_inifile["section1"].comment().view()[0] == "; Section comment");
    REQUIRE(loaded_inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(loaded_inifile["section1"]["key1"].comment().view()[0] == "; Field comment");
  }

  SECTION("Swap two ini files with comments")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 将 ini 数据保存到字符串
    std::string ini_data = inifile.to_string();

    // 清空 inifile2，并从字符串加载数据
    ini::inifile inifile2;
    inifile2.from_string(ini_data);

    // 执行 swap 操作
    inifile.swap(inifile2);

    // 检查 swap 后的内容
    REQUIRE(inifile["section1"].comment().view().size() == 1);
    REQUIRE(inifile["section1"].comment().view()[0] == "; Section comment");
    REQUIRE(inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(inifile["section1"]["key1"].comment().view()[0] == "; Field comment");
  }

  SECTION("Copy ini file with comments")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 使用拷贝构造
    ini::inifile copied_inifile = inifile;

    // 检查拷贝后的内容
    REQUIRE(copied_inifile["section1"].comment().view().size() == 1);
    REQUIRE(copied_inifile["section1"].comment().view()[0] == "; Section comment");
    REQUIRE(copied_inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(copied_inifile["section1"]["key1"].comment().view()[0] == "; Field comment");
  }

  SECTION("Move ini file with comments")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 使用移动构造
    ini::inifile moved_inifile = std::move(inifile);

    // 检查移动后的内容
    REQUIRE(moved_inifile["section1"].comment().view().size() == 1);
    REQUIRE(moved_inifile["section1"].comment().view()[0] == "; Section comment");
    REQUIRE(moved_inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(moved_inifile["section1"]["key1"].comment().view()[0] == "; Field comment");

    // 确保原 inifile 为空
    REQUIRE(inifile.size() == 0);
  }

  SECTION("Move assignment for ini file with comments")
  {
    // 添加注释和字段
    inifile["section1"].add_comment("; Section comment");
    inifile["section1"]["key1"] = "value1";
    inifile["section1"]["key1"].add_comment("; Field comment");

    // 使用移动赋值
    ini::inifile moved_inifile;
    moved_inifile = std::move(inifile);

    // 检查移动赋值后的内容
    REQUIRE(moved_inifile["section1"].comment().view().size() == 1);
    REQUIRE(moved_inifile["section1"].comment().view()[0] == "; Section comment");
    REQUIRE(moved_inifile["section1"]["key1"].comment().view().size() == 1);
    REQUIRE(moved_inifile["section1"]["key1"].comment().view()[0] == "; Field comment");

    // 确保原 inifile 为空
    REQUIRE(inifile.size() == 0);
  }
}

TEST_CASE("INI file comment preservation across copy, move, and swap")
{
  // 构造原始 ini 文件
  ini::inifile original;
  original["database"].add_comment("DB section");
  original["database"]["host"] = "localhost";
  original["database"]["host"].add_comment("; DB host");
  original["database"]["port"] = 5432;
  original["database"]["port"].add_comment("DB port");

  // 验证 to_string 和 from_string
  std::string ini_str = original.to_string();
  ini::inifile from_text;
  from_text.from_string(ini_str);

  REQUIRE(from_text["database"]["host"].as<std::string>() == "localhost");
  REQUIRE(from_text["database"]["host"].comment().view()[0] == "; DB host");

  // 拷贝构造
  ini::inifile copied = from_text;
  REQUIRE(copied["database"]["port"].as<std::string>() == "5432");
  REQUIRE(copied["database"]["port"].comment().view()[0] == "; DB port");

  // 拷贝赋值
  ini::inifile assigned;
  assigned = copied;
  REQUIRE(assigned["database"]["host"].as<std::string>() == "localhost");
  REQUIRE(assigned["database"]["host"].comment().view()[0] == "; DB host");

  // 移动构造
  ini::inifile moved = std::move(assigned);
  REQUIRE(moved["database"]["port"].as<std::string>() == "5432");
  REQUIRE(moved["database"]["port"].comment().view()[0] == "; DB port");

  // 移动赋值
  ini::inifile moved2;
  moved2 = std::move(moved);
  REQUIRE(moved2["database"]["host"].as<std::string>() == "localhost");
  REQUIRE(moved2["database"]["host"].comment().view()[0] == "; DB host");

  // swap 两个不同的数据
  ini::inifile another;
  another["server"].add_comment("; server section");
  another["server"]["ip"] = "10.0.0.1";
  another["server"]["ip"].add_comment("; server ip");

  moved2.swap(another);

  // 验证 swap 后的数据内容
  REQUIRE(moved2["server"]["ip"].as<std::string>() == "10.0.0.1");
  REQUIRE(moved2["server"]["ip"].comment().view()[0] == "; server ip");
  REQUIRE(another["database"]["port"].as<std::string>() == "5432");
  REQUIRE(another["database"]["port"].comment().view()[0] == "; DB port");
}

TEST_CASE("INI file save and load with numeric fields and comments")
{
  const std::string path = "test_numeric.ini";

  // 构造 ini 数据
  ini::inifile out;
  out["numbers"].add_comment("numeric section");
  out["numbers"]["int_val"] = 42;
  out["numbers"]["int_val"].add_comment("int value");

  out["numbers"]["float_val"] = 3.14f;
  out["numbers"]["float_val"].add_comment("float value", '#');

  out["numbers"]["double_val"] = 2.718281828;
  out["numbers"]["double_val"].add_comment("double value");

  // 保存到文件
  REQUIRE_NOTHROW(out.save(path));

  // 从文件读取
  ini::inifile in;
  REQUIRE_NOTHROW(in.load(path));

  // 验证值是否保持一致
  REQUIRE(in["numbers"]["int_val"].as<int>() == 42);
  REQUIRE(in["numbers"]["float_val"].as<float>() == Approx(3.14f));
  REQUIRE(in["numbers"]["double_val"].as<double>() == Approx(2.718281828));

  // 验证注释
  REQUIRE(in["numbers"].comment().view()[0] == "; numeric section");
  REQUIRE(in["numbers"]["int_val"].comment().view()[0] == "; int value");
  REQUIRE(in["numbers"]["float_val"].comment().view()[0] == "# float value");
  REQUIRE(in["numbers"]["double_val"].comment().view()[0] == "; double value");

  // 清理临时文件
  std::remove(path.c_str());
}

TEST_CASE("INI file comment with special characters", "[ini][comments][special]")
{
  ini::inifile inifile;

  // 添加特殊字符的注释
  inifile["section1"].add_comment("; Special characters: !@#$%^&*()_+");
  inifile["section1"]["key1"] = "value1";
  inifile["section1"]["key1"].add_comment("; Special chars in field: ~`|\\{}[]:\";'<>?,./");

  // 检查注释是否正确
  REQUIRE(inifile["section1"].comment().view()[0] == "; Special characters: !@#$%^&*()_+");
  REQUIRE(inifile["section1"]["key1"].comment().view()[0] == "; Special chars in field: ~`|\\{}[]:\";'<>?,./");
}
TEST_CASE("INI file comment with empty and whitespace-only lines", "[ini][comments][whitespace]")
{
  ini::inifile inifile;

  // 添加空行和仅包含空格的行
  inifile["section1"].add_comment("; ");
  inifile["section1"].add_comment(";    ");
  inifile["section1"]["key1"] = "value1";
  inifile["section1"]["key1"].add_comment("; \t \n");

  // 检查注释是否正确
  REQUIRE(inifile["section1"].comment().view()[0] == ";");
  REQUIRE(inifile["section1"].comment().view()[1] == ";");
  REQUIRE(inifile["section1"]["key1"].comment().view()[0] == ";");
}
TEST_CASE("INI file comment with non-ASCII characters", "[ini][comments][non-ascii]")
{
  ini::inifile inifile;

  // 添加非 ASCII 字符的注释
  inifile["section1"].add_comment("; 中文注释");
  inifile["section1"]["key1"] = "value1";
  inifile["section1"]["key1"].add_comment("; 日本語のコメント");

  // 检查注释是否正确
  REQUIRE(inifile["section1"].comment().view()[0] == "; 中文注释");
  REQUIRE(inifile["section1"]["key1"].comment().view()[0] == "; 日本語のコメント");
}

TEST_CASE("Copy and move inifile with comments and other operations", "[ini][copy][move][comments]")
{
  SECTION("Copy and move inifile with comments")
  {
    ini::inifile a;
    a["s"].add_comment("; copied section");
    a["s"]["k"] = 1;
    a["s"]["k"].add_comment("; copied key");

    ini::inifile b(a);              // 拷贝构造
    ini::inifile c = std::move(a);  // 移动构造

    REQUIRE(b["s"].comment().view()[0] == "; copied section");
    REQUIRE(b["s"]["k"].comment().view()[0] == "; copied key");

    REQUIRE(c["s"].comment().view()[0] == "; copied section");
    REQUIRE(c["s"]["k"].comment().view()[0] == "; copied key");
  }
  SECTION("Swap inifile with comments")
  {
    ini::inifile a, b;

    a["x"].add_comment("; comment a");
    a["x"]["ka"] = 1;
    a["x"]["ka"].add_comment("; field a");

    b["y"].add_comment("; comment b");
    b["y"]["kb"] = 2;
    b["y"]["kb"].add_comment("; field b");

    swap(a, b);

    REQUIRE(a["y"].comment().view()[0] == "; comment b");
    REQUIRE(a["y"]["kb"].comment().view()[0] == "; field b");

    REQUIRE(b["x"].comment().view()[0] == "; comment a");
    REQUIRE(b["x"]["ka"].comment().view()[0] == "; field a");
  }
  SECTION("INI without comments should not crash")
  {
    ini::inifile ini;
    ini["s"]["k"] = 123;

    std::string str = ini.to_string();
    ini::inifile loaded;
    loaded.from_string(str);

    REQUIRE(loaded["s"]["k"].as<int>() == 123);
    REQUIRE(loaded["s"].comment().view().empty());
    REQUIRE(loaded["s"]["k"].comment().view().empty());
  }
  SECTION("Multi-line comment round-trip")
  {
    ini::inifile ini;
    ini["sec"].comment().add("; first line");
    ini["sec"].comment().add("; second line");

    ini["sec"]["key"].comment().add("; k1");
    ini["sec"]["key"].comment().add("; k2");

    std::string str = ini.to_string();

    ini::inifile loaded;
    loaded.from_string(str);

    const auto &sec_comments = loaded["sec"].comment().view();
    REQUIRE(sec_comments.size() == 2);
    REQUIRE(sec_comments[0] == "; first line");
    REQUIRE(sec_comments[1] == "; second line");

    const auto &field_comments = loaded["sec"]["key"].comment().view();
    REQUIRE(field_comments.size() == 2);
    REQUIRE(field_comments[0] == "; k1");
    REQUIRE(field_comments[1] == "; k2");
  }
  SECTION("Comments with special characters")
  {
    ini::inifile ini;
    ini["a"].comment().add("; semicolon");
    ini["a"].comment().add("# hash", '#');
    ini["a"].comment().add("    ; leading spaces");

    std::string str = ini.to_string();
    ini::inifile loaded;
    loaded.from_string(str);

    REQUIRE(loaded["a"].comment().view().size() == 3);
    REQUIRE(loaded["a"].comment().view()[1] == "# hash");
  }
  SECTION("Comment add without needing prefix")
  {
    ini::inifile ini;
    ini["s"].comment().add("section comment");
    ini["s"]["k"] = 1;
    ini["s"]["k"].comment().add("field comment");

    std::string str = ini.to_string();

    // 检查生成的字符串中自动加上了 "; "
    REQUIRE(str.find("; section comment") != std::string::npos);
    REQUIRE(str.find("; field comment") != std::string::npos);
  }
  SECTION("Avoid double comment prefix")
  {
    ini::inifile ini;
    ini["a"].comment().add("; already has semicolon");
    ini["b"].comment().add("# already has hash");

    std::string str = ini.to_string();

    // 不应该变成 ;; 或 ;# 开头
    REQUIRE(str.find(";;") == std::string::npos);
    REQUIRE(str.find(";#") == std::string::npos);
  }
}

TEST_CASE("comment constructors behave correctly and unambiguously", "[comment][constructor]")
{
  using ini::comment;

  SECTION("Construct from std::string with default symbol")
  {
    std::string input = "line one\nline two";
    comment c(input);  // uses ';' as default
    auto view = c.view();

    REQUIRE(view.size() == 2);
    REQUIRE(view[0] == "; line one");
    REQUIRE(view[1] == "; line two");
  }

  SECTION("Construct from std::string with custom symbol '#'")
  {
    std::string input = "a\nb";
    comment c(input, '#');
    auto view = c.view();

    REQUIRE(view.size() == 2);
    REQUIRE(view[0] == "# a");
    REQUIRE(view[1] == "# b");
  }

  SECTION("Construct from std::vector<std::string>")
  {
    std::vector<std::string> vec = {"hello", "world"};
    comment c(vec);  // uses default symbol ';'
    auto view = c.view();

    REQUIRE(view.size() == 2);
    REQUIRE(view[0] == "; hello");
    REQUIRE(view[1] == "; world");
  }

  SECTION("Construct from std::vector<std::string> with '#'")
  {
    std::vector<std::string> vec = {"foo", "bar"};
    comment c(vec, '#');
    auto view = c.view();

    REQUIRE(view.size() == 2);
    REQUIRE(view[0] == "# foo");
    REQUIRE(view[1] == "# bar");
  }

  SECTION("Construct from initializer_list<std::string> (non-explicit)")
  {
    comment c{"init1", "init2"};  // should match initializer_list overload
    auto view = c.view();

    REQUIRE(view.size() == 2);
    REQUIRE(view[0] == "; init1");
    REQUIRE(view[1] == "; init2");
  }

  SECTION("Construct from initializer_list with custom symbol")
  {
    comment c({"x", "y"}, '#');
    auto view = c.view();

    REQUIRE(view.size() == 2);
    REQUIRE(view[0] == "# x");
    REQUIRE(view[1] == "# y");
  }

  SECTION("Ambiguity test: string literal should not match initializer_list constructor")
  {
    const char *str = "single line";
    std::string s{str};
    comment c(s);  // should use std::string constructor, not initializer_list

    auto view = c.view();
    REQUIRE(view.size() == 1);
    REQUIRE(view[0] == "; single line");
  }

  SECTION("Empty input produces empty comment")
  {
    comment c1(std::string{});
    REQUIRE(c1.empty());

    comment c2(std::vector<std::string>{});
    REQUIRE(c2.empty());

    comment c3({});
    REQUIRE(c3.empty());
    comment c("some string");
  }
}

TEST_CASE("field: construct from string and convert to types", "[field][conversion]")
{
  ini::field f("42");

  REQUIRE(f.as<int>() == 42);
  REQUIRE(f.as<std::string>() == "42");

  double d = 0.0;
  f.as_to(d);
  REQUIRE(d == Approx(42.0));
}
TEST_CASE("field: invalid conversion throws exception", "[field][conversion][exception]")
{
  ini::field f("not_a_number");

  REQUIRE_THROWS_AS(f.as<int>(), std::invalid_argument);
}
TEST_CASE("field: copy constructor and assignment", "[field][copy]")
{
  ini::field f1("abc");
  f1.set_comment("hello");

  ini::field f2 = f1;  // copy constructor
  REQUIRE(f2.as<std::string>() == "abc");
  REQUIRE(f2.comment().view() == f1.comment().view());

  ini::field f3;
  f3 = f1;  // copy assignment
  REQUIRE(f3.as<std::string>() == "abc");
  REQUIRE(f3.comment().view() == f1.comment().view());
}

TEST_CASE("field: as_to converts string to target type correctly", "[field][as_to]")
{
  ini::field f_int("123");
  int out_int = 0;
  int &ref_int = f_int.as_to(out_int);
  REQUIRE(ref_int == 123);
  REQUIRE(&ref_int == &out_int);  // 返回引用必须是传入的引用

  ini::field f_double("3.14159");
  double out_double = 0.0;
  double &ref_double = f_double.as_to(out_double);
  REQUIRE(ref_double == Approx(3.14159));
  REQUIRE(&ref_double == &out_double);

  ini::field f_str("hello");
  std::string out_str;
  std::string &ref_str = f_str.as_to(out_str);
  REQUIRE(ref_str == "hello");
  REQUIRE(&ref_str == &out_str);
}

TEST_CASE("field: as_to throws on invalid conversion", "[field][as_to][exception]")
{
  ini::field f("not_a_number");

  int out_int = 0;
  REQUIRE_THROWS_AS(f.as_to(out_int), std::invalid_argument);

  double out_double = 0.0;
  REQUIRE_THROWS_AS(f.as_to(out_double), std::invalid_argument);
}

struct MyType
{
  int id;
  int age;
  std::string name;

  bool operator==(const MyType &other) const
  {
    return id == other.id && age == other.age && name == other.name;
  }
};

template <>
struct INIFILE_TYPE_CONVERTER<MyType>
{
  // Encode MyType object into a string like "id,age,name"
  static void encode(const MyType &obj, std::string &value)
  {
    value = std::to_string(obj.id) + "," + std::to_string(obj.age) + "," + obj.name;
  }

  // Decode string "id,age,name" into MyType object
  static void decode(const std::string &value, MyType &obj)
  {
    size_t pos1 = value.find(',');
    size_t pos2 = value.find(',', pos1 + 1);

    if (pos1 == std::string::npos || pos2 == std::string::npos)
    {
      throw std::invalid_argument("Invalid format for MyType decoding");
    }

    obj.id = std::stoi(value.substr(0, pos1));
    obj.age = std::stoi(value.substr(pos1 + 1, pos2 - pos1 - 1));
    obj.name = value.substr(pos2 + 1);
  }
};

TEST_CASE("field: as_to converts to MyType correctly", "[field][as_to][MyType]")
{
  ini::inifile f;

  // 先用 encode 编码一个字符串，赋值给 field 的 value_
  MyType original{42, 30, "Tom"};
  f["key"]["value"] = original;  // 使用 INIFILE_TYPE_CONVERTER<MyType> 自动转换

  // 测试 as_to 转换
  MyType result{};
  MyType t1 = f["key"]["value"];
  REQUIRE(t1 == original);  // 确认转换正确

  auto t2 = f["key"]["value"].as<MyType>();
  REQUIRE(t2 == original);  // 确认 as<MyType> 转换正确

  MyType &ref = f["key"]["value"].as_to(result);
  REQUIRE(ref.id == original.id);
  REQUIRE(ref.age == original.age);
  REQUIRE(ref.name == original.name);

  // 确认返回的引用就是传入的 result 对象
  REQUIRE(&ref == &result);
}
