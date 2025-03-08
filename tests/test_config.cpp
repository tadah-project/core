#include "catch2/catch.hpp"
#include <tadah/core/config.h>

#include <dirent.h>
#include <sys/types.h>
#include <sstream>


TEST_CASE( "Testing Empty Config", "[config_empty]" ) {
  Config c1;
  Config c2;
  REQUIRE(c1==c2);
}

TEST_CASE( "Testing Config add()", "[config_add]" ) {
  Config c1;
  SECTION("Add internal") {
    c1.add("SIZE2B",10);
  }
  SECTION("Add invalid key name") {
    REQUIRE_THROWS(c1.add("SIZE2BX",10));
  }
  SECTION("Add invalid/empty key name") {
    REQUIRE_THROWS(c1.add("",10));
  }
  SECTION("Add the same key twice when not allowed case 1") {
    c1.add("SIZE2B",10);
    REQUIRE_THROWS(c1.add("SIZE2B",11));
  }
  SECTION("Add the same key twice when not allowed case 2") {
    c1.add("MODEL","M_BLR");
    c1.add("MODEL","BF_Linear");
    c1.add("MODEL","EXTRA_ALLOWED_FLAG");
    REQUIRE_THROWS(c1.add("MODEL","M_BLR"));
    REQUIRE_THROWS(c1.add("MODEL","ANYTHING"));
  }
  SECTION("Add allowed multiple keys") {
    c1.add("DBFILE", "f1");
    c1.add("DBFILE", "f2");
    c1.add("DBFILE", "f3");
  }
  SECTION("Add allowed negative multiple keys") {
    // Multiple negative allowed key
    c1.add("CGRID2B", 1);
    c1.add("CGRID2B", 2);
    c1.add("CGRID2B", 3);
    // Test various containers
    c1.add("CGRID2B", std::vector<double>{4,5,6});
    c1.add("CGRID2B", std::vector<int>{4,5,6});
    REQUIRE(c1.size("CGRID2B")==9);
  }
}
TEST_CASE( "Testing Config remove()", "[config_remove]" ) {
  Config c1;
  // Single key allowed
  c1.add("SIZE2B",10);
  c1.add("SSTDEV",std::vector<double>{1,2,3,4,5,6});

  // Multiple allowed key
  c1.add("DBFILE", "f1");
  c1.add("DBFILE", "f2");
  c1.add("DBFILE", "f3");

  // Multiple negative allowed key
  c1.add("CGRID2B", 1);
  c1.add("CGRID2B", 2);
  c1.add("CGRID2B", 3);
  c1.add("CGRID2B", std::vector<double>{4,5,6});

  SECTION("Remove valid key name") {
    REQUIRE(c1.remove("SIZE2B")==1);
  }
  SECTION("Remove valid key name twice") {
    // init2b is set by default
    REQUIRE(c1.remove("INIT2B")==1);
    REQUIRE(c1.remove("INIT2B")==0);

    REQUIRE(c1.remove("SIZE2B")==1);
    REQUIRE(c1.remove("SIZE2B")==0);

    REQUIRE(c1.remove("SSTDEV")==1);
    REQUIRE(c1.remove("SSTDEV")==0);
  }
  SECTION("Remove multiple key name twice") {
    REQUIRE(c1.remove("DBFILE")==1);
    REQUIRE(c1.remove("DBFILE")==0);
  }
  SECTION("Remove multiple negative key name twice") {
    REQUIRE(c1.remove("CGRID2B")==1);
    REQUIRE(c1.remove("CGRID2B")==0);
  }
}
TEST_CASE( "Testing Config size()", "[config_size]" ) {
  Config c;
  REQUIRE(c.size("INIT2B")==1);
  REQUIRE(c.size("INIT3B")==1);
  REQUIRE(c.size("INITMB")==1);
  REQUIRE_THROWS(c.size("SIZE2B"));
  c.add("SIZE2B",12);
  REQUIRE(c.size("SIZE2B")==1);
  c.remove("SIZE2B");
  REQUIRE_THROWS(c.size("SIZE2B"));
}
TEST_CASE( "Testing Config exist()", "[config_exist]" ) {
  Config c;
  REQUIRE(c.exist("INIT2B"));
  REQUIRE(c.exist("INIT3B"));
  REQUIRE(c.exist("INITMB"));
}
TEST_CASE( "Testing Config clear_internal_keys()", "[config_clear_internal_keys]" ) {
  Config c1;
  Config c2;
  SECTION("Clear empty config") {
    c2.clear_internal_keys();
    REQUIRE(c1==c2);
  }
  SECTION("Clear modified config") {
    c2.add("DSIZE",10);
    c2.add("SIZE2B",4);
    c2.add("SIZE3B",3);
    c2.add("SIZEMB",3);
    c2.add("ESTDEV",0.1444);
    c2.add("FSTDEV",0.1444);
    c2.add("SSTDEV",std::vector<double>{1,2,3,4,5,6});
    c2.clear_internal_keys();
    REQUIRE(c1==c2);
  }
}
TEST_CASE( "Testing Config operator()", "[config_operator()]" ) {
  std::string s1 = "f1/f1/f1";
  std::string s2 = "f2/f2/f2";
  std::string s3 = "f3/f3/f3";
  Config c;
  c.add("DBFILE",s1);
  c.add("DBFILE",s2);
  c.add("DBFILE",s3);
  auto v = c("DBFILE");
  REQUIRE(v[0]==s1);
  REQUIRE(v[1]==s2);
  REQUIRE(v[2]==s3);

  // Check some default values
  v = c("INIT2B");
  REQUIRE(v[0]=="false");
  v = c("INIT3B");
  REQUIRE(v[0]=="false");
  v = c("INITMB");
  REQUIRE(v[0]=="false");
}
TEST_CASE( "Testing is Config case sensitive", "[config_case]" ) {
  Config c;
  REQUIRE_THROWS(c.size("sIze2B"));
  c.add("SiZE2b",10);
  REQUIRE(c.exist("SIZE2B"));
  REQUIRE(c.exist("SIze2B"));
  REQUIRE(!(c.exist("SIze2Bx")));
  REQUIRE(!(c.exist(" SIze2B")));
  REQUIRE(c.size("sIze2B")==1);
  REQUIRE(c.remove("Size2b")==1);
}
TEST_CASE( "Testing Config get(k)", "[config_get]" ) {
  Config c;
  std::string s1 = "f1/f1/f1";
  std::string s2 = "f2/f2/f2";
  std::string s3 = "f3/f3/f3";
  c.add("DBFILE",std::vector<std::string>{s1,s2,s3});
  REQUIRE(s1==c.get<std::string>("DBFILE"));

  c.add("SSTDEV",std::vector<int>{1,2,3,4,5,6});
  REQUIRE(c.get<int>("SSTDEV")==1);
  REQUIRE(c.get<int>("SstdEV")==1);
}
TEST_CASE( "Testing Config get(k,v)", "[config_get_arr]" ) {
  Config c;
  std::string s1 = "f1/f1/f1";
  std::string s2 = "f2/f2/f2";
  std::string s3 = "f3/f3/f3";
  c.add("DBFILE",std::vector<std::string>{s1,s2,s3});
  std::vector<std::string> v(3);
  c.get("DBFILE",v);
  REQUIRE(v[0]==c("DBFILE")[0]);
  REQUIRE(v[1]==c("DBFILE")[1]);
  REQUIRE(v[2]==c("DBFILE")[2]);
  REQUIRE(v[2]==c("dbfilE")[2]);

  REQUIRE_NOTHROW(c.get("DBFILE",v));
  REQUIRE_THROWS(c.get("TYPE2B",v));

  c.add("SSTDEV",std::vector<int>{1,2,3,4,5,6});
  std::vector<int> v2(6);
  c.get("SstDEV",v2);
  for (int i=0; i<6; ++i)
    REQUIRE(v2[i]==(i+1));
}
TEST_CASE( "Testing Config check()", "[config]" ) {
  Config c;
  std::string key = "SIZE2B";
  std::string value = "2";
  REQUIRE_THROWS(c.check(key, value));

}
TEST_CASE( "Testing Config check_for_training()", "[config]" ) {

  std::string path = "tests_data/valid_train_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(Config(path+"/"+filename).check_for_training());
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  path = "tests_data/invalid_train_configs";
  dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_THROWS(Config(path+"/"+filename).check_for_training());
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  Config c;
  c.remove("DBFILE");
  c.remove("INIT2B");
  c.remove("TYPE2B");
  c.remove("RCTYPE2B");
  c.remove("RCTYPE2B");
  c.add("DBFILE","a/a/a");
  c.add("INIT2B","true");
  c.add("TYPE2B","D2_Dummy");
  c.add("RCTYPE2B","Cut_Dummy");
  c.add("RCUT2B",10);

  REQUIRE_NOTHROW(c.check_for_training());

  c.remove("DBFILE");
  REQUIRE_THROWS(c.check_for_training());
  c.add("DBFILE","a/a/a");

  c.remove("INIT2B");
  REQUIRE_THROWS(c.check_for_training());
  c.add("INIT2B","true");

  c.remove("TYPE2B");
  REQUIRE_THROWS(c.check_for_training());
  c.add("TYPE2B","D2_Dummy");

  c.remove("RCTYPE2B");
  REQUIRE_THROWS(c.check_for_training());
  c.add("RCTYPE2B","Cut_Dummy");

  c.remove("RCUT2B");
  REQUIRE_THROWS(c.check_for_training());
  c.add("RCUT2B",10);
}
TEST_CASE( "Testing Config read()", "[config_read]" ) {
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(Config(path+"/"+filename));
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
        REQUIRE_THROWS(Config(path+"/"+filename));
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  Config c("tests_data/valid_configs/valid_config1");
  REQUIRE(c.size("DBFILE")==4);

  REQUIRE_THROWS(Config("some_random_non_existing_config"));
}
TEST_CASE( "Testing Config <<", "[config]" ) {
  Config c;
  std::string s1 = "f1/f1/f1";
  std::string s2 = "f2/f2/f2";
  std::string s3 = "f3/f3/f3";
  c.add("DBFILE",std::vector<std::string>{s1,s2,s3});
  std::vector<std::string> v(3);
  c.get("DBFILE",v);
  REQUIRE(v[0]==c("DBFILE")[0]);
  REQUIRE(v[1]==c("DBFILE")[1]);
  REQUIRE(v[2]==c("DBFILE")[2]);
  REQUIRE(v[2]==c("dbfilE")[2]);

  REQUIRE_NOTHROW(c.get("DBFILE",v));
  REQUIRE_THROWS(c.get("TYPE2B",v));

  std::stringstream ss;
  ss<<c;
  std::string space ="     "; // must mathc config.h value
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("DBFILE"+space+s1));
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("DBFILE"+space+s2));
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("DBFILE"+space+s3));

  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("BIAS"+space+"true"));
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("INIT2B"+space+"false"));
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("INIT3B"+space+"false"));
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("INITMB"+space+"false"));
  REQUIRE_THAT(ss.str(), Catch::Matchers::
               Contains("ALPHA"+space+"1"));
}
TEST_CASE( "Testing Config add", "[config]" ) {
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(Config(path+"/"+filename));
        Config c1(Config(path+"/"+filename));
        Config c2;
        c2.clear();
        c2.add(path+"/"+filename);
        REQUIRE(c1==c2);
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }
}


TEST_CASE("Testing Config check_for_predict()", "[config][new]") {
    // check_for_predict() is currently a stub returning true
    // We ensure it does not throw and simply returns true.
    Config c;
    // Possibly set some keys
    c.add("DBFILE", "predict_file");
    // The default function does nothing special, so we only confirm no exceptions:
    REQUIRE_NOTHROW(c.check_for_predict());
    REQUIRE(c.check_for_predict() == true);
}

TEST_CASE("Testing Config check_pot_file()", "[config][new]") {
    // check_pot_file() is also a stub returning true
    // We'll confirm it doesn't throw:
    Config c;
    // Add minimal keys
    c.add("DBFILE", "my_potential_file");
    REQUIRE_NOTHROW(c.check_pot_file());
    REQUIRE(c.check_pot_file() == true);
}

TEST_CASE("Testing Config serialization round-trip", "[config][new]") {
    // Create a config, then remove the default "INIT2B" if it exists, so we can add a new one.
    Config c1;
    c1.remove("INIT2B"); // remove default "INIT2B: false"
    c1.add("INIT2B", true);

    // Now we can safely add DBFILE, etc.
    c1.add("DBFILE", "db_1");
    c1.add("DBFILE", "db_2");
    c1.add("RCUT2B", 5.5);

    // Round-trip serialization
    std::vector<char> buffer = c1.serialize();
    Config c2;
    c2.deserialize(buffer);

    // Check equality
    REQUIRE(c1 == c2);
    REQUIRE(c2.get<bool>("INIT2B") == true);
    REQUIRE(c2.get<double>("RCUT2B") == 5.5);
}

TEST_CASE("Testing Config operator== - partial differences", "[config][new]") {
    Config cA, cB;
    // Both start with default INIT2B, ALPHA, etc.

    // If we want ALPHA to be set manually, remove it from defaults first
    cA.remove("ALPHA");
    cB.remove("ALPHA");

    // Now proceed with your test scenario
    cA.add("DBFILE", "data_a1");
    cB.add("DBFILE", "data_a1");
    REQUIRE(cA == cB); // They match now

    // cB adds extra DBFILE => mismatch
    cB.add("DBFILE", "data_a2");
    REQUIRE_FALSE(cA == cB);

    // remove the extra DBFILE to match cA
    cB.remove("DBFILE");
    cB.add("DBFILE", "data_a1");
    REQUIRE(cA == cB);

    // Now we want to add ALPHA
    cA.add("ALPHA", 1);
    REQUIRE_FALSE(cA == cB);
    cB.add("ALPHA", 1);
    REQUIRE(cA == cB);
}

TEST_CASE("Testing Config with multi-token lines (simulation)", "[config][new]") {
    // We’ll simulate reading lines like "TOKEN val1 val2 val3" by calling add() multiple times.
    // The existing read() method already handles line input, but we can mimic it directly here.

    Config c;
    // "FOO" key is allowed as per config_keys.toml (assuming for demonstration).
    // If it’s not actually in your toml, adjust to a key that *is* allowed for multiple values
    // such as "DBFILE" or "CGRID2B" with negative or multiple allowance.

    c.add("DBFILE", std::vector<std::string>{"val1", "val2", "val3"});
    // Now we check the content
    REQUIRE(c.size("DBFILE") == 3);
    REQUIRE(c("DBFILE")[0] == "val1");
    REQUIRE(c("DBFILE")[1] == "val2");
    REQUIRE(c("DBFILE")[2] == "val3");

    // If you want to confirm that read() also handles multiple tokens on one line,
    // place an actual test config file under valid_configs or invalid_configs containing:
    //   DBFILE val1 val2 val3
    // and run your existing "Testing Config read()".
}


TEST_CASE("Config get<T>(key, T&) - parsing failure triggers catch(...) [lines 96-97]", "[config][coverage]") {
    Config c;
    // 1) Add a user key that can hold multiple values, e.g. "DBFILE"
    //    declared as "value_N=-1" or some multiple in config_keys.toml
    //    so we can store more than one string in it.
    c.add("DBFILE", "valid1");
    c.add("DBFILE", "invalid_data"); // We'll parse this as double => fail

    // 2) Prepare an array of doubles, expecting to parse "valid1"/"invalid_data" as numeric
    //    The first parse might succeed if "valid1" is numeric. If not, it should fail.
    //    Let's ensure "valid1" is also non-numeric, so it fails on the first element.
    //    So we guarantee an exception in the for-loop with "std::istringstream iss(v);" 
    std::vector<double> arr(2);

    // Expect "catch(...) { throw runtime_error(...) }" in Config::get(...)
    REQUIRE_THROWS_WITH(
        c.get("DBFILE", arr),
        Catch::Matchers::Contains("Config: Failed getting reading/writing to an array DBFILE")
    );
}

TEST_CASE("Config deserialize() - partial/truncated buffer [line 287]", "[config][coverage]") {
    Config c1;
    // Add something to c1 so serialization won't be empty
    c1.add("DBFILE", "file1");
    c1.add("DBFILE", "file2");

    // Serialize
    std::vector<char> buf = c1.serialize();

    // Truncate the buffer in the middle => partial data
    // "pos < buffer.size()" => We'll forcibly break that loop by giving incomplete data
  buf.resize(4); // tiny, ensures incomplete read

    Config c2;
    // Should parse until it runs out of bytes, possibly ignoring partial data
    // Usually it won't throw in your code because there's no explicit check for insufficient data,
    // but let's see if it hits line 287 in coverage.
    // If you want to force an exception on truncated data, you'd do so in your code. Currently, it won't throw.
    REQUIRE_NOTHROW(c2.deserialize(buf));

    // We do not expect c2 to match c1, because the buffer was truncated
    REQUIRE_FALSE(c2 == c1);
}

TEST_CASE("Config postprocess() no RCUTMAX if all cutoffs are zero [line 312]", "[config][coverage]") {
    // postprocess() sets RCUTMAX only if there's a non-zero c2b/c3b/cmb
    // We'll ensure all are zero => no RCUTMAX in the map
    Config c;
    // Remove any defaults that might set INIT2B or RCUT2B
    c.remove("INIT2B");
    c.remove("INIT3B");
    c.remove("INITMB");
    c.remove("RCUT2B");
    c.remove("RCUT3B");
    c.remove("RCUTMB");

    // Without those keys set or "true", the cutoffs remain zeros in postprocess
    c.postprocess();

    // Now confirm that RCUTMAX was NOT set
    REQUIRE_FALSE(c.exist("RCUTMAX"));

    // Then let's set some partial keys but keep them effectively zero
    c.add("INIT2B", false);  // key exist, but false => won't read RCUT2B
    c.add("INIT3B", false);
    c.add("INITMB", false);

    c.postprocess();
    REQUIRE_FALSE(c.exist("RCUTMAX")); // still no max
}
TEST_CASE("Config read() with empty lines and comments") {
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
    Config c;
  c.remove("INIT2B");
    REQUIRE_NOTHROW(c.add(filename));  
    // We expect "INIT2B true" was read
    REQUIRE(c.get<bool>("INIT2B") == true);

    // Cleanup
    std::remove(filename);
}

TEST_CASE("Config postprocess() partial usage") {
    // This scenario sets INIT2B and RCUT2B but no values => see if the code can handle empty RCUT2B
    Config c;
    c.remove("INIT2B");
    c.remove("RCUT2B");
    
    // Add minimal lines that can pass check_for_training but are incomplete
    // e.g. DBFILE is mandatory for check_for_training
    c.add("DBFILE", "some_file");
    c.add("INIT2B", true); 
    // but we do NOT add "RCUT2B" => so c2b remains 0
    
    // This triggers postprocess: c2b=0 => no RCUT2BMAX. 
    // We'll verify coverage for the if-check that sets "RCUT2BMAX" only if c2b != 0.
    c.postprocess();
    REQUIRE_FALSE(c.exist("RCUT2BMAX"));
    REQUIRE_FALSE(c.exist("RCUTMAX"));

    // This won't pass check_for_training fully if we haven't set RCTYPE2B, TYPE2B, etc., 
    // but we won't call check_for_training. The main thing is to ensure partial postprocess coverage.
}

TEST_CASE("Config deserialize() unusual buffer") {
    // We create a config, add some data, serialize it,
    // then tamper or create an unusual buffer to see if parse continues gracefully.

    Config c1;
    c1.add("DBFILE","file1");
    // c1.add("INIT2B", true);

    std::vector<char> buf = c1.serialize();
    // Now let's cut the buffer in half to cause partial data
    buf.resize(buf.size() / 2);

    // Make a new config, call deserialize
    Config c2;
    REQUIRE_NOTHROW(c2.deserialize(buf));
    // We won't necessarily expect c2 to match c1, but it's a coverage scenario for the loop in deserialize
}

TEST_CASE("Config set_defaults() after clear()") {
    // We'll ensure the lines in set_defaults() that skip or continue are used.
    // 1) clear everything
    Config c;
    c.clear();

    // 2) call set_defaults() manually => it tries to fill from tdc
    REQUIRE_NOTHROW(c.set_defaults());

    // Typically, keys like "INIT2B","INIT3B" might reappear with default= "false"
    // We'll just check one
    REQUIRE(c.exist("INIT2B")); 
}
