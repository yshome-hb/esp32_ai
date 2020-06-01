#ifndef _HAL_I2S_H_
#define _HAL_I2S_H_

#include "driver/i2s.h"
#include "wm8978.h"

//RIFF��
typedef struct
{
	uint32_t ChunkID;		   	//chunk id�̶�Ϊ"RIFF",��0X46464952
	uint32_t ChunkSize ;		//���ϴ�С=�ļ��ܴ�С-8
	uint32_t Format;	   		//��ʽ;WAVE,��0X45564157

}ChunkRIFF;

//fmt��
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;����̶�Ϊ"fmt ",��0X20746D66
	uint32_t ChunkSize ;		 //�Ӽ��ϴ�С(������ID��Size);����Ϊ:20.
	uint16_t AudioFormat;	  	//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	uint16_t NumOfChannels;		//ͨ������;1,��ʾ������;2,��ʾ˫����;
	uint32_t SampleRate;		//������;0X1F40,��ʾ8Khz
	uint32_t ByteRate;			//�ֽ�����;
	uint16_t BlockAlign;		//�����(�ֽ�);
	uint16_t BitsPerSample;		//�����������ݴ�С;4λADPCM,����Ϊ4
	//uint16_t ByteExtraData;	//���ӵ������ֽ�;2��; ����PCM,û���������

}ChunkFMT;

//fact��
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;����̶�Ϊ"fact",��0X74636166;����PCMû���������
	uint32_t ChunkSize ;		//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
	uint32_t FactSize;	  		//ת����PCM���ļ���С;

}ChunkFACT;

//LIST��
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;����̶�Ϊ"LIST",��0X74636166;
	uint32_t ChunkSize ;		//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.

}ChunkLIST;

//data��
typedef struct
{
	uint32_t ChunkID;		   	//chunk id;����̶�Ϊ"data",��0X5453494C
	uint32_t ChunkSize ;		//�Ӽ��ϴ�С(������ID��Size)

}ChunkDATA;

//wavͷ
typedef struct
{
	ChunkRIFF riff;				//riff��
	ChunkFMT fmt;  				//fmt��
	//ChunkFACT fact;			//fact�� ����PCM,û������ṹ��
	ChunkDATA data;				//data��

}WaveHeader;

//i2s���ò���
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





