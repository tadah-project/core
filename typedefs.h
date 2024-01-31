#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <vector>
#include "coretypes.h"
#include "../LIBS/Eigen/Core"

//typedef Vec3d vec3d;
//typedef Eigen::Ref<const Eigen::VectorXd> rvec;
typedef Eigen::VectorXd vec;
typedef Eigen::MatrixXd mat;

typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> phi_type;
typedef Eigen::Matrix<double,Eigen::Dynamic,1> t_type;
typedef std::vector<double> v_type;

typedef Eigen::Matrix3d stress_type;
//typedef Vec3d force_type;



/** Atomic Energy Descriptor (AED) type
 *
 * Dimensions:
 * [descriptor size]
 *
 * dim+1 when bias is used
 *
 */

/** Force Descriptor (FD) type
 *
 * Dimensions:
 * [descriptor size, 3]
 *
 */
//typedef Eigen::Matrix<double, Eigen::Dynamic, 3> fd_type;

/** Stress Descriptor (SD) type
 *
 * Dimensions:
 * [descriptor size,6]
 *
 */
typedef Eigen::Matrix<double, Eigen::Dynamic, 6> sd_type;   // sigma_ij order: 11,12,13,22,23,33

/** Density array to be used by many-body descriptors... TODO
 *
 * Dimensions:
 * [atom i][n]
 *
 * where n is specific to given descriptor
 *
 */
//typedef Eigen::Ref<Eigen::VectorXd> rho_type&;
//typedef Eigen::MatrixXd rhos_type;

#endif
