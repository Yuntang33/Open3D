// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2017 Jaesik Park <syncle@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include "Eigen.h"

#include <iostream> // just for debugging
#include <Eigen/Geometry>
#include <Core/Utility/Console.h>

namespace Eigen {

/// Extending Eigen namespace by adding frequently used matrix type
typedef Eigen::Matrix<double, 6, 6> Matrix6d;
typedef Eigen::Matrix<double, 6, 1> Vector6d;

}	// namespace Eigen

namespace three{

namespace {

/// Function to solve Ax=b
std::tuple<bool, Eigen::VectorXd> SolveLinearSystem(
		const Eigen::MatrixXd &A, const Eigen::VectorXd &b) 
{
	bool solution_exist = true;
	// note: computing determinant for large scale matrix would be bottleneck.
	double det = A.determinant();
	if (fabs(det) < 1e-6 || std::isnan(det) || std::isinf(det))
		solution_exist = false;

	if (solution_exist) {
		// Robust Cholesky decomposition of a matrix with pivoting.
		Eigen::MatrixXd x = -A.ldlt().solve(b);
		return std::make_tuple(solution_exist, std::move(x));
	} else {
		return std::make_tuple(false, std::move(Eigen::Vector6d::Zero()));
	}
}

}	// unnamed namespace

Eigen::Matrix4d TransformVector6dToMatrix4d(Eigen::Vector6d input)
{
	Eigen::Matrix4d output;
	output.setIdentity();
	output.block<3, 3>(0, 0) =
		(Eigen::AngleAxisd(input(2), Eigen::Vector3d::UnitZ()) *
			Eigen::AngleAxisd(input(1), Eigen::Vector3d::UnitY()) *
			Eigen::AngleAxisd(input(0), Eigen::Vector3d::UnitX())).matrix();
	output.block<3, 1>(0, 3) = input.block<3, 1>(3, 0);
	return output;
}

std::tuple<bool, std::vector<Eigen::Matrix4d>>
		SolveJacobianSystemAndObtainExtrinsicArray(
		const Eigen::Matrix6d &JTJ, const Eigen::Vector6d &JTr)
{
	std::vector<Eigen::Matrix4d> output_matrix_array;
	output_matrix_array.clear();
	if (JTJ.rows() != JTr.rows() || JTJ.cols() % 6 != 0) {
		PrintWarning("[SolveJacobianSystemAndObtainExtrinsicArray] Unsupported matrix format.\n");
		return std::make_tuple(false, std::move(output_matrix_array));
	}

	bool solution_exist;
	Eigen::VectorXd x;
	std::tie(solution_exist, x) = SolveLinearSystem(JTJ, JTr);

	if (solution_exist) {
		int nposes = (int)x.rows() / 6;
		for (int i = 0; i < nposes; i++) {
			Eigen::Matrix4d motion = TransformVector6dToMatrix4d(
				x.block<6, 1>(i * 6, 0));
			output_matrix_array.push_back(motion);
		}
		return std::make_tuple(solution_exist, std::move(output_matrix_array));
	}
	else {
		return std::make_tuple(false, std::move(output_matrix_array));
	}
}

}	// namespace three
