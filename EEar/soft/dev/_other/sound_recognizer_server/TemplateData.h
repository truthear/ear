#pragma once
#include <vector>
#include <math.h>

#define SV_FEAT_MFCC_1 0   
#define SV_SIZE_WINDOW 512    // размер 1 отрезка сигнала
#define SV_SIZE_VECTOR 22     // размер вектора признаков 

#define M_PI 3.1415926535897932384626433832795

using namespace std;

struct FeatureVector
{
	float V[SV_SIZE_VECTOR];//вектор
};

class TemplateData
{
private:
	void MFCC1(float *fft_re,float *fft_im,int N,float *C1, int NumBand);
	void fft(float *fftBuffer, long fftFrameSize, long sign);
	void transformFFTReIm(float *dSignal,float *fft_re,float *fft_im, int fftFrameSize);
	void createVectors(unsigned char * str,int iSize,int feat_type = SV_FEAT_MFCC_1);
	float fround(float x);
public:
	vector<FeatureVector> m_vec;
	int getCount() { return m_vec.size(); }
	int create(unsigned char *buf, int iSize, int feat_type = SV_FEAT_MFCC_1);
	TemplateData& operator = (const TemplateData & other)
	{
		m_vec.clear();
		for (unsigned int i=0;i<other.m_vec.size();i++)
			m_vec.push_back(other.m_vec[i]);
		return *this;
	}
	TemplateData(void);
	~TemplateData(void);
};
