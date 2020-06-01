#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "audio.h"


#define I2S_NUM		I2S_NUM_0

#define SAMPLE_RATE_NUM		9
#define WORD_LEN_NUM		4

/* 采样率设置*/
static const uint16_t SAMPLE_RATE_CFG[SAMPLE_RATE_NUM] = {48000, 32000, 24000, 16000, 12000, 8000, 0, 0, 44100};
/* 字长设置*/
static const uint8_t WORD_LEN_CFG[WORD_LEN_NUM] = {16, 20, 24, 32};


void waveheader_init(WaveHeader* wave_hdr, I2S_Params_st* i2s_params)
{
	wave_hdr->riff.ChunkID = 0x46464952;					//"RIFF"
	wave_hdr->riff.ChunkSize = 0;							//还未确定,最后需要计算
	wave_hdr->riff.Format = 0x45564157; 					//"WAVE"
	wave_hdr->fmt.ChunkID = 0x20746D66; 					//"fmt "
	wave_hdr->fmt.ChunkSize = 16; 							//大小为16个字节
	wave_hdr->fmt.AudioFormat = 0x01; 						//0x01,表示PCM;0x01,表示IMA ADPCM

	if(i2s_params->channel == I2S_CHANNEL_FMT_RIGHT_LEFT)
		wave_hdr->fmt.NumOfChannels = 2;					//双声道
	else
		wave_hdr->fmt.NumOfChannels = 1;					//单声道

	wave_hdr->fmt.SampleRate = SAMPLE_RATE_CFG[i2s_params->rate];	//采样速率
	wave_hdr->fmt.BitsPerSample = WORD_LEN_CFG[i2s_params->wlen];	//16位PCM
	wave_hdr->fmt.BlockAlign = (wave_hdr->fmt.NumOfChannels) 	\
								* (wave_hdr->fmt.BitsPerSample)/8;	//块大小=通道数*(ADC位数/8)
	wave_hdr->fmt.ByteRate = (wave_hdr->fmt.SampleRate)			\
							  * (wave_hdr->fmt.NumOfChannels)	\
							  * (wave_hdr->fmt.BitsPerSample)/8;		//字节速率=采样率*通道数*(ADC位数/8)
	wave_hdr->data.ChunkID = 0x61746164;					//"data"
	wave_hdr->data.ChunkSize = 0;							//数据大小,还需要计算
}


bool waveheader_read(WaveHeader* wave_hdr, I2S_Params_st* i2s_params)
{
	if(wave_hdr->fmt.NumOfChannels > 1)
		i2s_params->channel = I2S_CHANNEL_FMT_RIGHT_LEFT;	//双声道
	else
		i2s_params->channel = I2S_CHANNEL_FMT_ONLY_LEFT;	//单声道

	int i;

	for(i =0; i < SAMPLE_RATE_NUM; i++){
		if(wave_hdr->fmt.SampleRate == SAMPLE_RATE_CFG[i]){
			i2s_params->rate = i;
			break;
		}
	}

	if(i2s_params->rate != i)
		return false;

	for(i =0; i < WORD_LEN_NUM; i++){
		if(wave_hdr->fmt.BitsPerSample == WORD_LEN_CFG[i]){
			i2s_params->wlen = i;
			break;
		}
	}	

	return (i2s_params->wlen == i);
}


void Audio_i2s_init(I2S_Params_st* i2s_params)
{
	i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE_CFG[i2s_params->rate],
        .bits_per_sample = WORD_LEN_CFG[i2s_params->wlen],
        .channel_format = i2s_params->channel,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 2,
        .dma_buf_len = 512,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3                                //Interrupt level 1
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_I2S_SCLK,
        .ws_io_num = CONFIG_I2S_LRCK,
        .data_out_num = CONFIG_I2S_DOUT,
        .data_in_num = CONFIG_I2S_DIN
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);

#if CONFIG_GPIO_SUPPORT
#if (I2S_NUM==I2S_NUM_0)
    //clk out 
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    //REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0);
    REG_WRITE(PIN_CTRL, 0xFFFFFFF0);
#endif
#endif

	WM8978_Reset();
	WM8978_setSampleRate((i2s_params->rate)&0x0007);
#if  CONFIG_PLL_SUPPORT
	WM8978_setPLLEnable(true);
	if(i2s_params->rate == SR_44KHZ)
		WM8978_setPLLClock(true, 0x07, 0x86C226);
	else
		WM8978_setPLLClock(true, 0x08, 0x3126E9);
	WM8978_setClockControl(false, CLK_PLL, BCLK_DIV8, ((i2s_params->rate)&0x0007)+MCLK_DIV2);
#elif CONFIG_GPIO_SUPPORT
	WM8978_setClockControl(false, CLK_MCLK, BCLK_DIV1, MCLK_DIV1);
#endif

	WM8978_setI2SInterface(I2S_PHILLIPS, i2s_params->wlen);

	WM8978_setVMID(true, VMID_5K);
	WM8978_setMicBias(true, BIAS_06AVDD);

	WM8978_setTSDEnable(true);

	WM8978_setMIXEnable(true, true);

	WM8978_setDACEnable(true, true);
	WM8978_setDACMode(OSR_128X, true, false);

	WM8978_setLeftMixer(true, false, false, 0, 0);
	WM8978_setRightMixer(true, false, false, 0, 0);

	WM8978_setInvROUT2(true, false);

	WM8978_setADCEnable(true, true);
	WM8978_setADCMode(OSR_128X);

	WM8978_setEQ3DMode(EQ3D_DAC);
//	WM8978_setEQ1(0, EQ1_80HZ);
//	WM8978_setEQ2(0, EQ2_230HZ, false);
//	WM8978_setEQ3(0, EQ3_650HZ, false);
//	WM8978_setEQ4(0, EQ4_1800HZ, false);
//	WM8978_setEQ5(0x18, EQ5_5300HZ);
}


void Audio_i2s_uninit()
{
	i2s_zero_dma_buffer(I2S_NUM);
    i2s_driver_uninstall(I2S_NUM);
	WM8978_Reset();
}


void Audio_setSampleRates(uint16_t rate)
{
	WM8978_setSampleRate(rate&0x0007);

#if CONFIG_PLL_SUPPORT
	if(rate == SR_44KHZ)
		WM8978_setPLLClock(true, 0x07, 0x86C226);
	else
		WM8978_setPLLClock(true, 0x08, 0x3126E9);
	WM8978_setClockControl(false, CLK_PLL, BCLK_DIV8, (rate&0x0007)+MCLK_DIV2);
#endif

    i2s_set_sample_rates(I2S_NUM, SAMPLE_RATE_CFG[rate]);
}


void Audio_setTxclk(int sampleRate)
{
	for(int i =0; i < SAMPLE_RATE_NUM; i++){
		if(sampleRate == SAMPLE_RATE_CFG[i]){
			Audio_setSampleRates(i);
			break;
		}
	}
}


void Audio_setTxChannel(i2s_channel_fmt_t channel)
{
#if (I2S_NUM==I2S_NUM_0)
    I2S0.conf_chan.tx_chan_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? channel : (channel >> 1); // 0-two channel;1-right;2-left;3-righ;4-left
    I2S0.fifo_conf.tx_fifo_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? 0 : 1; // 0-right&left channel;1-one channel	
#else
	I2S1.conf_chan.tx_chan_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? channel : (channel >> 1); // 0-two channel;1-right;2-left;3-righ;4-left
    I2S1.fifo_conf.tx_fifo_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? 0 : 1; // 0-right&left channel;1-one channel	
#endif
}


void Audio_setRxChannel(i2s_channel_fmt_t channel)
{
#if (I2S_NUM==I2S_NUM_0)
    I2S0.conf_chan.rx_chan_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? channel : (channel >> 1); // 0-two channel;1-right;2-left;3-righ;4-left
    I2S0.fifo_conf.rx_fifo_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? 0 : 1; // 0-right&left channel;1-one channel
#else
    I2S1.conf_chan.rx_chan_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? channel : (channel >> 1); // 0-two channel;1-right;2-left;3-righ;4-left
    I2S1.fifo_conf.rx_fifo_mod = channel < I2S_CHANNEL_FMT_ONLY_RIGHT ? 0 : 1; // 0-right&left channel;1-one channel		
#endif
}


void Audio_setJD()
{
	WM8978_setSlowClkEnable(true);
	WM8978_setJDen(JD_OUT1_EN_0, JD_OUT2_EN_1);
	WM8978_setJDSel(GPIO1, true);	
}

void Audio_setLeftMic(bool mic_on)
{
	if(mic_on){
		WM8978_setInpPGAL(DIFFERENTIAL);
		WM8978_setBoostL(true, 1, 0, 0);
	}else{
		WM8978_setInpPGAL(INPUT_OFF);
		WM8978_setBoostL(false, 0, 0, 0);
	}
}


void Audio_setRightMic(bool mic_on)
{
	if(mic_on){
		WM8978_setInpPGAR(DIFFERENTIAL);
		WM8978_setBoostR(true, 1, 0, 0);
	}else{
		WM8978_setInpPGAR(INPUT_OFF);
		WM8978_setBoostR(false, 0, 0, 0);
	}
}


void Audio_setLeftMicVolume(uint8_t mic_vol)
{
	WM8978_setPGAGainL(false, mic_vol);
}


void Audio_setRightMicVolume(uint8_t mic_vol)
{
	WM8978_setPGAGainR(false, mic_vol);
}


void Audio_setEarphone(bool eph_on)
{
	WM8978_setOUT1Enable(eph_on, eph_on);
}

void Audio_setSpeaker(bool spk_on)
{
	WM8978_setOUT2Enable(spk_on, spk_on);
	WM8978_setSpkBoost(spk_on);	
}

void Audio_setEarphoneVolume(bool mute_on, uint8_t out_vol)
{
	WM8978_setLOUT1Volume(mute_on, out_vol);
	WM8978_setROUT1Volume(mute_on, out_vol);
}

void Audio_setSpeakerVolume(bool mute_on, uint8_t out_vol)
{
	WM8978_setLOUT2Volume(mute_on, out_vol);
	WM8978_setROUT2Volume(mute_on, out_vol);
}

void Audio_setEarphoneSpeaker(uint8_t vol)
{
	Audio_setEarphone(true);
	Audio_setSpeaker(true);
	Audio_setEarphoneVolume(false, vol);
	Audio_setSpeakerVolume(false, vol);
	Audio_setJD();
}

size_t Audio_i2sRead(char* dest, size_t size, TickType_t timeout)
{
	size_t read_size = 0;
	if(i2s_read(I2S_NUM, dest, size, &read_size, timeout) != ESP_OK){
		return 0;
	}
    return read_size;
}

size_t Audio_i2sWrite(const char* dest, size_t size, TickType_t timeout)
{
    size_t written_size = 0;
	if(i2s_write(I2S_NUM, dest, size, &written_size, timeout) != ESP_OK){
		return 0;
	}
	return written_size;
}

void Audio_i2s_clear()
{
	i2s_zero_dma_buffer(I2S_NUM);
}

