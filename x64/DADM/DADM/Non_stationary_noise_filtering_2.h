#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include "Diffusion_Structural_Module.h"
#include "Globals.h"

class Non_stationary_noise_filtering_2:
	public Diffusion_Structural_Module
{
public:
	Non_stationary_noise_filtering_2(Data3D, Data3D);
	Non_stationary_noise_filtering_2(Data4D, Data4D);
	~Non_stationary_noise_filtering_2() {};

	MatrixXd unlm(MatrixXd input, MatrixXd sigma);

private:
	virtual void StructuralDataAlgorithm();
	virtual void DiffusionDataAlgorithm();
	Data3D estimator3D;
	Data4D estimator4D;

	Data3D part_out;
};

