#include "catch2/catch.hpp"
#include <tadah/core/normaliser_core.h>
#include <tadah/core/core_types.h>

using namespace tadah::core;

TEST_CASE( "Testing Normaliser_Core()", "[create]" ) {
    REQUIRE_NOTHROW(Normaliser_Core());
}
TEST_CASE( "Testing Normaliser_Core(Config&)", "[create]" ) {

    Config c;
    REQUIRE_NOTHROW(Normaliser_Core(c));

    c.remove("BIAS");
    REQUIRE_THROWS(Normaliser_Core(c));

    c.add("BIAS", true);
    REQUIRE_NOTHROW(Normaliser_Core(c));

    c.remove("VERBOSE");
    REQUIRE_THROWS(Normaliser_Core(c));
}
TEST_CASE( "Testing Normaliser_Core::normalise(aed)", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config5");
    Normaliser_Core norm(c);
    aed_type a(c.size("NMEAN"));    // zeros
    a.set_zero();

    // should fail too b/c we subtract mean
    REQUIRE_NOTHROW(norm.normalise(a));
    REQUIRE_THAT( a.mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );

    a.random();
    REQUIRE_THAT( a.mean(), !Catch::Matchers::WithinAbs(0, 1e-12));

    // should fail because a is random
    REQUIRE_NOTHROW(norm.normalise(a));
    REQUIRE_THAT( a.mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
}
TEST_CASE( "Testing Normaliser_Core::normalise(fd)", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config5");
    Normaliser_Core norm(c);
    fd_type f(c.size("NMEAN"));
    f.set_zero();
    REQUIRE_NOTHROW(norm.normalise(f));

    // should not fail as we calc 0/st_dev
    REQUIRE_THAT( f(0).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );

    f.random();
    REQUIRE_NOTHROW(norm.normalise(f));
    REQUIRE_THAT( f(0).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
}
TEST_CASE( "Testing Normaliser_Core::normalise(fd,k)", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config5");
    Normaliser_Core norm(c);
    fd_type f(c.size("NMEAN"));
    f.set_zero();
    REQUIRE_NOTHROW(norm.normalise(f,0));
    REQUIRE_NOTHROW(norm.normalise(f,1));
    REQUIRE_NOTHROW(norm.normalise(f,2));
    REQUIRE_THAT( f(0).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );

    f.random();
    REQUIRE_NOTHROW(norm.normalise(f,0));
    REQUIRE_NOTHROW(norm.normalise(f,1));
    REQUIRE_NOTHROW(norm.normalise(f,2));
    REQUIRE_THAT( f(0).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
}

TEST_CASE( "Testing Normaliser_Core::normalise(aed) nobias", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config6");
    Normaliser_Core norm(c);
    aed_type a(c.size("NMEAN"));    // zeros
    a.set_zero();

    REQUIRE_THAT( a.mean(), Catch::Matchers::WithinAbs(0, 1e-12) );

    REQUIRE_NOTHROW(norm.normalise(a));
    REQUIRE_THAT( a.mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );

    a.random();
    REQUIRE_THAT( a.mean(), !Catch::Matchers::WithinAbs(0, 1e-12));

    // should fail because a is random
    REQUIRE_NOTHROW(norm.normalise(a));
    REQUIRE_THAT( a.mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
}
TEST_CASE( "Testing Normaliser_Core::normalise(fd) nobias", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config6");
    Normaliser_Core norm(c);
    fd_type f(c.size("NMEAN"));
    f.set_zero();
    REQUIRE_NOTHROW(norm.normalise(f));
    REQUIRE_THAT( f(0).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );

    f.random();
    REQUIRE_NOTHROW(norm.normalise(f));
    REQUIRE_THAT( f(0).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
}
TEST_CASE( "Testing Normaliser_Core::normalise(fd,k) nobias", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config6");
    Normaliser_Core norm(c);
    fd_type f(c.size("NMEAN"));
    f.set_zero();
    REQUIRE_NOTHROW(norm.normalise(f,0));
    REQUIRE_NOTHROW(norm.normalise(f,1));
    REQUIRE_NOTHROW(norm.normalise(f,2));
    REQUIRE_THAT( f(0).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), Catch::Matchers::WithinAbs(0, 1e-12) );

    f.random();
    REQUIRE_NOTHROW(norm.normalise(f,0));
    REQUIRE_NOTHROW(norm.normalise(f,1));
    REQUIRE_NOTHROW(norm.normalise(f,2));
    REQUIRE_THAT( f(0).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(1).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
    REQUIRE_THAT( f(2).mean(), !Catch::Matchers::WithinAbs(0, 1e-12) );
}
TEST_CASE( "Testing Normaliser_Core normaliser value", "[method]" ) {
    Config c("tests_data/valid_configs/valid_config6");
    Normaliser_Core norm;
    norm.bias = false;
    size_t n = 2;
    norm.mean = {0.1, 0.2};
    norm.std_dev = {0.3, 0.4};
    aed_type a(n);
    a[0] = 1.0;
    a[1] = 2.0;

    REQUIRE_THAT( a.mean(), Catch::Matchers::WithinAbs(1.5, 1e-12) );
    norm.normalise(a);
    REQUIRE_THAT( a.mean(), Catch::Matchers::WithinAbs(3.75, 1e-12) );

    norm.bias = true;
    a[0] = 1.0;
    a[1] = 2.0;
    norm.normalise(a);
    REQUIRE_THAT( a.mean(), Catch::Matchers::WithinAbs(2.75, 1e-12) );
}
