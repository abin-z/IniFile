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