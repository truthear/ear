#include "TemplateData.h"

TemplateData::TemplateData(void)
{
}

TemplateData::~TemplateData(void)
{
	m_vec.clear();
}

void TemplateData::MFCC1(float *fft_re,float *fft_im,
					  int N,// длина фрейма (по умолчанию 512)
					  float *C1, //массив полученных признаков, длиной кол-во полос*кол-во феймовNUM_SEGMENT 
					  int NumBand)//количество мел-полос (по умолчанию 22, не более 22)
{
	int NUM_SEGMENT=1;//кол-во фреймов, по которым считаются мел-коэф-ты
	int i,j,k;
	float E,sum;
	float *P;
	int begin[] = {1,4,7, 9,11,14,17,21,24,28,33,38,44,51,59,68,80, 94,113,136,164,199};//в отсчетах начала мел-полос
	int end[] = {3,6,8,10,13,16,20,23,27,32,37,43,50,58,67,79,93,112,135,163,198,255};//в отсчетах концы мел-полос
	float dPI = 3.14159265358979323846;

	P=new float[NumBand];
	for(k=0;k<NUM_SEGMENT;k++)
	{
		sum=0.0;
		for(j=0;j<N/2;j++)
			sum+=pow(fft_re[k*N+j],2)+pow(fft_im[k*N+j],2);//полная энергия на к-том фрейме
		for(i=0;i<NumBand;i++)
		{
			E=0.0;
			for(j=begin[i];j<=end[i];j++)
				E+=pow(fft_re[k*N+j],2)+pow(fft_im[k*N+j],2);//энергия для к-го фрейма на i-той полосе
			if(sum!=0.0 && E!=0.0)
				P[i]=log(E/sum);
			else
				P[i]=0.0;
		}
		for(i=1;i<NumBand;i++)
		{
			C1[k*NumBand+i]=0.0;
			for(j=0;j<NumBand;j++)
				C1[k*NumBand+i]+=P[j]*cos((float)i*dPI*(j-0.5)/NumBand);
		}
		C1[k*NumBand]=0.0;
		for(j=0;j<NumBand;j++)
			C1[k*NumBand]+=P[j];
		C1[k*NumBand]/=NumBand;
	}
	delete[] P;
}

void TemplateData::fft(float *fftBuffer, long fftFrameSize, long sign)
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2*fftFrameSize-2; i += 2) 
	{
		for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) 
		{
			if (i & bitm) j++;
			j <<= 1;
		}
		if (i < j) 
		{
			p1 = fftBuffer+i; p2 = fftBuffer+j;
			temp = *p1; *(p1++) = *p2;
			*(p2++) = temp; temp = *p1;
			*p1 = *p2; *p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(log((float)fftFrameSize)/log((float)2.)+.5); k++) 
	{
		le <<= 1;
		le2 = le>>1;
		ur = 1.0;
		ui = 0.0;
		arg = M_PI / (le2>>1);
		wr = cos(arg);
		wi = sign*sin(arg);
		for (j = 0; j < le2; j += 2) 
		{
			p1r = fftBuffer+j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (i = j; i < 2*fftFrameSize; i += le) 
			{
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr; *p2i = *p1i - ti;
				*p1r += tr; *p1i += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = ur*wr - ui*wi;
			ui = ur*wi + ui*wr;
			ur = tr;
		}
	}
}

void TemplateData::transformFFTReIm(float *dSignal,float *fft_re,float *fft_im, int fftFrameSize)
{
	float *dFFTworksp=new float[2*fftFrameSize];
	float window;
	//оконная функция Хемминга
	for (int k = 0; k < fftFrameSize;k++) 
	{
		window = 0.54 - 0.46*cos(2.*M_PI*(float)k/(float)(fftFrameSize - 1)); //Окно Хэмминга
		dFFTworksp[2*k] = dSignal[k] * window;
		dFFTworksp[2*k+1] = 0.;
	}

	fft(dFFTworksp,fftFrameSize,-1);

	for (int k=0;k<fftFrameSize;k++)
	{
		fft_re[k] = dFFTworksp[2*k];
		fft_im[k] = dFFTworksp[2*k+1];
	}
	delete [] dFFTworksp;
}

float TemplateData::fround(float x)
{
	return (float)floor(x+0.5);
}

//преобразование сигнала в последовательность фреймов
//без сглаживания
void TemplateData::createVectors(unsigned char * str,int iSize,int feat_type)
{
	int i,j;
	float *dSign = new float [SV_SIZE_WINDOW];
	FeatureVector Vec;
	float fft_re[SV_SIZE_WINDOW];
	float fft_im[SV_SIZE_WINDOW];
	int iNumFrames = (iSize/SV_SIZE_WINDOW)*2-1;
	m_vec.clear();
	if (feat_type == SV_FEAT_MFCC_1)
	{
		for( i=0;i < iNumFrames;i++)
		{
			for (j = 0;j<SV_SIZE_WINDOW;j++)
				dSign[j] = (float)str[i*(SV_SIZE_WINDOW/2)+j] - 128;
			transformFFTReIm(dSign,fft_re,fft_im,SV_SIZE_WINDOW);
			MFCC1(fft_re,fft_im,SV_SIZE_WINDOW,Vec.V,SV_SIZE_VECTOR);
			m_vec.push_back(Vec);
		}
		delete []dSign;
	}
}

int TemplateData::create(unsigned char *buf, int iSize,int feat_type)
{
	if (iSize < SV_SIZE_WINDOW)
		return 1;
	createVectors(buf,iSize,feat_type);
	return 0;
}


