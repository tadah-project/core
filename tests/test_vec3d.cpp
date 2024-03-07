#include "catch2/catch.hpp"
#include "../core_types.h"
#include "../vec3d.h"
#include <iomanip>

TEST_CASE( "Testing Vec3d()", "[maths]" ) {
    REQUIRE_NOTHROW(Vec3d());
    v_type v = {1,2,3};
    REQUIRE_NOTHROW(v);
    REQUIRE_NOTHROW(Vec3d(1,2,3));
}
TEST_CASE( "Testing Vec3d op", "[maths]" ) {
    v_type v1_ = {1,2,3};
    v_type v2_ = {2,3,4};

    Vec3d v1(v1_);
    Vec3d v2(v2_);
    Vec3d v3 = v2-v1;
    REQUIRE((v3[0]==1 && v3[1]==1 && v3[2]==1));
    REQUIRE((v3[0]==v3(0) && v3[1]==v3(1) && v3[2]==v3(2)));

    const Vec3d v4 = v2-v1;
    REQUIRE((v4[0]==1 && v4[1]==1 && v4[2]==1));
    REQUIRE((v4[0]==v4(0) && v4[1]==v4(1) && v4[2]==v4(2)));

    Vec3d v5 = v2+v1;
    REQUIRE((v5[0]==3 && v5[1]==5 && v5[2]==7));

    v5+=v1;
    REQUIRE((v5[0]==4 && v5[1]==7 && v5[2]==10));

    v5-=v1;
    REQUIRE((v5[0]==3 && v5[1]==5 && v5[2]==7));

    v5*=2;
    REQUIRE((v5[0]==6 && v5[1]==10 && v5[2]==14));

    v5 = -v5;
    REQUIRE((v5[0]==-6 && v5[1]==-10 && v5[2]==-14));

    REQUIRE(v1*v1==14);

    Vec3d v6 = v1*2;
    Vec3d v7 = 2*v1;
    REQUIRE(v6==v7);

    v7.set_zero();
    REQUIRE((v7[0]==0 && v7[1]==0 && v7[2]==0));
    REQUIRE(v7!=v6);

    v7 = v6;
    REQUIRE(v6==v6);

    v6(1,2,3);
    REQUIRE(v1==v6);

    REQUIRE_THAT( v1.norm(), !Catch::Matchers::WithinAbs(3.74165738677, 1e-12) );

    v6(4,5,6);
    v7 = v1.cross(v6);
    REQUIRE((v7[0]==-3 && v7[1]==6 && v7[2]==-3));
}
    
