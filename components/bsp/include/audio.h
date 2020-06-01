#ifndef _HAL_I2S_H_
#define _HAL_I2S_H_

#include "driver/i2s.h"
#include "wm8978.h"

//RIFF块
typedef struct
{
	uint32_t ChunkID;		   	//chunk id固定为"RIFF",即0X46464952
	uint32_t ChunkSize ;		//集合大小=文件总大小-8
	uint32_t Format;	   		//格式;WAVE,即0X45564157

}ChunkRIFF;

//fmt块
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;这里固定为"fmt ",即0X20746D66
	uint32_t ChunkSize ;		 //子集合大小(不包括ID和Size);这里为:20.
	uint16_t AudioFormat;	  	//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	uint16_t NumOfChannels;		//通道数量;1,表示单声道;2,表示双声道;
	uint32_t SampleRate;		//采样率;0X1F40,表示8Khz
	uint32_t ByteRate;			//字节速率;
	uint16_t BlockAlign;		//块对齐(字节);
	uint16_t BitsPerSample;		//单个采样数据大小;4位ADPCM,设置为4
	//uint16_t ByteExtraData;	//附加的数据字节;2个; 线性PCM,没有这个参数

}ChunkFMT;

//fact块
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;这里固定为"fact",即0X74636166;线性PCM没有这个部分
	uint32_t ChunkSize ;		//子集合大小(不包括ID和Size);这里为:4.
	uint32_t FactSize;	  		//转换成PCM的文件大小;

}ChunkFACT;

//LIST块
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;这里固定为"LIST",即0X74636166;
	uint32_t ChunkSize ;		//子集合大小(不包括ID和Size);这里为:4.

}ChunkLIST;

//data块
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;这里固定为"data",即0X5453494C
	uint32_t ChunkSize ;		//子集合大小(不包括ID和Size)

}ChunkDATA;

//wav头
typedef struct
{
	ChunkRIFF riff;				//riff块
	ChunkFMT fmt;  				//fmt块
	//ChunkFACT fact;			//fact块 线性PCM,没有这个结构体
	ChunkDATA data;				//data块

}WaveHeader;

//i2s配置参数
typedef struct {
	SampleRate_E rate;
	I2SWordLen_E wlen;
	i2s_channel_fmt_t channel;

}I2S_Params_st;


void waveheader_init(WaveHeader* wave_hdr, I2S_Params_st* i2s_params);
bool waveheader_read(WaveHeader* wave_hdr, I2S_Params_st* i2s_params);
void Audio_i2s_init(I2S_Params_st* i2s_params);
void Audio_i2s_uninit();
void Audio_setJD();
void Audio_setSampleRates(uint16_t rate);
void Audio_setTxclk(int sampleRate);
void Audio_setTxChannel(i2s_channel_fmt_t channel);
void Audio_setRxChannel(i2s_channel_fmt_t channel);
void Audio_setLeftMic(bool mic_on);
void Audio_setRightMic(bool mic_on);
void Audio_setLeftMicVolume(uint8_t mic_vol);
void Audio_setRightMicVolume(uint8_t mic_vol);
void Audio_setEarphone(bool eph_on);
void Audio_setSpeaker(bool spk_on);
void Audio_setEarphoneVolume(bool mute_on, uint8_t out_vol);
void Audio_setSpeakerVolume(bool mute_on, uint8_t out_vol);
void Audio_setEarphoneSpeaker(uint8_t vol);
size_t Audio_i2sRead(char* dest, size_t size, TickType_t timeout);
size_t Audio_i2sWrite(const char* dest, size_t size, TickType_t timeout);
void Audio_i2s_clear();


#endif





