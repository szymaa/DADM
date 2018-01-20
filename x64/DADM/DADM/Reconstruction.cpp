#include "Reconstruction.h"
#include "qdebug.h"
#include <fftw3.h>
#include <Globals.h>
#include <iostream>
#include <fstream>
#include <string>




Reconstruction::Reconstruction(Data4DRaw raw_data, Data3DRaw sensitivity_maps, int L, int r)
{
	qDebug() << "Reconstruction constructor called";
	data4DRaw_input = raw_data;
	sensitivityMaps3D = sensitivity_maps;
	this->L = L;
	this->r = r;
	dtype = STRUCTURAL_DATA;
}

Reconstruction::Reconstruction(Data5DRaw data, Data3DRaw sensitivity_maps, int L, int r)
{
	qDebug() << "Reconstruction constructor called";
	data5DRaw_input = data;
	sensitivityMaps3D = sensitivity_maps;
	this->L = L;
	this->r = r;
	dtype = DIFFUSION_DATA;

}

void Reconstruction::StructuralDataAlgorithm() {

	int slices_no = data4DRaw_input.size();//liczba przekroj�w
	Data3DRaw data_raw;
	MatrixXd reconstructed_data;
	Data3D slices(slices_no);

	for (int i = 0; i < slices_no; i++)
	{
		//przeprowadzenie Odwrotnej Transformaty Fouriera
		data_raw = data4DRaw_input[i];
		data_raw = FourierTransform(data_raw);
		
		//Rekonstrukcja obrazu
		reconstructed_data = LSreconstruction(data_raw);
		qDebug() << "LS reconstruction done";

		reconstructed_data = TikhonovRegularization(data_raw, reconstructed_data);
		qDebug() << "Tikhonov regularization done";

		//------------------
		//normalizacja warto�ci
		double min = reconstructed_data.minCoeff();
		double max = reconstructed_data.maxCoeff();
		for (int z = 0; z < 256; z++)
		{
			for (int a = 0; a < 256; a++)
			{
				reconstructed_data(z,a)=((reconstructed_data(z, a)-min)*255)/(max-min);
			}
		}
		slices.at(i) = reconstructed_data;
		
	}
	//zwracana warto��
	//data3D_output = slices;
	//tymczasowo
	Global::structuralData = slices;
	
} 

void Reconstruction::DiffusionDataAlgorithm()
{
	int slices_no = data5DRaw_input.size();
	Data4D slices(slices_no);
	Data3DRaw data_raw;

	for (int i = 0; i < slices_no; i++)
	{
		Data3DRaw data(L);

		Data4DRaw data4d_raw = data5DRaw_input[i];
		int gradients_no = data4d_raw.size();
		
		Data3D reconstructed_data_3D(gradients_no);

		for (int j = 0; j < gradients_no; j++)
		{
			data_raw = data4d_raw.at(j);
			data = FourierTransform(data_raw);
			MatrixXd reconstructed_data(256,256);
			//Rekonstrukcja obrazu
			reconstructed_data = LSreconstruction(data);
			//normalizacja warto�ci
			
			double min = reconstructed_data.minCoeff();
			double max = reconstructed_data.maxCoeff();
			for (int z = 0; z < 256; z++)
				{
					for (int a = 0; a < 256; a++)
						{
							reconstructed_data(z, a) = ((reconstructed_data(z, a) - min) * 255) / (max - min);
						}
				}
			reconstructed_data_3D.at(j) = reconstructed_data;
							
			Global::structuralData = reconstructed_data_3D;
			}

		slices.at(i) = reconstructed_data_3D;

	}

	//zwracana warto��
	//data4D_output = slices;
}

Data3DRaw Reconstruction::FourierTransform(Data3DRaw raw_data)
{

	Data3DRaw data;
	data = ifft(raw_data); //wywo�anie funkcji do odwrotnej transformaty Fouriera
	return data;
}

Data3DRaw Reconstruction::ifft(Data3DRaw raw_data)
{
	int sizey = raw_data.at(0).rows(); // liczba wierszy
	int sizex = raw_data.at(0).cols();// liczba kolumn
	MatrixXcd afteridftmatrix(sizey, sizex);
	Data3DRaw afteridftvector(L);

	for (int k = 0; k < L; k++)
	{
		MatrixXcd log = raw_data.at(k);

		//-----------------------

		std::vector<std::complex<double>> a(sizex*sizey);//zdefiniowanie wektora wej�ciowego
		int count = 1;
		#pragma omp parallel for shared(log, a, count, sizey, sizex) private(i, j)
		for (int i = 0; i < sizey; i++)
		{
			for (int j = 0; j < sizex; j++)
			{
				a[i*sizex + j] = log(i, j);
				count++;
			}
		}

		std::vector<std::complex<double>> b(sizex*sizey);//zdefiniowanie wektora wyj�ciowego

		//zdefiniowanie IDFT plan
		fftw_plan plan = fftw_plan_dft_2d(sizey, sizex, reinterpret_cast<fftw_complex*>(&a[0]), reinterpret_cast<fftw_complex*>(&b[0]), FFTW_BACKWARD, FFTW_ESTIMATE);
		fftw_execute(plan);
		fftw_destroy_plan(plan);
		fftw_cleanup();
		std::complex<double> tmp;
		//powr�t do macierzy
		#pragma omp parallel for shared(b, tmp, afteridftmatrix, sizey, sizex) private(i, j)
		for (int i = 0; i < sizey; i++)
		{
			for (int j = 0; j < sizex; j++)
			{
				tmp = b.at(i*sizex + j);
				afteridftmatrix(i, j) = tmp;
			}
		}

		afteridftvector.at(k) = afteridftmatrix;

	}
	return afteridftvector;
}

MatrixXd Reconstruction::LSreconstruction(Data3DRaw data)
{
	//rekonstrukcja SENSE
	int rec_step = 256 / r;
	MatrixXcd Sx(L, r);
	VectorXcd Dr(r, 1);
	MatrixXd Image(256, 256);
	MatrixXcd Ds(L, 1);
	Data3DRaw I_raw = data;
	//std::vector<MatrixXcd> S = sensitivityMaps3D;
	//std::vector<MatrixXcd> Sd(L);
	MatrixXcd temp(256, 256);
	MatrixXcd log = I_raw.at(1);

	int sizex = log.rows(); // liczba wierszy
	int sizey = log.cols(); //liczba kolumn

	for (int y = 0; y <sizey; y++)
	{
		for (int x = 0; x<sizex; x++)
		{

			for (int j = 0; j<L; j++)
			{
				for (int p=0; p<r; p++)
				{ 
				Sx(j, p) = sensitivityMaps3D.at(j)(x + p*rec_step, y);
				}
			}

			for (int k = 0; k<L; k++)
			{
				Ds(k) = I_raw.at(k)(x, y);
			}
			//wyliczenie zgodnie ze wzorem
			MatrixXcd temp = (Sx.transpose()*Sx);
			MatrixXcd temp2 = temp.inverse()*Sx.transpose();
			Dr = temp2*Ds;

			for (int q = 0; q < r; q++)
			{
				Image(x+q*rec_step, y) = abs(Dr(q));
			}

		}

	}

	return Image; //zrekonstruowany obraz
}

MatrixXd Reconstruction::TikhonovRegularization(Data3DRaw data, MatrixXd image)
{
	//regularyzacja Tichonova
	int rec_step = 256 / r;
	double lambda = 0.02;
	MatrixXcd Sx(L, r);
	VectorXcd Dr(r, 1);
	MatrixXd Image(256, 256);
	MatrixXcd Ds(L, 1);
	Matrix2d A;
	A(0, 0) = 1;
	A(1, 0) = 0;
	A(0, 1) = 0;
	A(1, 1) = 1;
	Data3DRaw I_raw = data;
	MatrixXd LastImage = medianFilter(image, 3);
	MatrixXd ImagePoint(r, 1);

	int sizex = I_raw.at(1).rows(); // liczba wierszy
	int sizey = I_raw.at(1).cols(); //liczba kolumn


	for (int y = 0; y <sizey; y++)
	{
		for (int x = 0; x<sizex; x++)
		{

			for (int j = 0; j<L; j++)
			{
				for (int p = 0; p<r; p++)
				{
					Sx(j, p) = sensitivityMaps3D.at(j)(x + p*rec_step, y);
				}
			}

			for (int k = 0; k<L; k++)
			{
				Ds(k) = I_raw.at(k)(x, y);
			}
			//wyliczenie zgodnie ze wzorem
			for (int w = 0; w < r; w++) 
			{
				ImagePoint(w, 0) = LastImage(x + w*rec_step, y);
			}
			
			Dr = ImagePoint + ((Sx.transpose()*Sx + ((lambda*lambda)*A.transpose()*A)).inverse()*Sx.transpose())*(Ds - (Sx*ImagePoint));
			
			for (int q = 0; q < r; q++)
			{
				Image(x + q*rec_step, y) = abs(Dr(q));
			}

		}

	}
	return Image; //zrekonstruowany obraz
}

void Reconstruction::writeToCSVfile(std::string name, MatrixXd matrix)
{
	std::ofstream file(name.c_str());

	for (int i = 0; i < matrix.rows(); i++) {
		for (int j = 0; j < matrix.cols(); j++) {

			if (j + 1 == matrix.cols()) {
				file << (matrix(i, j));
			}
			else {
				file << (matrix(i, j)) << ',';
			}
		}
		file << '\n';
	}
	file.close();
}

MatrixXd Reconstruction::medianFilter(MatrixXd image, int windowSize)
{

	std::vector<double> pixels;
	double color;
	int a;
	int b;

	MatrixXd newimage(image.rows(), image.cols());
	//glowny loop dla siatki pixeli wyznaczajacy tzw pixel wyrozniony
	for (int x = 0; x < image.rows(); ++x)
	{
		for (int y = 0; y < image.cols(); ++y)
		{
			//loopy odpowiedzialne za operowanie na oknie filtrujacym
			// #pragma omp parallel for shared(b, a, x, y, color, pixels) private(i, j)
			for (int j = -(windowSize / 2); j < windowSize - 1; ++j)
			{
				for (int i = -(windowSize / 2); i < windowSize - 1; ++i)
				{
					a = x + i;
					b = y + j;
					if (a<0)
						a = 0;
					if (a > 255)
						a = 255;
					if (b<0)
						b = 0;
					if (b > 255)
						b = 255;
					color = image(a, b);
					pixels.push_back(color);
				}
			}

			sort(pixels.begin(), pixels.end());
			newimage(x, y) = pixels[pixels.size() / 2];
			pixels.clear();
		}
	}
	return newimage;
}

