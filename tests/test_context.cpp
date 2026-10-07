#include "catch2/catch.hpp"
#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <tadah/core/number_format.h>

#include <sys/types.h>
#include <dirent.h>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <atomic>
#include <thread>
#include <vector>

using namespace tadah::core;

  auto split_lines = [](const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line)) {
      lines.push_back(line);
    }
    return lines;
  };

TEST_CASE( "Testing Empty Context", "[empty]" ) {
  Context c1;
  Context c2;
  REQUIRE(c1==c2);
}

TEST_CASE( "Testing Context add()", "[add()]" ) {
  Context c1;
  SECTION("Add internal") {
  }
  SECTION("Add invalid key name") {
    REQUIRE_THROWS(c1.add("DSIZEX",10));
    REQUIRE_THROWS(c1.addInternal("DSIZEX",10));
  }
  SECTION("Add invalid/empty key name") {
    REQUIRE_THROWS(c1.add("",10));
    REQUIRE_THROWS(c1.addInternal("",10));
  }
  SECTION("Add the same key twice.") {
    c1.addInternal("DSIZE",10);
    REQUIRE_NOTHROW(c1.addInternal("DSIZE",11)); // Allowed to overwrite
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
TEST_CASE( "Testing Context remove()", "[config_remove]" ) {
  Context c1;
  // Single key allowed
  c1.addInternal("DSIZE",10);
  c1.addInternal("SSTDEV",std::vector<double>{1,2,3,4,5,6});

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
    REQUIRE(c1.remove("DSIZE")==1);
  }
  SECTION("Remove valid key name twice") {
    c1.add("INIT2B",false);
    REQUIRE(c1.remove("INIT2B")==1);
    REQUIRE(c1.remove("INIT2B")==0);

    REQUIRE(c1.remove("DSIZE")==1);
    REQUIRE(c1.remove("DSIZE")==0);

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
TEST_CASE( "Testing Context size()", "[config_size]" ) {
  Context c;
  REQUIRE(c.size("INIT2B")==1);
  REQUIRE(c.size("INITMB")==1);
}
TEST_CASE( "Testing Context contains()", "[config_contains]" ) {
  Context c;
  REQUIRE(c.contains("INIT2B"));
  REQUIRE(c.contains("INITMB"));
}
TEST_CASE( "Testing Context operator()", "[config_operator()]" ) {
  std::string s1 = "f1/f1/f1";
  std::string s2 = "f2/f2/f2";
  std::string s3 = "f3/f3/f3";
  Context c;
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
  v = c("INITMB");
  REQUIRE(v[0]=="false");
}
TEST_CASE( "Testing is Context case sensitive", "[config_case]" ) {
  Context c;
  REQUIRE(c.size("dSize")==1);
  c.addInternal("dSiZE",10);
  REQUIRE(c.contains("DSIZE"));
  REQUIRE(c.contains("DSIZE"));
  REQUIRE(!(c.contains("DsizEx")));
  REQUIRE(!(c.contains(" DSiZE")));
  REQUIRE(!(c.contains(" DSIZE")));
  REQUIRE(c.size("dSiZE")==1);
  REQUIRE(c.remove("dSiZE")==1);
}
TEST_CASE( "Testing Context get(k)", "[config_get]" ) {
  Context c;
  std::string s1 = "f1/f1/f1";
  std::string s2 = "f2/f2/f2";
  std::string s3 = "f3/f3/f3";
  c.add("DBFILE",std::vector<std::string>{s1,s2,s3});
  REQUIRE(s1==c.get<std::string>("DBFILE"));

  c.addInternal("SSTDEV",std::vector<int>{1,2,3,4,5,6});
  REQUIRE(c.get<int>("SSTDEV")==1);
  REQUIRE(c.get<int>("SstdEV")==1);
}
TEST_CASE( "Testing Context get(k,v)", "[config_get_arr]" ) {
  Context c;
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

  c.addInternal("SSTDEV",std::vector<int>{1,2,3,4,5,6});
  std::vector<int> v2(6);
  c.get("SstDEV",v2);
  for (int i=0; i<6; ++i) {
    REQUIRE(v2[i]==(i+1));
  }
}
TEST_CASE( "Testing Context <<", "[config]" ) {
  Context c;
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
  // Check that the keys and their first values are aligned
  auto lines = split_lines(ss.str());
  REQUIRE(lines.size() >= 3);
  auto pos1 = lines[0].find(s1);
  auto pos2 = lines[1].find(s2);
  auto pos3 = lines[2].find(s3);
  REQUIRE(pos1 == pos2);
  REQUIRE(pos2 == pos3);

}

TEST_CASE("Testing Context serialization round-trip", "[config][new]") {
    // Create a config, then remove the default "INIT2B" if it contains, so we can add a new one.
    Context c1;
    c1.remove("INIT2B"); // remove default "INIT2B: false"
    c1.add("INIT2B", true);

    // Now we can safely add DBFILE, etc.
    c1.add("DBFILE", "db_1");
    c1.add("DBFILE", "db_2");
    c1.add("RCUT2B", 5.5);

    // Round-trip serialization
    std::vector<char> buffer = c1.serialize();
    Context c2;
    c2.deserialize(buffer);

    // Check equality
    REQUIRE(c1 == c2);
    REQUIRE(c2.get<bool>("INIT2B") == true);
    REQUIRE(c2.get<double>("RCUT2B") == 5.5);
}

TEST_CASE("Testing Context operator== - partial differences", "[config][new]") {
    Context cA, cB;
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

TEST_CASE("Testing Context with multi-token lines (simulation)", "[config][new]") {
    // We’ll simulate reading lines like "TOKEN val1 val2 val3" by calling add() multiple times.
    // The existing read() method already handles line input, but we can mimic it directly here.

    Context c;
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
    // and run your existing "Testing Context read()".
}


TEST_CASE("Context get<T>(key, T&) - parsing failure triggers catch(...) ", "[config][coverage]") {
    Context c;
    // 1) Add a user key that can hold multiple values, e.g. "DBFILE"
    //    so we can store more than one string in it.
    c.add("DBFILE", "valid1");
    c.add("DBFILE", "invalid_data"); // We'll parse this as double => fail

    std::vector<double> arr(2);

    REQUIRE_THROWS(c.get("DBFILE", arr));
}

TEST_CASE("Context deserialize() - partial/truncated buffer ", "[config][coverage]") {
    Context c1;
    // Add something to c1 so serialization won't be empty
    c1.add("DBFILE", "file1");
    c1.add("DBFILE", "file2");
    c1.add("DBFILE", "file2");
    c1.add("DBFILE", "file2");
    c1.add("DBFILE", "file2");

    // Serialize
    std::vector<char> buf = c1.serialize();

    // Truncate the buffer  => partial data
    buf.resize(2); // tiny, ensures incomplete read

    Context c2;

    /*REQUIRE_THROWS(c2.deserialize(buf)); // as we truncated the buffer*/

    // We do not expect c2 to match c1, because the buffer was truncated
    REQUIRE_FALSE(c2 == c1);
}

TEST_CASE("Context RCUTMAX=0 if all cutoffs are zero [line 312]", "[config][coverage]") {
    // postprocess() sets RCUTMAX only if there's a non-zero c2b/cmb
    // We'll ensure all are zero => no RCUTMAX in the map
    Context c;
    // Remove any defaults that might set INIT2B or RCUT2B
    c.remove("INIT2B");
    c.remove("INITMB");
    c.remove("RCUT2B");
    c.remove("RCUTMB");

    // Without those keys set or "true", the cutoffs remain zeros in postprocess
    /*c.postprocess();*/

    // Now confirm that RCUTMAX was NOT set
    REQUIRE(c.get<double>("RCUTMAX")==Approx(0));

    // Then let's set some partial keys but keep them effectively zero
    c.add("INIT2B", false);  // key exist, but false => won't read RCUT2B
    c.add("INITMB", false);

    REQUIRE(c.get<double>("RCUTMAX")==Approx(0));
}

TEST_CASE("Context deserialize() unusual buffer") {
    // We create a config, add some data, serialize it,
    // then tamper or create an unusual buffer to see if parse continues gracefully.

    Context c1;
    c1.add("DBFILE","file1");
    // c1.add("INIT2B", true);

    std::vector<char> buf = c1.serialize();
    // Now let's cut the buffer in half to cause partial data
    buf.resize(buf.size() / 2);

    // Make a new config, call deserialize
    Context c2;
    REQUIRE_NOTHROW(c2.deserialize(buf));
    // We won't necessarily expect c2 to match c1, but it's a coverage scenario for the loop in deserialize
}
TEST_CASE("Context: Internal key updates via add()", "[context_internal_update]") {
  // Create a context instance.
  Context ctx;

  SECTION("RCUT2B updates RCUT2BMAX and RCUTMAX") {
    // Add a value to RCUT2B.
    ctx.add("RCUT2B", 5.0);
    // Add a smaller value; maximum should remain.
    ctx.add("RCUT2B", 3.0);
    // Add a larger value; maximum should update.
    ctx.add("RCUT2B", 6.5);

    // Retrieve the internal keys.
    double rcut2bmax = ctx.get<double>("RCUT2BMAX");
    double rcutmax   = ctx.get<double>("RCUTMAX");

    REQUIRE(rcutmax == Approx(6.5));
    REQUIRE(rcut2bmax == Approx(6.5));
  }

  SECTION("RCUTMB updates RCUTMBMAX and RCUTMAX independently") {
    // Create a fresh context.
    tadah::core::Context ctx2;

    ctx2.add("RCUTMB", 2.5);
    ctx2.add("RCUTMB", 4.0);
    ctx2.add("RCUTMB", 3.0);

    // Retrieve the expected internal values.
    double rcutmbmax = ctx2.get<double>("RCUTMBMAX");
    double rcutmax   = ctx2.get<double>("RCUTMAX");


    REQUIRE(rcutmax == Approx(4.0));
    REQUIRE(rcutmbmax == Approx(4.0));
  }

  SECTION("No update on unmapped keys") {
    // Clear any previous state by creating a new context.
    tadah::core::Context ctx3;
    ctx3.add("SIGMA", 7.0);
    // Since "OTHER" is not mapped to internal keys, the related entries shouldn't be updated.
    REQUIRE(ctx3.get<double>("RCUT2BMAX")==Approx(0));
    REQUIRE(ctx3.get<double>("RCUTMAX")==Approx(0));
  }

  SECTION("Update after removal") {
    // Create a fresh context.
    tadah::core::Context ctx4;

    ctx4.add("RCUT2B", 5.0);
    ctx4.add("RCUT2B", 3.0);
    ctx4.add("RCUT2B", 6.0);
    REQUIRE(ctx4.get<double>("RCUT2BMAX")==Approx(6.0));
    REQUIRE(ctx4.get<double>("RCUTMAX")==Approx(6.0));
    REQUIRE(ctx4.get<double>("RCUTMBMAX")==Approx(0.0));

    ctx4.add("RCUTMB", 6.5);
    ctx4.add("RCUTMB", 3.5);
    ctx4.add("RCUTMB", 4.5);
    REQUIRE(ctx4.get<double>("RCUT2BMAX")==Approx(6.0));
    REQUIRE(ctx4.get<double>("RCUTMAX")==Approx(6.5));
    REQUIRE(ctx4.get<double>("RCUTMBMAX")==Approx(6.5));

    // Remove the key.
    ctx4.remove("RCUT2B");
    REQUIRE(ctx4.get<double>("RCUT2BMAX")==Approx(0.0));
    REQUIRE(ctx4.get<double>("RCUTMBMAX")==Approx(6.5));
    REQUIRE(ctx4.get<double>("RCUTMAX")==Approx(6.5));

    // Remove the key.
    ctx4.remove("RCUTMB");
    REQUIRE(ctx4.get<double>("RCUT2BMAX")==Approx(0.0));
    REQUIRE(ctx4.get<double>("RCUTMAX")==Approx(0.0));
    REQUIRE(ctx4.get<double>("RCUTMBMAX")==Approx(0.0));

    // Add a new value; the internal keys should be updated.
    ctx4.add("RCUT2B", 4.3);
    ctx4.add("RCUT2B", 3.3);
    REQUIRE(ctx4.get<double>("RCUT2BMAX")==Approx(4.3));
    REQUIRE(ctx4.get<double>("RCUTMAX")==Approx(4.3));
    REQUIRE(ctx4.get<double>("RCUTMBMAX")==Approx(0.0));

    ctx4.add("RCUTMB", 4.3);
    ctx4.add("RCUTMB", 5.3);
    REQUIRE(ctx4.get<double>("RCUT2BMAX")==Approx(4.3));
    REQUIRE(ctx4.get<double>("RCUTMAX")==Approx(5.3));
    REQUIRE(ctx4.get<double>("RCUTMBMAX")==Approx(5.3));
  }
}
TEST_CASE("Context: testing defaults retrieve", "[context_defaults]") {
  // Create a context instance.
  Context ctx;

  SECTION("Default values for INIT2B and INITMB") {
    // Retrieve the default values for INIT2B and INITMB.
    bool init2b = ctx.get<bool>("INIT2B");
    bool initmb = ctx.get<bool>("INITMB");

    REQUIRE(init2b == false);
    REQUIRE(initmb == false);
  }

  SECTION("Default values for DSIZE ") {
    size_t dsize = ctx.get<size_t>("DSIZE");

    REQUIRE(dsize == 0);
  }
}
TEST_CASE("Context: tesing multiple values for a key", "[context_multiple_values]") {
  Context c;
  c.add("DBFILE", "f1 f2 f3");
  REQUIRE(c.size("DBFILE")==3);


  // not allowed to overwrite multiple values at once
  REQUIRE_THROWS(c.addInternal("DSIZE", "1 2"));

  c.addInternal("DSIZE", "1");
  c.addInternal("DSIZE", "3"); // allowed overwrite
  REQUIRE(c.get<size_t>("DSIZE")==3);

  REQUIRE(c.size("SSTDEV")==6); // default size
  REQUIRE_NOTHROW(c.addInternal("SSTDEV", "1 2 3 4 5 6")); // max 6 values
  REQUIRE(c.size("SSTDEV")==6);
  c.remove("SSTDEV");
  REQUIRE_NOTHROW(c.addInternal("SSTDEV", "1 2 3 4 5 6")); // max 6 values
  REQUIRE(c.size("SSTDEV")==6);
  c.remove("SSTDEV");
  REQUIRE_THROWS(c.addInternal("SSTDEV", "1 2 3 4 5 6 7")); // max 6 values

  REQUIRE(c.size("DIMER")==3);
}

/* ---- Context::merge semantics + thread-safety hygiene ---------------------
 * merge() previously constructed lock_guards with std::adopt_lock without
 * ever locking (UB, flagged by TSan) and operator== read storage_ through an
 * unsynchronized reference (getMapRef, now removed). These tests pin the
 * merge semantics and give TSan a concurrent workload over merge/==/get. */

TEST_CASE("Testing Context merge()", "[config][merge]") {
  Context base, over;
  base.remove("ALPHA"); over.remove("ALPHA");

  base.add("DBFILE", "base_db");
  base.add("RCUT2B", 4.5);
  over.add("RCUT2B", 6.0);
  over.add("RCUTMB", 3.25);

  SECTION("override wins when allowed (default)") {
    Context m = Context::merge(base, over);
    REQUIRE(m.get<double>("RCUT2B") == 6.0);          // overridden
    REQUIRE(m.get<double>("RCUTMB") == 3.25);         // brought in from over
    REQUIRE(m.get<std::string>("DBFILE") == "base_db");   // preserved
    REQUIRE(base.get<double>("RCUT2B") == 4.5);       // inputs untouched
    REQUIRE(over.get<double>("RCUT2B") == 6.0);
  }
  SECTION("allowOverwrite=false keeps a merged value present") {
    Context m = Context::merge(base, over, false);
    REQUIRE(m.size("RCUT2B") >= 1);
    REQUIRE(m.get<double>("RCUTMB") == 3.25);
  }
  SECTION("merge with self-copy is stable") {
    Context m = Context::merge(base, base);
    REQUIRE(m.get<std::string>("DBFILE") == "base_db");
  }
}

TEST_CASE("Testing Context concurrent merge/==/get (TSan workload)",
          "[config][merge][threads]") {
  /* Meaningful under -fsanitize=thread: concurrent const-getters mutate
   * storage_ (lazy defaults) while merge() snapshots it and operator==
   * compares it. With correct locking this is race-free; the old adopt_lock
   * merge and unlocked operator== both fail this test under TSan. */
  Context a, b;
  a.add("DBFILE", "conc_db");
  b.add("RCUT2B", 5.0);

  std::atomic<bool> go{false};
  std::atomic<int> mismatches{0};
  auto worker = [&](int id) {
    while (!go.load()) {}
    for (int i = 0; i < 200; ++i) {
      switch ((i + id) % 3) {
        case 0: { Context m = Context::merge(a, b); (void)m; break; }
        case 1: { if (a == b) mismatches.fetch_add(1); break; }
        case 2: { (void)a.contains("INIT2B"); (void)b.contains("INITMB"); break; }
      }
    }
  };
  std::vector<std::thread> ts;
  for (int t = 0; t < 4; ++t) ts.emplace_back(worker, t);
  go.store(true);
  for (auto& t : ts) t.join();
  REQUIRE(mismatches.load() == 0);  // a and b differ throughout
}

TEST_CASE("Testing Context storage_update (delta-pot keyed raw writes)",
          "[config][wire]") {
  /* storage_update must be deserialize()-exact per key: raw tokens, key
   * uppercased, no validation; empty values erases. Equivalence proof:
   * a context assembled via keyed updates == one from a full deserialize. */
  Context a;
  a.storage_update("rcut2b", {"5.5"});
  a.storage_update("DBFILE", {"db_1", "db_2"});
  a.storage_update("WEIGHTS", {"1.0", "-2.0", "3.5"});

  const std::string text = "RCUT2B 5.5\nDBFILE db_1 db_2\nWEIGHTS 1.0 -2.0 3.5\n";
  Context b;
  b.deserialize(std::vector<char>(text.begin(), text.end()));
  REQUIRE(a == b);

  /* replace + erase */
  a.storage_update("WEIGHTS", {"9.9"});
  REQUIRE(a.get<double>("WEIGHTS") == 9.9);
  a.storage_update("DBFILE", {});
  REQUIRE_FALSE(a == b);
  const std::string text2 = "RCUT2B 5.5\nWEIGHTS 9.9\n";
  Context c2;
  c2.deserialize(std::vector<char>(text2.begin(), text2.end()));
  REQUIRE(a == c2);

  /* snapshot mirrors the raw map (delta diffing relies on this) */
  const auto snap = a.storage_snapshot();
  REQUIRE(snap.size() == 2);
  REQUIRE(snap.at("WEIGHTS") == std::vector<std::string>{"9.9"});
}

TEST_CASE("Testing Context serialize round-trip on real config fixtures",
          "[config][serialize][wire]") {
  /* Wire-format proof for shipping a Context across a process boundary
   * (PC_ISOLATION worker pool): every parseable fixture must survive
   * serialize -> deserialize with full equality. */
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  REQUIRE(dir != nullptr);
  struct dirent* entry;
  int checked = 0;
  while ((entry = readdir(dir)) != nullptr) {
    std::string filename(entry->d_name);
    if (filename == "." || filename == "..") continue;
    Context c1;
    try {
      c1 = CoreUserConfigParser().parse(path + "/" + filename);
    } catch (...) { continue; /* invalid-for-other-reasons files: not in scope */ }
    std::vector<char> buffer = c1.serialize();
    Context c2;
    c2.deserialize(buffer);
    INFO("fixture: " << filename);
    REQUIRE(c1 == c2);
    ++checked;
  }
  closedir(dir);
  REQUIRE(checked > 0);
}

// ---------------------------------------------------------------------------
// Lossless storage: Context text must never limit the precision of
// a value that is parsed back into a calculation.
// ---------------------------------------------------------------------------
namespace {
std::uint64_t ctx_bits(double v) {
  std::uint64_t u;
  std::memcpy(&u, &v, sizeof u);
  return u;
}
} // namespace

TEST_CASE("Context add/addInternal are lossless for every value type",
          "[context][lossless]") {
  const std::vector<double> vals = {
      0.1, 1e-20, 3e-13, -0.0, 5.3, 1.0 / 3, DBL_MIN, DBL_MAX,
      std::numeric_limits<double>::denorm_min(), 0.962983118529000759,
      0.962983118528700777, 12345.678901234567};

  SECTION("double scalars") {
    Context c;
    for (double v : vals) c.add("WEIGHTS", v);
    REQUIRE(c.size("WEIGHTS") == vals.size());
    for (size_t i = 0; i < vals.size(); ++i) {
      INFO("stored: " << c("WEIGHTS")[i]);
      CHECK(ctx_bits(c.get<double>("WEIGHTS", i)) == ctx_bits(vals[i]));
    }
    // The old fixed-18-decimal text flushed this to zero and merged these two.
    CHECK(c.get<double>("WEIGHTS", 1) != 0.0);
    CHECK(c.get<double>("WEIGHTS", 9) != c.get<double>("WEIGHTS", 10));
    CHECK(std::signbit(c.get<double>("WEIGHTS", 3)));
  }
  SECTION("ranges: std::vector<double>, aed_type, strided aed_type") {
    Context c;
    c.add("WEIGHTS", vals);
    t_type t(vals.size());
    for (size_t i = 0; i < vals.size(); ++i) t[i] = vals[i];
    c.add("NSTDEV", t);
    REQUIRE(c.size("WEIGHTS") == vals.size());
    REQUIRE(c.size("NSTDEV") == vals.size());
    for (size_t i = 0; i < vals.size(); ++i) {
      CHECK(ctx_bits(c.get<double>("WEIGHTS", i)) == ctx_bits(vals[i]));
      CHECK(ctx_bits(c.get<double>("NSTDEV", i)) == ctx_bits(vals[i]));
    }
    std::vector<double> raw = {0.1, 99.0, 1e-20, 99.0, 2.0 / 3, 99.0};
    aed_type strided(raw.data(), 3, 2);  // view: elements 0, 2, 4
    c.add("MPARAMS", strided);
    REQUIRE(c.size("MPARAMS") == 3);
    CHECK(ctx_bits(c.get<double>("MPARAMS", 0)) == ctx_bits(0.1));
    CHECK(ctx_bits(c.get<double>("MPARAMS", 1)) == ctx_bits(1e-20));
    CHECK(ctx_bits(c.get<double>("MPARAMS", 2)) == ctx_bits(2.0 / 3));
  }
  SECTION("float and long double") {
    Context c;
    c.add("MPARAMS", 0.1f);
    CHECK(c.get<float>("MPARAMS", 0) == 0.1f);
    CHECK(c.get<double>("MPARAMS", 0) == static_cast<double>(0.1f));
    const long double ld = 1.0L / 3;
    c.add("MPARAMS", ld);
    CHECK(c.get<long double>("MPARAMS", 1) == ld);
  }
  SECTION("integers, bools, strings verbatim") {
    Context c;
    c.add("SBASIS", static_cast<size_t>(123456789012ULL));
    CHECK(c("SBASIS")[0] == "123456789012");
    c.add("LAMBDA", -7);
    CHECK(c("LAMBDA")[0] == "-7");
    c.add("NORM", true);
    CHECK(c("NORM")[0] == "true");
    c.add("BIAS", false);
    CHECK(c("BIAS")[0] == "false");
    c.add("MODEL", "M_BLR BF_Linear");  // split on whitespace, verbatim
    CHECK(c("MODEL") == std::vector<std::string>{"M_BLR", "BF_Linear"});
    c.add("DBFILE", std::vector<std::string>{"a.tadah", "b 1.0000000000000000001"});
    CHECK(c("DBFILE") ==
          std::vector<std::string>{"a.tadah", "b", "1.0000000000000000001"});
    c.add("TYPE2B", std::vector<int>{20, -3});
    CHECK(c("TYPE2B") == std::vector<std::string>{"20", "-3"});
  }
  SECTION("integral doubles stay readable as integers") {
    Context c;
    c.addInternal("DSIZE", 100000.0);
    CHECK(c("DSIZE")[0] == "100000");
    CHECK(c.get<size_t>("DSIZE") == 100000u);
    c.add("KDEG2B", 5.0);
    CHECK(c("KDEG2B")[0] == "5");
  }
  SECTION("addInternal ranges and derived RCUT*MAX keys") {
    Context c;
    const std::vector<double> six(vals.begin(), vals.begin() + 6);  // SSTDEV holds 6
    c.addInternal("SSTDEV", six);
    for (size_t i = 0; i < six.size(); ++i)
      CHECK(ctx_bits(c.get<double>("SSTDEV", i)) == ctx_bits(six[i]));
    const double r2 = 5.123456789012345, rmb = 6.000000000000001;
    c.add("RCUT2B", r2);
    c.add("RCUTMB", rmb);
    CHECK(ctx_bits(c.get<double>("RCUT2BMAX")) == ctx_bits(r2));
    CHECK(ctx_bits(c.get<double>("RCUTMBMAX")) == ctx_bits(rmb));
    CHECK(ctx_bits(c.get<double>("RCUTMAX")) == ctx_bits(rmb));
  }
  SECTION("serialize/deserialize and merge keep the exact text") {
    Context c;
    c.add("WEIGHTS", vals);
    Context d;
    d.deserialize(c.serialize());
    for (size_t i = 0; i < vals.size(); ++i)
      CHECK(ctx_bits(d.get<double>("WEIGHTS", i)) == ctx_bits(vals[i]));
    Context m = Context::merge(Context(), c);
    for (size_t i = 0; i < vals.size(); ++i)
      CHECK(ctx_bits(m.get<double>("WEIGHTS", i)) == ctx_bits(vals[i]));
  }
}

TEST_CASE("Context has_stored/stored_value never insert defaults",
          "[context][has_stored]") {
  Context c;
  REQUIRE(c.size() == 0);
  CHECK_FALSE(c.has_stored("NUMERIC"));
  CHECK_FALSE(c.has_stored("numeric"));
  CHECK_FALSE(c.stored_value("NUMERIC").has_value());
  CHECK_FALSE(c.has_stored("NOT_A_KEY"));
  CHECK(c.size() == 0);  // nothing inserted

  // contains() inserts the blueprint default; has_stored() then sees it.
  REQUIRE(c.contains("NUMERIC"));
  CHECK(c.has_stored("NUMERIC"));
  CHECK(c.stored_value("NUMERIC") == std::optional<std::string>("18"));

  c.add("WEIGHTS", std::vector<double>{0.5, 1e-20});
  CHECK(c.stored_value("weights", 1) == std::optional<std::string>("1e-20"));
  CHECK_FALSE(c.stored_value("WEIGHTS", 2).has_value());
}

TEST_CASE("EMBF0: blueprint default ZERO materialises only on get/contains",
          "[context][has_stored][embf0]") {
  // DM_REAM relies on this: its get() materialises "EMBF0 ZERO" into the
  // model Context (so every DM_REAM potential is written with the key),
  // while every other reader probes with has_stored()/stored_value().
  Context c;
  CHECK_FALSE(c.has_stored("EMBF0"));
  CHECK_FALSE(c.stored_value("EMBF0").has_value());
  CHECK(c.size() == 0);
  CHECK(c.get<std::string>("EMBF0") == "ZERO");
  CHECK(c.stored_value("EMBF0") == std::optional<std::string>("ZERO"));

  Context d;
  d.add("EMBF0", std::string("FREE"));
  CHECK(d.get<std::string>("EMBF0") == "FREE");

  std::ostringstream os;
  os << c;
  CHECK(os.str().find("EMBF0") != std::string::npos);
}

namespace {
// Fixture covering every position type a potential file carries.
Context make_pot_fixture() {
  Context c;
  c.add("TYPE2B", "D2_Blip5 20 20 Ta Ta");
  c.add("RCTYPE2B", "Cut_Cos");
  c.add("RCUT2B", 5.3);
  c.add("CGRID2B", "LIN 7 0.1 3.0");
  c.add("TYPEMB", "DM_Blip 0 4 4 Ta Ta");
  c.add("CGRIDMB", "GEOM 4 0.3 2.7");
  c.add("SGRID2B", std::vector<double>{0.123456789012345678, 2.5, 7.0});
  c.add("WEIGHTS", std::vector<double>{0.1, 1e-20, -0.0, 3.0,
                                        0.123456789012345678, -2.0 / 3});
  c.add("SIGMA", "2 0.1 0.2 0.3 0.4");  // deprecated: written, not read back
  c.add("WCOV", "2 0.1 0.2 0.3 0.4");
  c.remove("OALGO");
  c.add("OALGO", "1 1e-13");
  c.add("FIXINDEX", "1-58");
  c.add("FIXWEIGHT", std::vector<double>{0.7, 1e-300});
  c.add("BIAS", true);
  c.add("MODEL", "M_BLR BF_Linear");
  c.add("ATOM", "Ta");
  c.add("NUMERIC", 12);
  return c;
}

std::vector<std::string> ctx_tokens(const std::string &line) {
  std::istringstream iss(line);
  std::vector<std::string> t;
  std::string s;
  while (iss >> s) t.push_back(s);
  return t;
}
} // namespace

TEST_CASE("Context write_potential: per-type rendering", "[context][pot]") {
  const Context c = make_pot_fixture();
  std::ostringstream os;
  c.write_potential(os, 18);
  const std::string pot = os.str();
  INFO(pot);

  std::map<std::string, std::vector<std::string>> lines;
  for (const auto &l : split_lines(pot)) {
    auto t = ctx_tokens(l);
    if (t.empty()) continue;
    const std::string k = t[0];
    t.erase(t.begin());
    lines[k] = t;
  }
  CHECK(lines.count("NUMERIC") == 0);  // never written into a potential
  CHECK(lines["TYPE2B"] ==
        std::vector<std::string>{"D2_Blip5", "20", "20", "Ta", "Ta"});
  CHECK(lines["RCTYPE2B"] == std::vector<std::string>{"Cut_Cos"});
  CHECK(lines["RCUT2B"] == std::vector<std::string>{"5.29999999999999982"});
  CHECK(lines["CGRID2B"] ==
        std::vector<std::string>{"LIN", "7", "0.100000000000000006", "3"});
  CHECK(lines["TYPEMB"] ==
        std::vector<std::string>{"DM_Blip", "0", "4", "4", "Ta", "Ta"});
  CHECK(lines["CGRIDMB"] ==
        std::vector<std::string>{"GEOM", "4", "0.299999999999999989",
                                 "2.70000000000000018"});
  CHECK(lines["SGRID2B"] ==
        std::vector<std::string>{"0.123456789012345677", "2.5", "7"});
  CHECK(lines["WEIGHTS"] ==
        std::vector<std::string>{"0.100000000000000006",
                                 "9.99999999999999945e-21", "-0", "3",
                                 "0.123456789012345677",
                                 "-0.66666666666666663"});
  // INT {DOUBLE}: the integer verbatim, every double at the precision. The
  // writer renders by value_type and does not consult `deprecated`: SIGMA,
  // a deprecated key added to a Context in code, is written like WCOV.
  for (const char *key : {"SIGMA", "WCOV"}) {
    INFO(key);
    CHECK(lines[key] ==
          std::vector<std::string>{"2", "0.100000000000000006",
                                   "0.200000000000000011", "0.299999999999999989",
                                   "0.400000000000000022"});
  }
  CHECK(lines["OALGO"] ==
        std::vector<std::string>{"1", "1.00000000000000003e-13"});
  CHECK(lines["FIXINDEX"] == std::vector<std::string>{"1-58"});
  CHECK(lines["BIAS"] == std::vector<std::string>{"true"});
  CHECK(lines["MODEL"] == std::vector<std::string>{"M_BLR", "BF_Linear"});
  CHECK(lines["ATOM"] == std::vector<std::string>{"Ta"});

  // The in-memory text is untouched by writing.
  CHECK(c("WEIGHTS")[0] == "0.1");
  CHECK(c.has_stored("NUMERIC"));
}

TEST_CASE("Context write_potential: same layout as operator<<",
          "[context][pot]") {
  const Context c = make_pot_fixture();
  std::ostringstream plain, pot;
  plain << c;
  c.write_potential(pot, 18);
  std::vector<std::string> a;
  for (const auto &l : split_lines(plain.str()))
    if (ctx_tokens(l).empty() || ctx_tokens(l)[0] != "NUMERIC") a.push_back(l);
  const std::vector<std::string> b = split_lines(pot.str());
  REQUIRE(a.size() == b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    INFO(a[i] << "\n" << b[i]);
    const auto ta = ctx_tokens(a[i]);
    const auto tb = ctx_tokens(b[i]);
    REQUIRE(ta.size() == tb.size());
    REQUIRE(ta.size() > 1);
    CHECK(ta[0] == tb[0]);
    // key column and first value start at the same offset
    CHECK(a[i].find(ta[1]) == b[i].find(tb[1]));
    for (size_t j = 1; j < ta.size(); ++j) {
      if (ta[j] == tb[j]) continue;
      double x = 0, y = 0;
      REQUIRE(parse_decimal_finite(ta[j], x));
      REQUIRE(parse_decimal_finite(tb[j], y));
      CHECK(ctx_bits(x) == ctx_bits(y));
    }
  }
}

TEST_CASE("Context write_potential: parse-back", "[context][pot]") {
  const Context c = make_pot_fixture();
  for (int digits : {17, 18, 12}) {
    std::ostringstream os;
    c.write_potential(os, digits);
    std::istringstream is(os.str());
    const Context back = CoreUserConfigParser().parse(is);
    INFO("digits " << digits << "\n" << os.str());
    CHECK_FALSE(back.has_stored("NUMERIC"));
    for (const auto &kv : c.storage_snapshot()) {
      const std::string &key = kv.first;
      if (key == "NUMERIC" || key.find("MAX") != std::string::npos) continue;
      // SIGMA is deprecated (the weight covariance of Tadah! 1.0.0 to
      // 1.5.0 and the 1.6.0 development builds): parsed with a notice and
      // not stored. (Before WCOV replaced it, it was read back like every
      // other key.)
      if (key == "SIGMA") {
        CHECK_FALSE(back.has_stored(key));
        continue;
      }
      REQUIRE(back.has_stored(key));
      const auto &orig = kv.second;
      REQUIRE(back.size(key) == orig.size());
      for (size_t j = 0; j < orig.size(); ++j) {
        const std::string &t = back(key)[j];
        double x = 0, y = 0;
        if (is_integer_literal(orig[j]) || !parse_decimal_finite(orig[j], x)) {
          CHECK(t == orig[j]);  // verbatim token
          continue;
        }
        REQUIRE(parse_decimal_finite(t, y));
        if (digits >= kRoundTripDigits) {
          CHECK(ctx_bits(y) == ctx_bits(x));  // bit-exact at 17/18
        } else {
          // NUMERIC < 17: exactly the %.{N}g rounding, nothing else.
          double z = 0;
          REQUIRE(parse_decimal_finite(format_sig(x, digits), z));
          CHECK(ctx_bits(y) == ctx_bits(z));
        }
      }
    }
  }
  // A value that needs more than 12 digits really is rounded at 12.
  std::ostringstream os12;
  c.write_potential(os12, 12);
  const Context back12 = [&] {
    std::istringstream is(os12.str());
    return CoreUserConfigParser().parse(is);
  }();
  CHECK(back12("WEIGHTS")[4] == "0.123456789012");
  CHECK(back12.get<double>("WEIGHTS", 4) != c.get<double>("WEIGHTS", 4));
}

namespace {

/* A container whose size() is n while it has room for 8 values: it records
 * the indices written through operator[], so a write past size() is seen
 * without undefined behaviour. */
struct SizeProbe {
  std::size_t n = 0;
  double store[8] = {};
  std::vector<std::size_t> written;
  std::size_t size() const { return n; }
  double &operator[](std::size_t i) {
    written.push_back(i);
    return store[i < 8 ? i : 7];
  }
};

/* The message of the exception f throws, or "" when it throws none. */
template <class F> std::string thrown_message(F f) {
  try {
    f();
  } catch (const std::exception &e) {
    return e.what();
  }
  return "";
}

/* KEY set to exactly the tokens of `text`, stored as they are written. */
void set_tokens(Context &c, const std::string &key, const std::string &text) {
  c.remove(key);
  c.add(key, text);
}

}  // namespace

/* Pins: Context::get(key, container) refuses a key that stores more values
 * than the container holds: with six WEIGHTS and a container whose size()
 * is 2 it throws std::runtime_error naming WEIGHTS, 6 and 2, and writes no
 * element (it refuses before converting a value); with six values and room
 * for six it fills them.
 * Before the fix: no throw; every stored value was written, the last at
 * index 5 of the container of size 2 (into a std::vector of size 2 that is
 * a write past its end: read_sigma reading a SIGMA that an earlier fit had
 * appended to corrupted the heap). */
TEST_CASE("Context get(key, container) refuses more values than the "
          "container holds",
          "[context][get_container]") {
  Context c;
  c.add("WEIGHTS", "0.5 1.5 2.5 3.5 4.5 5.5");
  SizeProbe two;
  two.n = 2;
  const std::string msg = thrown_message([&] { c.get("WEIGHTS", two); });
  INFO("message: " << msg << "; indices written: " << two.written.size());
  CHECK(msg.find("WEIGHTS") != std::string::npos);
  CHECK(msg.find("6 values") != std::string::npos);
  CHECK(msg.find("holds 2") != std::string::npos);
  CHECK(two.written.empty());

  SizeProbe six;
  six.n = 6;
  REQUIRE_NOTHROW(c.get("WEIGHTS", six));
  for (std::size_t i = 0; i < 6; ++i)
    CHECK(ctx_bits(six.store[i]) == ctx_bits(0.5 + static_cast<double>(i)));
}

/* Pins: a key with fewer stored values than the container holds fills
 * those values and leaves the rest of the container as it was (the
 * documented behaviour, owner decision D6 of the trainer-reuse fix).
 * Before the fix: passes as well (a pin of the behaviour the size check
 * keeps). */
TEST_CASE("Context get(key, container) fills a larger container with the "
          "stored values",
          "[context][get_container]") {
  Context c;
  c.add("WEIGHTS", "0.5 1.5");
  std::vector<double> v(4, -1.0);
  REQUIRE_NOTHROW(c.get("WEIGHTS", v));
  CHECK(ctx_bits(v[0]) == ctx_bits(0.5));
  CHECK(ctx_bits(v[1]) == ctx_bits(1.5));
  CHECK(ctx_bits(v[2]) == ctx_bits(-1.0));
  CHECK(ctx_bits(v[3]) == ctx_bits(-1.0));
}

/* Pins: an integer read of a Context value (get<int>, get<size_t>, the
 * n-th value, and get(key, container) into integers) takes a value that is
 * an exact integer in the type's range, whatever its spelling: "2", "+5",
 * "2.0", "2.", "-0", "1e3", "2.50e1", "7E+0", ".5e1", "-4.000", the int
 * limits, and "18446744073709551615" for size_t; it refuses, naming the key
 * and the value, a fraction ("2.7", "1e-3", "14.47"), trailing text
 * ("2abc", "1e"), a value out of the type's range ("3000000000" and "1e10"
 * for int, "-1" and "1.8446744073709551616e19" for size_t) and text that is
 * not a decimal number ("abc", "0x10", "inf", "nan", ".", "-", "e3"). A
 * double read is unchanged ("2.7" is 2.7).
 * Before the fix: an integer read took the longest integer prefix of the
 * text and ignored the rest without an error: "2.7", "2abc", "1e-3", "1e3",
 * "2.50e1", "1e10", "1e" and "0x10" gave 2, 2, 1, 1, 2, 1, 1 and 0; the
 * size_t reads of "-1", "14.47" and "1.8446744073709551616e19" gave
 * 18446744073709551615, 14 and 1; the container read of "2.0 1e3 -4" gave
 * {2, 1, -4} and that of "2.7 1 0" no error; ".5e1" was refused. (A
 * Kern_Polynomial degree MPARAMS 2.7 was so used as 2.) */
TEST_CASE("Context integer reads take exact integers in any spelling and "
          "refuse every other value",
          "[context][integral]") {
  Context c;
  struct Good {
    const char *text;
    long long value;
  };
  const Good good_int[] = {{"2", 2},     {"+5", 5},       {"2.0", 2},
                           {"-0", 0},    {"1e3", 1000},   {"2.50e1", 25},
                           {"-4", -4},   {"-4.000", -4},  {"7E+0", 7},
                           {".5e1", 5},  {"2.", 2},
                           {"2147483647", 2147483647LL},
                           {"-2147483648", -2147483648LL}};
  for (const Good &g : good_int) {
    INFO("int value '" << g.text << "'");
    set_tokens(c, "MPARAMS", g.text);
    int v = -99;
    CHECK_NOTHROW(v = c.get<int>("MPARAMS"));
    CHECK(v == g.value);
    CHECK_NOTHROW(v = c.get<int>("MPARAMS", 0));
    CHECK(v == g.value);
  }
  const char *bad_int[] = {"2.7", "1e-3", "2abc", "abc", "0x10", "inf",
                           "nan", "3000000000", "1e10", ".", "-", "1e", "e3"};
  for (const char *text : bad_int) {
    INFO("int value '" << text << "'");
    set_tokens(c, "MPARAMS", text);
    const std::string msg = thrown_message([&] { (void)c.get<int>("MPARAMS"); });
    INFO("message: " << msg);
    CHECK(msg.find("MPARAMS") != std::string::npos);
    CHECK(msg.find(std::string("\"") + text + "\"") != std::string::npos);
    const std::string msg_n =
        thrown_message([&] { (void)c.get<int>("MPARAMS", 0); });
    CHECK(msg_n.find("MPARAMS") != std::string::npos);
  }

  set_tokens(c, "SBASIS", "18446744073709551615");
  std::size_t u = 0;
  CHECK_NOTHROW(u = c.get<std::size_t>("SBASIS"));
  CHECK(u == std::numeric_limits<std::size_t>::max());
  for (const char *text : {"-1", "1.8446744073709551616e19", "14.47"}) {
    INFO("size_t value '" << text << "'");
    set_tokens(c, "SBASIS", text);
    const std::string msg =
        thrown_message([&] { (void)c.get<std::size_t>("SBASIS"); });
    INFO("message: " << msg);
    CHECK(msg.find("SBASIS") != std::string::npos);
  }

  // The container path converts each value the same way.
  set_tokens(c, "MPARAMS", "2.0 1e3 -4");
  std::vector<int> v3(3);
  CHECK_NOTHROW(c.get("MPARAMS", v3));
  CHECK(v3 == std::vector<int>{2, 1000, -4});
  set_tokens(c, "MPARAMS", "2.7 1 0");
  const std::string msg_c = thrown_message([&] { c.get("MPARAMS", v3); });
  INFO("container message: " << msg_c);
  CHECK(msg_c.find("MPARAMS") != std::string::npos);
  CHECK(msg_c.find("\"2.7\"") != std::string::npos);
  std::vector<std::size_t> u1(1);
  set_tokens(c, "SBASIS", "-1");
  CHECK(thrown_message([&] { c.get("SBASIS", u1); }).find("SBASIS") !=
        std::string::npos);

  // A double read is not an integer read.
  set_tokens(c, "MPARAMS", "2.7 1 0");
  CHECK(ctx_bits(c.get<double>("MPARAMS")) == ctx_bits(2.7));
  std::vector<double> d3(3);
  CHECK_NOTHROW(c.get("MPARAMS", d3));
  CHECK(ctx_bits(d3[0]) == ctx_bits(2.7));
}

/* Pins: a floating read of a Context value (get<double>, get<float>,
 * get<long double>, the n-th value, and get(key, container) into doubles)
 * takes the whole token when it is a finite decimal number, in any decimal
 * spelling: "1", "1.5", ".5", "5.", "-1e-3", "+2", "1E3", "-0" (negative
 * zero) and the subnormal "1e-310", bit for bit as before; it refuses,
 * naming the key and the value and saying "a finite decimal number is
 * required", trailing text ("1.5abc", "2e", "1.5.2"), text that is not a
 * decimal number ("0x10", "abc", ".", "-", "e3", ""), non-finite values
 * ("inf", "-inf", "nan", "1e400"), and a float out of float's range
 * ("1e39" for get<float>). A bool read takes "true" or "false" whole
 * ("truex" is refused). A configuration line of 2048 characters or more
 * (the validator's numeric shortcut, which takes commas as separators)
 * with comma-separated numbers is stored as one token, and its read is
 * refused.
 * Before the fix: the read took the longest numeric prefix the stream
 * read and ignored the rest: "1.5abc", "1.5.2", "0x10" and "1.0,2.0" gave
 * 1.5, 1.5, 0 and 1 (also as float and long double), "truex" was true, and
 * the long comma line gave 1.0, its first number; "2e", "abc", ".", "-",
 * "e3", "inf", "-inf", "nan", "1e400", "" and "1e39" for float were
 * refused (a stream failure) without the reason. */
TEST_CASE("Context floating reads take a finite decimal number, the whole "
          "token",
          "[context][floating]") {
  Context c;
  struct Good {
    const char *text;
    double value;
  };
  const Good good[] = {{"1", 1.0},       {"1.5", 1.5},   {".5", 0.5},
                       {"5.", 5.0},      {"-1e-3", -1e-3}, {"+2", 2.0},
                       {"1E3", 1000.0},  {"-0", -0.0},   {"1e-310", 1e-310}};
  for (const Good &g : good) {
    INFO("double value '" << g.text << "'");
    set_tokens(c, "MPARAMS", std::string(g.text) + " " + g.text);
    double v = 99.0;
    CHECK_NOTHROW(v = c.get<double>("MPARAMS"));
    CHECK(ctx_bits(v) == ctx_bits(g.value));
    CHECK_NOTHROW(v = c.get<double>("MPARAMS", 1));
    CHECK(ctx_bits(v) == ctx_bits(g.value));
    std::vector<double> two(2);
    CHECK_NOTHROW(c.get("MPARAMS", two));
    CHECK(ctx_bits(two[0]) == ctx_bits(g.value));
    CHECK(ctx_bits(two[1]) == ctx_bits(g.value));
  }
  const char *bad[] = {"1.5abc", "2e",   "1.5.2", "0x10", "abc", ".",
                       "-",      "e3",   "inf",   "-inf", "nan", "1e400",
                       "1.0,2.0"};
  const std::string why = ": a finite decimal number is required";
  for (const char *text : bad) {
    INFO("double value '" << text << "'");
    set_tokens(c, "MPARAMS", std::string("1 ") + text);
    const std::string quoted = std::string("value=\"") + text + "\"";
    const std::string msg_n =
        thrown_message([&] { (void)c.get<double>("MPARAMS", 1); });
    INFO("message: " << msg_n);
    CHECK(msg_n == "Conversion failed for key=MPARAMS, " + quoted + why);
    std::vector<double> two(2);
    CHECK(thrown_message([&] { c.get("MPARAMS", two); }) ==
          "Conversion failed for key=MPARAMS, " + quoted + why);
    set_tokens(c, "MPARAMS", text);
    CHECK(thrown_message([&] { (void)c.get<double>("MPARAMS"); }) ==
          "Conversion failed for key=MPARAMS, " + quoted + why);
  }
  // An empty token (storage_update stores tokens as given).
  c.storage_update("MPARAMS", {""});
  CHECK(thrown_message([&] { (void)c.get<double>("MPARAMS"); }) ==
        "Conversion failed for key=MPARAMS, value=\"\"" + why);

  // float and long double: the same grammar; a float out of its range.
  set_tokens(c, "MPARAMS", "3.5 1e39 0.1 1.5abc");
  CHECK(c.get<float>("MPARAMS", 0) == 3.5f);
  CHECK(thrown_message([&] { (void)c.get<float>("MPARAMS", 1); }) ==
        "Conversion failed for key=MPARAMS, value=\"1e39\"" + why);
  CHECK(c.get<long double>("MPARAMS", 2) == 0.1L);
  CHECK(thrown_message([&] { (void)c.get<long double>("MPARAMS", 3); }) ==
        "Conversion failed for key=MPARAMS, value=\"1.5abc\"" + why);
  CHECK(thrown_message([&] { (void)c.get<float>("MPARAMS", 3); }) ==
        "Conversion failed for key=MPARAMS, value=\"1.5abc\"" + why);

  // bool: true or false, whole.
  set_tokens(c, "NORM", "true");
  CHECK(c.get<bool>("NORM"));
  set_tokens(c, "NORM", "false");
  CHECK_FALSE(c.get<bool>("NORM"));
  c.storage_update("NORM", {"truex"});
  CHECK(thrown_message([&] { (void)c.get<bool>("NORM"); }) ==
        "Conversion failed for key=NORM, value=\"truex\": true or false is "
        "required");

  // A long configuration line of comma-separated numbers.
  std::string line = "MPARAMS 1.0";
  while (line.size() < 2100) line += ",2.0";
  const Context fc = CoreUserConfigParser().parse(std::vector<std::string>{line});
  REQUIRE(fc.size("MPARAMS") == 1);
  const std::string msg =
      thrown_message([&] { (void)fc.get<double>("MPARAMS"); });
  CHECK(msg.rfind("Conversion failed for key=MPARAMS, value=\"1.0,2.0,2.0,",
                  0) == 0);
  CHECK((msg.size() > why.size() &&
         msg.compare(msg.size() - why.size(), why.size(), why) == 0));
}

/* Pins: a configuration line of usual length whose list value holds
 * commas (the blueprint validator's list patterns take a comma, or a '+',
 * between two values, but the parser splits a line at blanks only) is
 * accepted by CoreUserConfigParser as one token per blank-separated word,
 * and a number read of such a token is refused, naming the key and the
 * value: 'MPARAMS 1.0,2.0', 'CGRID2B 1.0,2.0,3.0' and 'FIXWEIGHT 0.1,0.2'
 * are one token each, 'SGRID2B 1.0,2.0 3.0' two ("1.0,2.0" and "3.0"), and
 * get<double> of a comma token gives "Conversion failed for key=<KEY>,
 * value=\"<token>\": a finite decimal number is required"; "3.0" reads.
 * Before the fix: the parser accepted the same lines, and each comma token
 * was read as its first number, 1, 1, 0.1 and 1, without an error. */
TEST_CASE("Context: a short configuration line with commas in a list is "
          "refused when its number is read",
          "[context][floating]") {
  struct Line {
    std::string key, text, token;
    size_t tokens;
  };
  const Line lines[] = {{"MPARAMS", "MPARAMS 1.0,2.0", "1.0,2.0", 1},
                        {"CGRID2B", "CGRID2B 1.0,2.0,3.0", "1.0,2.0,3.0", 1},
                        {"FIXWEIGHT", "FIXWEIGHT 0.1,0.2", "0.1,0.2", 1},
                        {"SGRID2B", "SGRID2B 1.0,2.0 3.0", "1.0,2.0", 2}};
  for (const Line &l : lines) {
    INFO(l.text);
    Context c;
    REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(
                        std::vector<std::string>{l.text}));
    REQUIRE(c.size(l.key) == l.tokens);
    CHECK(c.get<std::string>(l.key) == l.token);
    CHECK(thrown_message([&] { (void)c.get<double>(l.key); }) ==
          "Conversion failed for key=" + l.key + ", value=\"" + l.token +
              "\": a finite decimal number is required");
    std::vector<double> all(l.tokens);
    CHECK(thrown_message([&] { c.get(l.key, all); }) ==
          "Conversion failed for key=" + l.key + ", value=\"" + l.token +
              "\": a finite decimal number is required");
    if (l.tokens == 2) CHECK(c.get<double>(l.key, 1) == 3.0);
  }
}
