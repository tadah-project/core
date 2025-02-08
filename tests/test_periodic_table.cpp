#include "catch2/catch.hpp"
#include <tadah/core/periodic_table.h>

TEST_CASE("PeriodicTable Initialization", "[initialize]") {
    // Ensure that the periodic table is properly initialized
    REQUIRE(PeriodicTable::symbol.empty());
    REQUIRE(PeriodicTable::name.empty());
    REQUIRE(PeriodicTable::Z.empty());
    REQUIRE(PeriodicTable::masses.empty());

    PeriodicTable::initialize();

    REQUIRE_FALSE(PeriodicTable::symbol.empty());
    REQUIRE_FALSE(PeriodicTable::name.empty());
    REQUIRE_FALSE(PeriodicTable::Z.empty());
    REQUIRE_FALSE(PeriodicTable::masses.empty());
}

TEST_CASE("Find Element by Symbol", "[find_by_symbol]") {
    PeriodicTable::initialize();

    SECTION("Known symbol") {
        const Element& oxygen = PeriodicTable::find_by_symbol("O");
        REQUIRE(oxygen.symbol == "O");
        REQUIRE(oxygen.Z == 8);
    }

    SECTION("Unknown symbol") {
        REQUIRE_THROWS(
            PeriodicTable::find_by_symbol("Xx")
        );
    }
}

TEST_CASE("Find Element by Name", "[find_by_name]") {
    PeriodicTable::initialize();

    SECTION("Known name") {
        const Element& carbon = PeriodicTable::find_by_name("Carbon");
        REQUIRE(carbon.symbol == "C");
        REQUIRE(carbon.Z == 6);
    }

    SECTION("Unknown name") {
        REQUIRE_THROWS(
            PeriodicTable::find_by_name("Unobtainium")
        );
    }
}

TEST_CASE("Find Element by Atomic Number (Z)", "[find_by_Z]") {
    PeriodicTable::initialize();

    SECTION("Known atomic number") {
        const Element& iron = PeriodicTable::find_by_Z(26);
        REQUIRE(iron.symbol == "Fe");
        REQUIRE(iron.Z == 26);
    }

    SECTION("Unknown atomic number") {
        REQUIRE_THROWS(
            PeriodicTable::find_by_Z(999)
        );
    }
}

TEST_CASE("Get Atomic Mass", "[get_mass]") {
    PeriodicTable::initialize();

    SECTION("By atomic number") {
        double mass_O = PeriodicTable::get_mass(8);
        REQUIRE(mass_O == Approx(15.9990).epsilon(0.0001));
    }

    SECTION("By Element object") {
        const Element& gold = PeriodicTable::find_by_symbol("Au");
        double mass_Au = PeriodicTable::get_mass(gold);
        REQUIRE(mass_Au == Approx(196.967).epsilon(0.0001));
    }

    SECTION("Unknown atomic number") {
        REQUIRE_THROWS(
            PeriodicTable::get_mass(999)
        );
    }
}

