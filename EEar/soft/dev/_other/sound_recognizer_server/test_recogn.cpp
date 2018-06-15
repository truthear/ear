#include <iostream>
#include <string>
#include <fstream>
#include <direct.h>
#include "WavReader.h"
#include "Recognizer.h"

using namespace std;

string GetCurrentWorkingDir(void) 
{
	char buff[FILENAME_MAX];
	_getcwd( buff, FILENAME_MAX );
	string current_working_dir(buff);
	return current_working_dir;
}

//преобразование из 16 в 8 бит
unsigned char Make8bit(short int sample)
{
	sample >>= 8;  
	sample ^= 0x80;
	return (sample & 0xFF);
}

int main(int argc, char* argv[]) 
{
	string listFileName;
	int train = 0;
	if(argc > 1)
	{
		if (argc > 2)
		{
			string param1(argv[1]);
			if (param1 == "-t")
				train = 1;
			listFileName = argv[2];
		}
		else
			listFileName = argv[1];
	}
	else
		return 0;
	string line;
	ifstream inFile(listFileName.c_str());
	string sLine,s1,s2;
	if(!inFile)
	{
		cout << "(ERROR) Can't read input file: " << listFileName << endl;
		return 1;
	}
	short int *wavData;
	unsigned char *data8bit;
	wav_header_t header;
	WavReader wav;
	Recognizer recg;
	unsigned int samples_count;
	string dataFileName = GetCurrentWorkingDir() + "\\etalons.dat";
	if (train == 0)
	{
		ifstream dataFile(dataFileName.c_str());
		if (dataFile.good()) 
		{
			recg.loadEtalons(dataFileName);
			//распознавание
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
					data8bit = new unsigned char[samples_count];
					if (header.bitsPerSample == 8)
					{
						for(unsigned int i = 0; i < samples_count;i++)
							data8bit[i] = (unsigned char)wavData[i]; 
					}
					if (header.bitsPerSample == 16)
					{
						for(unsigned int i = 0; i < samples_count;i++)
							data8bit[i] = Make8bit(wavData[i]); 
					}

					TemplateData templTest;
					templTest.create(data8bit,samples_count);
					string res;
					recg.recognize(templTest);
					//cout << "Result: " << res  << endl;
					//cout << "Dmin = " << d_min << "\n\n";

					delete [] wavData;
					delete [] data8bit;
				}
			}
		}
	}
	else
	{
		//создание базы эталонов
		while(getline(inFile, line))
		{
			string fileName, etName;
			string::size_type div_pos = line.find("|");
			if (div_pos != string::npos)
			{
				etName = line.substr(0,div_pos);
				etName.erase(0, etName.find_first_not_of(' '));
				etName.erase(etName.find_last_not_of(' ')+1);
				fileName = line.substr(div_pos + 1, line.size() - div_pos - 1);
				fileName.erase(0, fileName.find_first_not_of(' '));
				fileName.erase(fileName.find_last_not_of(' ')+1);
			}
			//
			cout << "Reading file: " << fileName << endl;
			if (wav.readFile(fileName.c_str(),wavData,header,samples_count))
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
				data8bit = new unsigned char[samples_count];
				if (header.bitsPerSample == 8)
				{
					for(unsigned int i = 0; i < samples_count;i++)
						data8bit[i] = (unsigned char)wavData[i]; 
				}
				if (header.bitsPerSample == 16)
				{
					for(unsigned int i = 0; i < samples_count;i++)
						data8bit[i] = Make8bit(wavData[i]); 
				}

				TemplateData templ;
				EtalonStruct et;
				et.numOfMean = 1;
				et.name = etName;
				templ.create(data8bit,samples_count);
				et.templ = templ;
				recg.addEtalon(et,0,DTW_INSERT);

				delete [] wavData;
				delete [] data8bit;
			}
		}
		inFile.clear();                 
		inFile.seekg(0, std::ios::beg); 
		inFile.close();
		recg.saveEtalons(dataFileName);
	}

	//system("pause");
	return 0;
}

