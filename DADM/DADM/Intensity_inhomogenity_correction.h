#pragma once
#include "MRI_Module.h"
class Intensity_inhomogenity_correction:
	public MRI_Module<float***>
{
public:
	Intensity_inhomogenity_correction();
	~Intensity_inhomogenity_correction();
};

