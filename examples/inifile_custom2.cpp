/*
 *  Sample: vector_type_conversion_demo
 *  -----------------------------------
 *  Demonstrates how to specialize INIFILE_TYPE_CONVERTER for std::vector<T>
 *  to support automatic serialization and deserialization of vectors
 *  to/from delimited strings in ini::inifile.
 *
 *  示例：std::vector<T> 类型转换演示
 *  ------------------------------
 *  演示如何为 std::vector<T> 特化 INIFILE_TYPE_CONVERTER，
 *  支持 vector 与字符串的自动序列化和反序列化，
 *  方便通过 ini::inifile 读写vector数据。
 */

#include <inifile/inifile.h>

/// @brief Specialization of INIFILE_TYPE_CONVERTER for std::vector<T>.
/// @tparam T The type of elements in the vector.
template <typename T>
struct INIFILE_TYPE_CONVERTER<std::vector<T>>
{
  /// @brief Encodes a vector into a delimited string.
  void encode(const std::vector<T> &vec, std::string &value)
  {
    constexpr char delimiter = ',';  // Delimiter used to separate elements in the string
    std::string encoded_item;
    for (const auto &v : vec)
    {
      INIFILE_TYPE_CONVERTER<T> conv;
      conv.encode(v, encoded_item);       // Encode each element of the vector
      value += encoded_item + delimiter;  // Append the encoded element and delimiter
    }
    if (value.back() == delimiter) value.pop_back();  // Remove the trailing delimiter
  }

  /// @brief Decodes a delimited string into a vector.
  void decode(const std::string &value, std::vector<T> &vec)
  {
    constexpr char delimiter = ',';                                // Delimiter used to separate elements in the string
    std::vector<std::string> info = ini::split(value, delimiter);  // Split the string
    T encoded_item;
    for (const auto &v : info)
    {
      INIFILE_TYPE_CONVERTER<T> conv;
      conv.decode(v, encoded_item);               // Decode each part of the string
      vec.emplace_back(std::move(encoded_item));  // Add the decoded element to the vector
    }
  }
};

/// @brief Print the contents of a vector to the console.
template <typename T>
void print_vector(const std::vector<T> &vec)
{
  std::cout << "[";
  for (size_t i = 0; i < vec.size(); ++i)
  {
    std::cout << vec[i];
    if (i != vec.size() - 1)
    {
      std::cout << ", ";
    }
  }
  std::cout << "]" << std::endl;
}

int main()
{
  ini::inifile inif;  // Create an INI file object

  // Define vectors of different types
  std::vector<int>         vec1 = {1, 2, 3, 4, 5};
  std::vector<double>      vec2 = {1.1111, 2.2222, 3.3333, 4.4444, 5.5555};
  std::vector<std::string> vec3 = {"aaa", "bbb", "ccc", "ddd", "eee"};

  // Set different types of vectors in the INI file object
  inif["section"]["key1"] = vec1;
  inif["section"]["key2"] = vec2;
  inif["section"]["key3"] = vec3;

  // Get different vectors from INI file object
  std::vector<int>         v1 = inif["section"]["key1"];
  std::vector<double>      v2 = inif["section"]["key2"];
  std::vector<std::string> v3 = inif["section"]["key3"];

  // Print the vectors to the console
  print_vector(v1);
  print_vector(v2);
  print_vector(v3);

  // Print the entire INI file content as a string
  std::cout << "ini info:\n" << inif.to_string() << std::endl;

  return 0;  // Exit the program
}