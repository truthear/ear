#pragma once
#include "TemplateData.h"
#include <algorithm>
#include <string>

#define DTW_AVERAGE   0     //усреднить с существующим
#define DTW_REPLACE   1		//заменить существующий
#define DTW_INSERT   2		//вставка нового эталона
#define UP   0
#define RIGHT   1
#define UPRIGHT   2

//структура описывающая эталон
struct EtalonStruct
{
	TemplateData templ;
	string name;
	int numOfMean;
};

class Recognizer
{
public:
	Recognizer(void);
	~Recognizer(void);
	vector<EtalonStruct> m_vEt;
	vector<EtalonStruct> m_vEtCopy;
	void setWord(unsigned int i, string sWord);
	void addEtalon(EtalonStruct newEt,unsigned int Num,int iMode=DTW_AVERAGE);
	string getWord(unsigned int i);
	unsigned int getEtalonCount();
	void saveEtalons(string path);
	void loadEtalons(string path);
	void delEtalon(unsigned int Num);
	void delAllEtalons();
	int getN(unsigned int i);
	void getEtalon(EtalonStruct* &v, unsigned int Num);
	float distDTW(TemplateData E, TemplateData A);
	float dist2Vectors(float x[SV_SIZE_VECTOR],float y[SV_SIZE_VECTOR]);
	void recognize(TemplateData RecVec);
};

