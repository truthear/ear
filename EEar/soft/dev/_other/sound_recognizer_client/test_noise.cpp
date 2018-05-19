#include <iostream>
#include <string>
#include <fstream>
#include "NoiseDetection.h"
#include "WavReader.h"

using namespace std;

//преобразование из 16 в 8 бит
unsigned char Make8bit(short int sample)
{
	sample >>= 8;  
	sample ^= 0x80;
	return (sample & 0xFF);
}

int main(int argc, char* argv[]) 
{
	string listFileName = "testlist_all.txt";
	string line;
	ifstream inFile(listFileName.c_str());
	string sLine,s1,s2;
	if(!inFile)
	{
		cout << "(ERROR) Can't read input file: " << listFileName << endl;
		return 1;
	}
	short int *wavData;
	unsigned char noiseData[MAX_DATA_SIZE];
	wav_header_t header;
	WavReader wav;
	NoiseDetection noiseDetector;
	unsigned int samples_count;
	
	while(getline(inFile, line))
	{
		cout << "Reading file: " << line << endl;
		if (wav.readFile(line.c_str(),wavData,header,samples_count))
		{
			if (header.sampleRate != 16000)
			{
				cout << "(ERROR) Input must be sampled at 16000 Hz.\n";
				continue;
			}

			if (header.numChannels != 1)
			{
				cout << "(ERROR) Input must have 1 channel.\n";
				continue;
			}

			if (header.bitsPerSample != 8 && header.bitsPerSample != 16)
			{
				cout << "(ERROR) Input data must be 8 or 16 bit.\n";
				continue;
			}

			if ( samples_count > MAX_DATA_SIZE )
			{
			  samples_count = MAX_DATA_SIZE;
			  cout << "Truncated" << endl;
			}
			
			if (header.bitsPerSample == 8)
			{
				for(unsigned int i = 0; i < samples_count;i++)
					noiseData[i] = (unsigned char)wavData[i]; 
			}
			if (header.bitsPerSample == 16)
			{
				for(unsigned int i = 0; i < samples_count;i++)
					noiseData[i] = Make8bit(wavData[i]); 
			}

			//результат 0 - шум, 1 - полезный сигнал
			cout << "Result: " << noiseDetector.checkNoise(noiseData,samples_count)  << endl << endl;
			
			delete [] wavData;
		}
	}
	//system("pause");
	return 0;
}
