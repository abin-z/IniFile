#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <inifile/inifile.h>

TEST_CASE("basic test")
{
  REQUIRE(1 + 1 == 2);
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
  TestType x = 3.14159;
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
                      double v2 = file["section"]["key"]; // out_of_range
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
  REQUIRE(file.has("section") == true);
  REQUIRE(file.has("section", "key") == true);
  REQUIRE(file.has("section_no") == false);
  REQUIRE(file.has("section_no", "key") == false);
  REQUIRE(file.has("section", "key_no") == false);
  REQUIRE(file["section"].size() == 1);
  REQUIRE(file["section"]["key"].as<std::string>() == std::string("hello world"));
  REQUIRE_FALSE(file.has("section") != true);
}