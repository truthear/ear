#pragma once

#define MAX_DATA_SIZE 20000

class NoiseDetection
{
protected:
	void filtr(int N,int F1,int F2, unsigned char data[], unsigned int dataSize);
	int checkWithPeriod(unsigned char data[], unsigned int data_size);
	int checkWithVariation(unsigned char data[], unsigned int dataSize,
                           int threshVar, int threshMinLen);
	int checkWithVariationZCR(unsigned char data[], unsigned int dataSize);
	void normVol(unsigned char data[],unsigned int dataSize);
public:
	int checkNoise(unsigned char data[], unsigned int data_size);
	NoiseDetection(void);
	~NoiseDetection(void);
};

