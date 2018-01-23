#pragma once
#include "Diffusion_Module.h"
#include <Eigen\Eigenvalues>
#include <complex>
#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <iostream>
#include <fftw3.h>
#include "Globals.h"
#include <unsupported/Eigen/CXX11/Tensor>

//typedef std::vector<std::vector<std::vector<matrixxcd>>> data5draw;
//typedef std::vector<std::vector<matrixxcd>> data4draw;
typedef std::vector<std::vector<std::vector<Eigen::Matrix2d>>> TensorData;

class Diffusion_tensor_imaging:
	public Diffusion_Module
{
public:
	Diffusion_tensor_imaging(Data4D, double, MatrixXd);
	virtual void Start();
	~Diffusion_tensor_imaging();

private:
	TensorData EstimateTensor();
	double b_value;
	MatrixXd gradients;
	Data4D diffusionData4D;
	MatrixXd BMatrix();
	VectorXd EigenVector();
	Data4D FractionalAnisotropy();
	Data3D RelativeAnisotropy();
	Data3D MeanDiffusivity();
	Data3D VolumeRatio();
};

