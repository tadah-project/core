#include "catch2/catch.hpp"
#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <dirent.h>

using namespace tadah::core;

TEST_CASE( "Testing Context read()", "[config_read]" ) {
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(CoreUserConfigParser().parse(path+"/"+filename));
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  path = "tests_data/invalid_configs";
  dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        std::cerr << "Testing: " << filename << std::endl;
        REQUIRE_THROWS(CoreUserConfigParser().parse(path+"/"+filename));
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  Context c = CoreUserConfigParser().parse("tests_data/valid_configs/valid_config1");
  REQUIRE(c.size("DBFILE")==4);

  REQUIRE_THROWS(CoreUserConfigParser().parse("some_random_non_existing_config"));
}
TEST_CASE( "Testing Context add", "[config]" ) {
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(CoreUserConfigParser().parse(path+"/"+filename));
        Context c1 = CoreUserConfigParser().parse(path+"/"+filename);
        Context c2 = CoreUserConfigParser().parse(path+"/"+filename);
        REQUIRE(c1==c2);
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }
}
TEST_CASE("read() with empty lines and comments") {
    // We'll create a temporary file containing some blank lines and commented lines
    const char* filename = "test_empty_comment.cfg";
    {
        std::ofstream ofs(filename);
        ofs << "\n";               // empty line
        ofs << "# This is a comment line\n";
        ofs << "INIT2B true   # key plus inline comment\n";
        ofs << "\n";               // another blank line
    }

    // Now read it
    Context c = CoreUserConfigParser().parse(filename);
    // We expect "INIT2B true" was read
    REQUIRE(c.get<bool>("INIT2B") == true);

    // Cleanup
    std::remove(filename);
}
