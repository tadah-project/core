#include "catch2/catch.hpp"
#include <tadah/core/core_types.h>
#include <tadah/core/maths.h>
#include <iomanip>

using namespace tadah::core;

TEST_CASE("aed_type Constructors") {
    SECTION("Default constructor") {
        aed_type vec;
        REQUIRE(vec.size() == 0);
        REQUIRE(vec.data() == nullptr);
    }

    SECTION("Constructor with size and stride") {
        size_t size = 5;
        size_t stride = 2;
        aed_type vec(size, stride);
        REQUIRE(vec.size() == size);
        REQUIRE(vec.data() != nullptr);
    }
}

TEST_CASE("aed_type Constructors with External Arrays") {
    SECTION("Non-allocating constructor with double*") {
        size_t size = 5;
        double data[] = {1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == Approx(data[i])); // CHANGED
        }
    }

    SECTION("Allocating constructor with const double*") {
        size_t size = 5;
        const double data[] = {1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == Approx(data[i])); // CHANGED
        }
    }
}

TEST_CASE("aed_type Constructors with 'new' Allocated Arrays") {
    SECTION("Non-allocating constructor with double*") {
        size_t size = 5;
        double* data = new double[size]{1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == Approx(data[i])); // CHANGED
        }

        delete[] data; // Clean up
    }

    SECTION("Allocating constructor with const double*") {
        size_t size = 5;
        const double* data = new double[size]{1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == Approx(data[i])); // CHANGED
        }

        delete[] data; // Clean up
    }
}

TEST_CASE("Testing copy constructors of aed_type") {
    REQUIRE_NOTHROW(aed_type());
    REQUIRE_NOTHROW(aed_type(5));
    REQUIRE_NOTHROW(aed_type(5,2));
    REQUIRE_NOTHROW(aed_type(6,2));

    // Create an initial object
    aed_type original(3);
    aed_type original2(3);
    original[0] = 0.100000000000001; 
    original[1] = 10; 
    original[2] = 0.0001;

    SECTION("Const copy constructor") {
        const aed_type const_original = original;
        aed_type const_copy = const_original;
        REQUIRE(const_copy != original2);
    }

    SECTION("Non-const copy constructor") {
        aed_type non_const_copy = original;
        REQUIRE(non_const_copy == original);  // uses operator==
        REQUIRE(non_const_copy != original2);
    }
}

TEST_CASE("aed_type Assignment Operator") {
    size_t size = 5;
    aed_type original(size);
    for (size_t i = 0; i < size; ++i) {
        original[i] = static_cast<double>(i);
    }

    aed_type copy;
    copy = original;
    REQUIRE(copy == original); // uses operator==
}

TEST_CASE("aed_type Element Access") {
    size_t size = 3;
    aed_type vec(size);
    vec[0] = 1.0;
    vec[1] = 2.0;
    vec[2] = 3.0;

    REQUIRE(vec[0] == Approx(1.0)); // CHANGED
    REQUIRE(vec[1] == Approx(2.0)); // CHANGED
    REQUIRE(vec[2] == Approx(3.0)); // CHANGED
}

TEST_CASE("aed_type Operations") {
    size_t size = 3;
    aed_type vec(size);
    vec[0] = 1.0;
    vec[1] = 2.0;
    vec[2] = 3.0;

    vec += 1.0;
    REQUIRE(vec[0] == Approx(2.0)); // CHANGED
    REQUIRE(vec[1] == Approx(3.0)); // CHANGED
    REQUIRE(vec[2] == Approx(4.0)); // CHANGED

    vec -= 1.0;
    REQUIRE(vec[0] == Approx(1.0)); // CHANGED
    REQUIRE(vec[1] == Approx(2.0)); // CHANGED
    REQUIRE(vec[2] == Approx(3.0)); // CHANGED

    vec *= 2.0;
    REQUIRE(vec[0] == Approx(2.0)); // CHANGED
    REQUIRE(vec[1] == Approx(4.0)); // CHANGED
    REQUIRE(vec[2] == Approx(6.0)); // CHANGED
}

TEST_CASE("aed_type Copy Method with Range") {
    size_t size = 5;
    aed_type vec(size);
    for (size_t i = 0; i < size; ++i) {
        vec[i] = static_cast<double>(i + 1);
    }

    aed_type subVec = vec.copy(3, 1);
    REQUIRE(subVec.size() == 3);
    REQUIRE(subVec[0] == Approx(2.0)); // CHANGED
    REQUIRE(subVec[1] == Approx(3.0)); // CHANGED
    REQUIRE(subVec[2] == Approx(4.0)); // CHANGED
}

TEST_CASE("aed_type Random Initialization") {
    size_t size = 6;
    SECTION("Default bounds") {
        aed_type vec(size);
        vec.random();

        // Check values are within a specified range
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] >= -1e4);
            REQUIRE(vec[i] <= 1e4);
        }
    }

    SECTION("Custom bounds") {
        int seed = 501;
        double low = 1e-5;
        double high = 1e-4;
        aed_type vec2(size);
        vec2.random(seed, low, high);

        // Check values are within a specified range
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec2[i] >= low);
            REQUIRE(vec2[i] <= high);
        }
    }
}

TEST_CASE("Matrix_T basic functionality", "[matrix][new]") {
    SECTION("Default constructor") {
        Matrix_T<> mat;
        REQUIRE(mat.rows() == 0);
        REQUIRE(mat.cols() == 0);
        REQUIRE(mat.size() == 0);
    }

    SECTION("Constructor with rows and cols") {
        Matrix_T<> mat(3, 4);
        REQUIRE(mat.rows() == 3);
        REQUIRE(mat.cols() == 4);
        REQUIRE(mat.size() == 12); 
    }

    SECTION("Resize") {
        Matrix_T<> mat(2, 2);
        REQUIRE(mat.size() == 4);

        mat.resize(3, 5);
        REQUIRE(mat.rows() == 3);
        REQUIRE(mat.cols() == 5);
        REQUIRE(mat.size() == 15);
    }
}

TEST_CASE("Matrix_T element access and set_zero()", "[matrix][new]") {
    Matrix_T<> mat(2, 3);
    REQUIRE(mat.size() == 6);

    // Fill with some values
    for (size_t i = 0; i < mat.size(); ++i) {
        mat[i] = static_cast<double>(i + 1);
    }
    // Check them
    for (size_t i = 0; i < mat.size(); ++i) {
        REQUIRE(mat[i] == Approx(i + 1.0));
    }

    SECTION("Row-col operator()") {
        REQUIRE(mat(0, 0) == Approx(1.0));
        REQUIRE(mat(0, 1) == Approx(3.0)); // row-major or col-major? (We see col-major in code)
        // Actually, based on the code: m_[i + rows_*j] => col-major
        // So (0,1) => index = 0 + 2*1 = 2 => mat[2] => 3.0
        REQUIRE(mat(0, 1) == Approx(3.0));
        REQUIRE(mat(1, 2) == Approx(6.0));
    }

    SECTION("set_zero()") {
        mat.set_zero();
        for (size_t i = 0; i < mat.size(); ++i) {
            REQUIRE(mat[i] == Approx(0.0));
        }
    }
}

TEST_CASE("Matrix_T swap_columns()", "[matrix][new]") {
    Matrix_T<> mat(3, 3);
    // Populate matrix in col-major order:
    // col 0: 1,2,3
    // col 1: 4,5,6
    // col 2: 7,8,9
    // So underlying container: [1,2,3, 4,5,6, 7,8,9]
    for (size_t c = 0; c < 3; ++c) {
        for (size_t r = 0; r < 3; ++r) {
            mat(r, c) = 1.0 + r + c * 3;
        }
    }
    // Check initial order
    REQUIRE(mat(0, 0) == Approx(1.0));
    REQUIRE(mat(1, 0) == Approx(2.0));
    REQUIRE(mat(2, 0) == Approx(3.0));
    REQUIRE(mat(0, 2) == Approx(7.0));
    REQUIRE(mat(1, 2) == Approx(8.0));
    REQUIRE(mat(2, 2) == Approx(9.0));

    SECTION("Swapping col 0 and col 2") {
        mat.swap_columns(0, 2);
        // Now col 0 <-> col 2
        REQUIRE(mat(0, 0) == Approx(7.0));
        REQUIRE(mat(1, 0) == Approx(8.0));
        REQUIRE(mat(2, 0) == Approx(9.0));
        REQUIRE(mat(0, 2) == Approx(1.0));
        REQUIRE(mat(1, 2) == Approx(2.0));
        REQUIRE(mat(2, 2) == Approx(3.0));
    }
}

TEST_CASE("Matrix_T operator* (matrix-vector)", "[matrix][new]") {
    // 3x2 matrix times aed_type(2) => aed_type(3)
    Matrix_T<> mat(3, 2);
    mat.set_zero();

    // Fill col-major:
    // col0 -> [1, 2, 3]
    // col1 -> [4, 5, 6]
    // => stored in mat.m_ as [1,2,3,4,5,6]
    for (size_t c = 0; c < 2; ++c) {
        for (size_t r = 0; r < 3; ++r) {
            mat(r, c) = (double)(r + 1 + c * 3);
        }
    }

    aed_type vec(2); // Just aed_type with size=2
    vec[0] = 10.0;
    vec[1] = 100.0;

    aed_type result = mat * vec;

    REQUIRE(result.size() == 3);
    // col0 contributes: [1*10, 2*10, 3*10] => [10, 20, 30]
    // col1 contributes: [4*100, 5*100, 6*100] => [400, 500, 600]
    // sum => [410, 520, 630]
    REQUIRE(result[0] == Approx(410.0));
    REQUIRE(result[1] == Approx(520.0));
    REQUIRE(result[2] == Approx(630.0));
}

TEST_CASE("Matrix_T row() and col()", "[matrix][new]") {
    Matrix_T<> mat(2, 3);
    // Fill col-major => col0-> [1,2], col1-> [3,4], col2-> [5,6]
    for (size_t c = 0; c < 3; ++c) {
        for (size_t r = 0; r < 2; ++r) {
            mat(r, c) = (double)(1 + r + c * 2);
        }
    }

    SECTION("row(r) read-only usage") {
        aed_type row0 = mat.row(0);
        REQUIRE(row0.size() == mat.cols());
        // row(0) => [mat(0,0), mat(0,1), mat(0,2)]
        // => [1, 3, 5]
        REQUIRE(row0[0] == Approx(1.0));
        REQUIRE(row0[1] == Approx(3.0));
        REQUIRE(row0[2] == Approx(5.0));
    }

    SECTION("col(c) read-only usage") {
        aed_type col1 = mat.col(1);
        REQUIRE(col1.size() == 2);
        // col(1) => [mat(0,1), mat(1,1)] => [3,4]
        REQUIRE(col1[0] == Approx(3.0));
        REQUIRE(col1[1] == Approx(4.0));
    }
}

TEST_CASE("Matrix_T transpose free function", "[matrix][new]") {
    // Original is 2 x 3
    Matrix_T<> mat(2,3);
    // col0-> [1,2], col1-> [3,4], col2-> [5,6]
    for (size_t c = 0; c < 3; ++c) {
        for (size_t r = 0; r < 2; ++r) {
            mat(r,c) = (double)(1 + r + c*2);
        }
    }
    // Transposed -> 3 x 2
    Matrix_T<> matT = transpose(mat);
    REQUIRE(matT.rows() == 3);
    REQUIRE(matT.cols() == 2);
    // Check a few values
    // mat(0,0) => 1 => should appear as matT(0,0)
    REQUIRE(matT(0,0) == Approx(mat(0,0)));
    // mat(1,2) => 6 => should appear as matT(2,1)
    REQUIRE(matT(2,1) == Approx(mat(1,2)));
}

TEST_CASE("Matrix3d_T basic usage", "[matrix3d][new]") {
    Matrix3d m;
    // By default, m.rows() = 3, m.cols() = 3
    REQUIRE(m.rows() == 3);
    REQUIRE(m.cols() == 3);
    REQUIRE(m.size() == 9);

    SECTION("Check norm() of zero-initialized matrix") {
        REQUIRE(m.norm() == Approx(0.0));
    }

    SECTION("Set and get elements") {
        // col-major => index = i + 3*j
        // Let's fill in [1..9]
        for (size_t j = 0; j < 3; ++j) {
            for (size_t i = 0; i < 3; ++i) {
                m(i, j) = (double)(1 + i + j * 3);
            }
        }
        REQUIRE(m(0,0) == Approx(1.0));
        REQUIRE(m(2,2) == Approx(9.0));
        REQUIRE(m.norm() == Approx(std::sqrt(1*1 + 2*2 + 3*3 + 4*4 + 5*5 + 6*6 + 7*7 + 8*8 + 9*9)));
    }
}

TEST_CASE("Matrix3d_T Identity()", "[matrix3d][new]") {
    auto I = Matrix3d::Identity();
    REQUIRE(I.rows() == 3);
    REQUIRE(I.cols() == 3);
    // Diagonal = 1, others = 0
    for (size_t j = 0; j < 3; ++j) {
        for (size_t i = 0; i < 3; ++i) {
            double expected = (i == j) ? 1.0 : 0.0;
            REQUIRE(I(i,j) == Approx(expected));
        }
    }
    REQUIRE(I.norm() == Approx(std::sqrt(3.0))); // 3 diagonal ones
}

TEST_CASE("Matrix3d_T transpose() method", "[matrix3d][new]") {
    Matrix3d m;
    // Fill it: col0-> [1,2,3], col1->[4,5,6], col2->[7,8,9]
    for (size_t j = 0; j < 3; ++j) {
        for (size_t i = 0; i < 3; ++i) {
            m(i, j) = (double)(1 + i + j * 3);
        }
    }
    Matrix3d mt = m.transpose();
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            REQUIRE(mt(i,j) == Approx(m(j,i)));
        }
    }
}

TEST_CASE("Matrix3d_T inverse()", "[matrix3d][new]") {
    SECTION("Inverse of Identity should be Identity") {
        auto I = Matrix3d::Identity();
        auto invI = I.inverse();
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                REQUIRE(I(i,j) == Approx(invI(i,j)));
            }
        }
    }

    SECTION("Inverse of a known matrix") {
        // Let A = [1 2 3
        //          0 1 4
        //          5 6 0]
        // We'll compute A^-1 manually or check numeric result
        Matrix3d A;
        A(0,0) = 1; A(1,0) = 0; A(2,0) = 5;
        A(0,1) = 2; A(1,1) = 1; A(2,1) = 6;
        A(0,2) = 3; A(1,2) = 4; A(2,2) = 0;
        Matrix3d invA = A.inverse();

        // We can check that invA * A = Identity, within a margin
        [[maybe_unused]] auto checkMat = invA * A.row(0); // but note the operator*(const Vec3d&)
        // or do matrix multiply using some known function. 
        // For a simpler approach, we'll do a quick identity check entry by entry:
        Matrix3d product;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                double sum = 0.0;
                for (size_t k = 0; k < 3; ++k) {
                    sum += invA(i,k) * A(k,j);
                }
                product(i,j) = sum;
            }
        }
        // product should be close to Identity
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                double expected = (i == j)? 1.0 : 0.0;
                REQUIRE(product(i,j) == Approx(expected).margin(1e-12));
            }
        }
    }

    SECTION("Singular matrix (zero determinant)") {
        Matrix3d S;
        // Make rows linearly dependent
        // col-major => row 0: [1,2,3], row1: [2,4,6], row2: [3,6,9]
        // But we fill in code as needed:
        S(0,0) = 1; S(1,0) = 2; S(2,0) = 3;
        S(0,1) = 2; S(1,1) = 4; S(2,1) = 6;
        S(0,2) = 3; S(1,2) = 6; S(2,2) = 9;

        // This matrix is singular -> determinant = 0
        // The code doesn't handle that explicitly (no throw).
        // We'll just verify that dividing by zero occurs => INF or NaN in many compilers.
        REQUIRE_NOTHROW(S.inverse());
        // The result may be Inf/NaN. We won't do a strict check, just acknowledge it:
        [[maybe_unused]] auto Sinv = S.inverse();
        SUCCEED("S.inverse() called on singular matrix. Typically produces Inf or NaN.");
    }
}

TEST_CASE("Matrix3d_T operator*(Vec3d)", "[matrix3d][new]") {
    Matrix3d M;
    // Fill col-major => col0-> [1,2,3], col1-> [4,5,6], col2-> [7,8,9]
    for (size_t j = 0; j < 3; ++j) {
        for (size_t i = 0; i < 3; ++i) {
            M(i, j) = (double)(1 + i + j*3);
        }
    }
    // Vec3d = (10, 100, 1000)
    Vec3d v(10, 100, 1000);

    Vec3d res = M * v;
    // By columns:
    // col0 => multiplies v.x => [1*10, 2*10, 3*10] => [10,20,30]
    // col1 => multiplies v.y => [4*100,5*100,6*100] => [400,500,600]
    // col2 => multiplies v.z => [7*1000,8*1000,9*1000] => [7000,8000,9000]
    // sum => [7410, 8520, 9630]
    REQUIRE(res[0] == Approx(7410.0));
    REQUIRE(res[1] == Approx(8520.0));
    REQUIRE(res[2] == Approx(9630.0));
}

TEST_CASE("Mat6R3C row()", "[mat6r3c][new]") {
    Mat6R3C mat;
    // Fill in row-major for clarity, but we access by (i,j):
    // (i, j) => i * 3 + j
    // we have i in [0..5], j in [0..2]
    double val = 1.0;
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            mat(i, j) = val++;
        }
    }
    // row(0) => (1,2,3)
    // row(5) => (16,17,18)
    SECTION("Top row") {
        Vec3d r0 = mat.row(0);
        REQUIRE(r0[0] == Approx(1.0));
        REQUIRE(r0[1] == Approx(2.0));
        REQUIRE(r0[2] == Approx(3.0));
    }
    SECTION("Bottom row") {
        Vec3d r5 = mat.row(5);
        REQUIRE(r5[0] == Approx(16.0));
        REQUIRE(r5[1] == Approx(17.0));
        REQUIRE(r5[2] == Approx(18.0));
    }
}
