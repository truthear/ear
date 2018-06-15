#include "WavReader.h"
#include <stdio.h>
#include <string.h>

//Chunks
struct chunk_t
{
	char ID[4]; //"data" = 0x61746164
	unsigned long size;  //Chunk data bytes
};

int WavReader::readFile(const char* fileName, 
	                    short int *&wavData,
						wav_header_t &header, 
						unsigned int &samples_count)
{
	FILE *fin = fopen(fileName, "rb");
	if(fin == NULL)
	{
		printf("(ERROR) Can't read input file: %s\n",fileName);
		return 0;
	}
	
	//Read WAV header
	if (fread(&header, sizeof(header), 1, fin) < 1) 
	{
		printf("(ERROR) Can't read input file header %s\n", fileName);
		return 0;
	}
	//Print WAV header
	/*printf("WAV File Header read: \n");
	printf("File Type: %s\n", header.chunkID);
	printf("File Size: %ld\n", header.chunkSize);
	printf("WAV Marker: %s\n", header.format);
	printf("Format Name: %s\n", header.subchunk1ID);
	printf("Format Length: %ld\n", header.subchunk1Size );
	printf("Format Type: %hd\n", header.audioFormat);
	printf("Number of Channels: %hd\n", header.numChannels);
	printf("Sample Rate: %ld\n", header.sampleRate);
	printf("Sample Rate * Bits/Sample * Channels / 8: %ld\n", header.byteRate);
	printf("Bits per Sample * Channels / 8.1: %hd\n", header.blockAlign);
	printf("Bits per Sample: %hd\n", header.bitsPerSample);*/

	//Reading file
	chunk_t chunk;
	//printf("id\t" "size\n");
	//go to data chunk
	while (true)
	{
		fread(&chunk, sizeof(chunk), 1, fin);
		//printf("%c%c%c%c\t" "%li\n", chunk.ID[0], chunk.ID[1], chunk.ID[2], chunk.ID[3], chunk.size);
		if (*(unsigned int *)&chunk.ID == 0x61746164)
			break;
		//skip chunk data bytes
		fseek(fin, chunk.size, SEEK_CUR);
	}

	//Number of samples
	int sample_size = header.bitsPerSample / 8;
	samples_count = chunk.size * 8 / header.bitsPerSample;
	//printf("Samples count = %i\n", samples_count);

	wavData = new short int[samples_count];
	memset(wavData, 0, sizeof(short int) * samples_count);

	//Reading data
	for (unsigned int i = 0; i < samples_count; i++)
		fread(&wavData[i], sample_size, 1, fin);
	
	fclose(fin);
	return 1;
}

WavReader::WavReader(void)
{
}


WavReader::~WavReader(void)
{
}
