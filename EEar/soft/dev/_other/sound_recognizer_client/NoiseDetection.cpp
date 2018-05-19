#include "NoiseDetection.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

const int SAMPLE_RATE = 16000;
const int VAR_BLOCK_SIZE = 186;
const int ETALON_BLOCK_SIZE = 267;
const int MIN_PERIOD = 43;
const int MAX_PERIOD = 145;

const int TH_PERIOD_LEN = 75;
const int TH_PERIOD_NUM = 7;

const float TH_ZCR = 0.06;
const int TH_VARIATION_BAND_1 = 95;
const int TH_VARIATION_BAND_2 = 20;
const float TH_VAR_PART_1 = 0.81;
const float TH_VAR_PART_2 = 0.4;
const float TH_VAR_PART_3 = 0.1;
const float TH_ZCR_PART = 0.4;


NoiseDetection::NoiseDetection(void)
{
}

NoiseDetection::~NoiseDetection(void)
{
}

//нормализация громкости
void NoiseDetection::normVol(unsigned char data[],unsigned int dataSize)
{
	unsigned char max = data[0];
	float norm;
	for(unsigned int i = 1; i < dataSize;i++)
		if (data[i] > max) max = data[i];

	if (max < 255 && max > 128)
	{
		for(unsigned int i = 0; i < dataSize;i++)
		{
			norm = (float)(data[i] - 128) * 127.0 / (float)(max - 128);
			if (norm > 127) norm = 127;
			if (norm < -128) norm = -128;
			norm += 128.0;
			data[i] = (unsigned char)norm;
		}
	}
}

//проверка на квазипериодичность (присутствие голоса)
// 1 - голос присутствует
// 0 - голос отсутствует
int NoiseDetection::checkWithPeriod(unsigned char data[], unsigned int dataSize)
{
	if (dataSize <= MIN_PERIOD) return 0;
	
	unsigned int mini;
	int pos = 0;
	int periodLen = 0;
	int periodNum = 0;
	int maxPeriodNum = 0;

	int k;
	unsigned int L[MAX_PERIOD-MIN_PERIOD];

	while(pos < (dataSize - MAX_PERIOD - 1))
	{
		//инициализация
		for(int i = 0;i < MAX_PERIOD-MIN_PERIOD;i++)
			L[i]=0;

		//евклидова метрика
		k = MIN_PERIOD;
		while(k < MAX_PERIOD)
		{
			for(int i = 0;i < k;i++)
			{
				if ((pos+i+k) >= (dataSize - 1)) break;
				L[k-MIN_PERIOD] += abs(data[pos+i] - data[pos+i+k]);
			}
			k++;
		}

		//минимальная евклидова метрика
		unsigned int min = L[0];
		mini = 0;
		for(int i = 1;i < MAX_PERIOD-MIN_PERIOD;i++)
		{
			if(min > L[i])
			{
				min = L[i];
				mini = i;
			}
		}
		periodLen = mini+MIN_PERIOD;
		pos += periodLen;
		if (periodLen >= TH_PERIOD_LEN)
		{
			periodNum++;
		}
		else
		{
			if (periodNum > maxPeriodNum)
				maxPeriodNum = periodNum;
			periodNum = 0;
		}	
	}
	
	if (periodNum > maxPeriodNum)
		maxPeriodNum = periodNum;
	if (maxPeriodNum >=  TH_PERIOD_NUM)
		return 1;
	return 0;
}

int sgn_8b_unsigned(unsigned char &x) 
{
	if (x >= 128) return 1; else return -1;
}

//полосовой фильтр
void NoiseDetection::filtr(int N,int F1,int F2, unsigned char data[], unsigned int dataSize)
{
	float M_PI = 3.1415926535;
	float a1[200],a2[200],w[200];
	float C1,C2;
	if (F2 == 0) F2=1;
	C1=2*F2/(float)SAMPLE_RATE;
	C2=2*F1/(float)SAMPLE_RATE;
	a1[0]= C1;
	a2[0]= C2;
	a1[0]=a1[0]-a2[0];
	w[0]=1;
	for (int i=1;i<=N;i++)
	{
		a1[i]= C1*((sin(i*M_PI*C1))/(i*M_PI*C1));
		a2[i]= C2*((sin(i*M_PI*C2))/(i*M_PI*C2));
		a1[i]=a1[i]-a2[i];
		w[i]=(1+cos(M_PI*i/N))/2.0;
	}

	unsigned char y[MAX_DATA_SIZE];

	for(int i=0; i<=N;i++) y[i] = 128;
	for(int i=dataSize-N; i<dataSize;i++) y[i] = 128;
	for (int n = N;n < dataSize-N;n++)
	{
		float sum = 0.0;
		for(int k = -N;k <= N;k++)
		{
			sum += a1[abs(k)]*w[abs(k)]*(float)((data[n-k]) - 128);
		}
		if (sum > 128) sum = 128;
		if (sum < -127) sum = -127;
		y[n] = (unsigned char)(sum + 128);
	}
	memcpy(data,y,(dataSize)*sizeof(unsigned char));
}



//проверка на уровень вариации и частоты переходов через 0
// 1 - полезный сигнал
// 0 - шум
int NoiseDetection::checkWithVariationZCR(unsigned char data[],	unsigned int dataSize)
	
{
	int blockNum = dataSize/VAR_BLOCK_SIZE;
	if (blockNum == 0) return 0;
	unsigned int sumVar,sumZCR;
	int varOverNum = 0, zcrOverNum = 0;
	//вычисление частоты переходов через 0
	for (int i = 0;i < blockNum;i++)
	{
		sumZCR = 0;
		for (int j = (i*VAR_BLOCK_SIZE); j < ((i + 1)*VAR_BLOCK_SIZE - 1); j++)
			sumZCR += abs(sgn_8b_unsigned(data[j + 1]) - sgn_8b_unsigned(data[j]));
		if ((float)sumZCR / (2 * VAR_BLOCK_SIZE) > TH_ZCR) zcrOverNum++;
	}
	
	unsigned char dataOrig[MAX_DATA_SIZE];
	memcpy(dataOrig, data, (dataSize) * sizeof(unsigned char));
	
	filtr(100,500,100,data,dataSize);
	
	//вычисление вариации
	for (int i = 0;i < blockNum;i++)
	{
		sumVar = 0;
		for (int j = (i*VAR_BLOCK_SIZE);j < ((i+1)*VAR_BLOCK_SIZE - 1);j++)
			sumVar += abs(data[j+1] - data[j]);
		if (sumVar >= TH_VARIATION_BAND_1) varOverNum++;
	}
	
	float varRate = (float)varOverNum/blockNum;
	float zcrRate = (float)zcrOverNum/blockNum;

	printf("varRate = %.2f    zcrRate = %.2f\n",varRate, zcrRate);
	
	if (varRate > TH_VAR_PART_1)
	{
		return 1;
	}
	else
	{
		if (varRate > TH_VAR_PART_2 && zcrRate > TH_ZCR_PART)
			return 1;
	}

	filtr(100, 10, 1, dataOrig, dataSize);
	
	//вычисление вариации
	varOverNum = 0;
	for (int i = 0; i < blockNum; i++)
	{
		sumVar = 0;
		for (int j = (i*VAR_BLOCK_SIZE); j < ((i + 1)*VAR_BLOCK_SIZE - 1); j++)
			sumVar += abs(dataOrig[j + 1] - dataOrig[j]);
		if (sumVar >= TH_VARIATION_BAND_2) varOverNum++;
	}
	varRate = (float)varOverNum/blockNum;
	printf("varRate1-10 = %.2f\n", varRate);
	if (varRate > TH_VAR_PART_3)
		return 1;
	
	
	return 0;
}


// 1 - полезный сигнал
// 0 - шум
int NoiseDetection::checkNoise(unsigned char data[], unsigned int dataSize)
{
	if (dataSize > MAX_DATA_SIZE)
		dataSize = MAX_DATA_SIZE;

	//нормализация громкости
	normVol(data,dataSize);

	if (checkWithPeriod(data,dataSize) == 1)
	{
		//если голос присутствует
		printf("voice present!\n");
		return 0;
	}
	else
	{
		return checkWithVariationZCR(data,dataSize);
	}
}
