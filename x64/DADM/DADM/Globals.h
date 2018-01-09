#pragma once
#include <Eigen/Dense>
//#include <unsupported\Eigen\CXX11\Tensor>
#include "MRI_Module.h"
#include "Segmentation.h"
#include "Reconstruction.h"
#include "dadm.h"

namespace Global {
	extern FilteringType ftype;
	extern DataType dtype;
	extern double b_value;
	extern MatrixXd gradients;
	extern Data3D temporaryData;
	extern Data3D structuralData;
	extern Data3DRaw structuralRawData;
	extern Data3DRaw structuralSensitivityMaps;
	extern double L;
	extern double r;
	extern Data4DRaw diffusionRawData;
	extern Data3DRaw diffusionSensitivityMaps;
	extern Data4D diffusionData4D;
	extern Data3D diffusionData3D;
	extern SegmentationData segmentationData;
	extern MatrixXd FA;
	extern MatrixXd MD;
	extern MatrixXd RA;
	extern MatrixXd VR;
}