#include "Recognizer.h"
#include <iostream>
#include <fstream>
#include <stdio.h>

Recognizer::Recognizer(void)
{
}


Recognizer::~Recognizer(void)
{
}

//добавление нового эталона в базу
void Recognizer::addEtalon(EtalonStruct newEt,unsigned int Num,int iMode)
{
	if (Num >= m_vEt.size() || iMode == DTW_INSERT)
	{
		m_vEt.push_back(newEt);
	}
	else
	{
        m_vEt[Num].templ = newEt.templ;
		m_vEt[Num].numOfMean = 1;
	}
}

//изменение имени указанного эталона
void Recognizer::setWord(unsigned int Num, string sWord)
{
	if (Num < m_vEt.size())
		m_vEt[Num].name = sWord;
}

string Recognizer::getWord(unsigned int Num)
{
	if (Num < m_vEt.size())
		return m_vEt[Num].name;
	else
		return "";
}

//возвращает количество эталонов в базе
unsigned int Recognizer::getEtalonCount()
{
	return m_vEt.size();
}

void Recognizer::saveEtalons(string fileName)
{
	//string fileName = path + "\\etalons.dat";
	FILE *f = fopen(fileName.c_str(),"wb");
	if (f == NULL)
	{
		cout << "(ERROR) Can't read data file: " << fileName << endl;
		return;
	}
	int iVCount,iSize;
	iSize = m_vEt.size();
	//запись количества эталонов
	fwrite(&iSize,sizeof(int),1,f);
	for (int i = 0;i < m_vEt.size();i++)
	{
		iSize = m_vEt[i].name.size();
		fwrite(&iSize,sizeof(int),1,f);
		fwrite(m_vEt[i].name.c_str(),1,iSize,f);
		iVCount = m_vEt[i].templ.getCount();
		fwrite(&iVCount,sizeof(int),1,f);
		for(int j = 0;j < iVCount;j++)
			fwrite(m_vEt[i].templ.m_vec[j].V,sizeof(float),SV_SIZE_VECTOR,f);
		fwrite(&m_vEt[i].numOfMean,sizeof(int),1,f);
	}
	fclose(f);
}


//загрузка базы эталонов дифонов из файла
void Recognizer::loadEtalons(string fileName)
{
	char buf[100];
	delAllEtalons();
	FILE *f1 = NULL;
	//string fileName = path+"\\etalons.dat";
	f1 = fopen(fileName.c_str(),"rb");
	if (f1 == NULL)
	{
		m_vEt.clear();
		cout << "(ERROR) Can't write data to file: " << fileName << endl;
		return;
	}
	int iNum;
	fread(&iNum,sizeof(int),1,f1);
	int iVCount,iSizeStr;
	FeatureVector Vec;
	for (int i = 0;i < iNum;i++)
	{
		fread(&iSizeStr,sizeof(int),1,f1);
		fread(&buf,1,iSizeStr,f1);
		buf[iSizeStr] = '\0';
        EtalonStruct newEt;
		fread(&iVCount,sizeof(int),1,f1);
		for(int j = 0;j < iVCount;j++)
		{
			fread(Vec.V,sizeof(float),SV_SIZE_VECTOR,f1);
			newEt.templ.m_vec.push_back(Vec);
		}
		fread(&newEt.numOfMean,sizeof(int),1,f1);
		newEt.name = buf;
		m_vEt.push_back(newEt);
	}
	fclose(f1);

}

//удаление всех эталонов
void Recognizer::delAllEtalons()
{
	m_vEt.clear();
}

//удаление одного эталона
void Recognizer::delEtalon(unsigned int Num)
{
	if (Num < m_vEt.size())
		m_vEt.erase(m_vEt.begin()+Num);
}

//количество реализаций для эталона с номером i
int Recognizer::getN(unsigned int Num)
{
	if (Num < m_vEt.size())
		return  m_vEt[Num].numOfMean;
	else
		return -1;
}

void Recognizer::getEtalon(EtalonStruct* &v, unsigned int Num)
{
	if (Num < m_vEt.size())
		v = &m_vEt[Num];
}

//расстояние между двумя векторами
float Recognizer::dist2Vectors(float x[SV_SIZE_VECTOR],float y[SV_SIZE_VECTOR])
{
	float d = 0;
	for(int i = 0;i<SV_SIZE_VECTOR;i++) 
		d += pow(x[i]-y[i],2);
	return sqrt(d);
}

//минимум для 3-х чисел
inline float MIN(float x, float y, float z)
{
	return min(x,min(y,z));
}

float Recognizer::distDTW(TemplateData E, TemplateData A)
{
	float ** C;
	int i,j;
	float res,g1,g2,g3,d;
	int I_max,J_max;
	I_max = E.getCount();
	J_max = A.getCount();
	//заполнение рейтинговой матрицы
	C = new float*[J_max];
	for(j = 0;j < J_max;j++)
		C[j] = new float[I_max];
	C[0][0] = 2 * dist2Vectors(E.m_vec[0].V,A.m_vec[0].V);
	for(i = 1; i < I_max;i++)
		C[0][i] = C[0][i-1] + dist2Vectors(A.m_vec[0].V,E.m_vec[i].V);
	for(j = 1;j < J_max;j++)
		C[j][0] = C[j-1][0] + dist2Vectors(A.m_vec[j].V,E.m_vec[0].V);
	for(j = 1;j < J_max;j++)
	{
		for(i = 1;i < I_max;i++)
		{
			d = dist2Vectors(A.m_vec[j].V,E.m_vec[i].V);
			g1 = C[j-1][i] + d;
			g2 = C[j-1][i-1] + 2*d;
			g3 = C[j][i-1] + d;
			C[j][i] = MIN(g1,g2,g3);
		}
	}
	res = C[J_max - 1][I_max - 1];
	float Norm = J_max + I_max;
	res /= Norm;

	for(j = 0;j < J_max; j++)
		delete[] C[j];
	delete[] C;

	return res;
}


//распознавание словаря
void Recognizer::recognize(TemplateData RecVec)
{
  vector<pair<float,string>> cmp;
	
	for(int i = 0;i < m_vEt.size();i++)
	{
		float dist = distDTW(m_vEt[i].templ,RecVec);
		string name = m_vEt[i].name;
		cmp.push_back(pair<float,string>(dist,name));
	}
	

  for ( int m = 0; m < (int)cmp.size(); m++ )	
  for ( int n = 0; n < (int)cmp.size()-1; n++ )	
      {
        if ( cmp[n].first > cmp[n+1].first )
           {
             swap(cmp[n],cmp[n+1]);
           }
      }

  for ( int n = 0; n < (int)cmp.size(); n++ )	
    printf("%.1f\t%s\n",cmp[n].first,cmp[n].second.c_str());
  printf("\n");
	
}









