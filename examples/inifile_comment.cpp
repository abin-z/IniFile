#include <inifile/inifile.h>

int main()
{
  constexpr char path[] = "comment.ini";
  ini::inifile inif;

  // Basic key-value assignment
  inif["section"]["key"] = "value";
  inif["section"]["flag"] = true;

  inif["database"]["host"] = "localhost";
  inif["database"]["port"] = 3306;
  inif["database"]["username"] = "admin";

  inif["network"]["ip"] = "127.0.0.1";
  inif["network"]["port"] = 1024;
  inif["network"]["timeout"] = 30;

  // Set section comments
  inif["database"].set_comment("comment about database section", '#');
  inif["network"].add_comment("network config"); // equivalent to comment().add(...)

  // Set single-line key comments
  inif["database"]["host"].set_comment("database host");
  inif["database"]["port"].set_comment("database port");
  inif.at("database").at("username").set_comment("database username");

  // Append more comment lines
  inif["section"]["key"].add_comment("Extra comment line1.");
  inif["section"]["key"].comment().add("Extra comment line2.");

  // Set multi-line section and key comments 
  inif["section"].set_comment("section-comment line1\nsection-comment line2\nsection-comment line3", '#');

  inif["section"]["key"].comment().add({
    "Main key for the section.",
    "Can be any string value.",
    "Used in test cases."
  });
  inif["section"]["key"].comment().add({"Another one.\nFinal line."});

  // Clear comments
  inif["network"]["ip"].clear_comment();      // clear via shortcut
  inif["network"]["port"].comment().clear();  // clear via comment object

  // Access comment object
  ini::comment &cmt = inif["section"]["key"].comment();  // get reference to comment

  // Read comment content
  const std::vector<std::string> &view = inif["section"]["key"].comment().view();       // view (non-owning)
  std::vector<std::string>     cmt_vtr = inif["section"]["key"].comment().to_vector();  // deep copy

  // Save to file
  inif.save(path);

  // Load from file and print
  ini::inifile loadinif;
  loadinif.load(path);
  std::cout << loadinif.to_string() << std::endl;
}
