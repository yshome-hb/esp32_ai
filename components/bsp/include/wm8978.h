#ifndef _WM8978_H_
#define _WM8978_H_

#include <stdint.h>
#include <stdbool.h>

#define WM8978_ADDR		0X1A

#define bit0  0x00
#define bit1  0x01
#define bit2  0x02
#define bit3  0x03
#define bit4  0x04
#define bit5  0x05
#define bit6  0x06
#define bit7  0x07
#define bit8  0x08

/********************** R1 **********************/
#define BUFIOEN		bit2
#define BIASEN		bit3
#define MICBEN		bit4
#define PLLEN		bit5
#define OUT3MIXEN	bit6
#define OUT4MIXEN	bit7
#define BUFDCOPEN	bit8

/********************** R2 **********************/
#define ADCENL		bit0
#define ADCENR		bit1
#define INPPGAENL	bit2
#define INPPGAENR	bit3
#define BOOSTENL	bit4
#define BOOSTENR	bit5
#define SLEEP		bit6
#define LOUT1EN		bit7
#define ROUT1EN		bit8

/********************** R3 **********************/
#define DACENL		bit0
#define DACENR		bit1
#define LMIXEN		bit2
#define RMIXEN		bit3
#define ROUT2EN		bit5
#define LOUT2EN		bit6
#define OUT3EN		bit7
#define OUT4EN		bit8

/********************** R4 **********************/
#define MONO		bit0
#define ADCLRSWAP	bit1
#define DACLRSWAP	bit2
#define LRP			bit7
#define BCP			bit8

/********************** R5 **********************/
#define LOOPBACK	bit0
#define WL8			bit5

/********************** R6 **********************/
#define MS			bit0
#define CLKSEL		bit8

/********************** R7 **********************/
#define SLOWCLKEN	bit0

/********************** R8 **********************/
#define GPIO1POL	bit3

/********************** R9 **********************/
#define JD_EN		bit6

/********************** R10 **********************/
#define DACPOLL		bit0
#define DACPOLR		bit1
#define AMUTE		bit2
#define DACOSR		bit3
#define SOFTMUTE	bit6

/********************** R11 **********************/
#define DACVU		bit8

/********************** R13 **********************/
#define OUT1_EN_0	bit0
#define OUT2_EN_0	bit1
#define OUT3_EN_0	bit2
#define OUT4_EN_0	bit3
#define OUT1_EN_1	bit4
#define OUT2_EN_1	bit5
#define OUT3_EN_1	bit6
#define OUT4_EN_1	bit7

/********************** R14 **********************/
#define ADCLPOL		bit0
#define ADCRPOL		bit1
#define ADCOSR128	bit3
#define HPFAPP		bit7
#define HPFEN		bit8

/********************** R15 **********************/
#define ADCVU		bit8

/********************** R18 **********************/
#define EQ3DMODE	bit8

/********************** R19 **********************/
#define EQ2BW		bit8

/********************** R20 **********************/
#define EQ3BW		bit8

/********************** R21 **********************/
#define EQ4BW		bit8

/********************** R24 **********************/
#define LIMEN		bit8

/********************** R27 **********************/
#define NFEN		bit7
#define NFU			bit8

/********************** R32 **********************/
#define ALCSEL_R	bit7
#define ALCSEL_L	bit8

/********************** R35 **********************/
#define NGATEN		bit3

/********************** R36 **********************/
#define PLLPRESCALE	bit4

/********************** R43 **********************/
#define BEEPEN			bit0
#define INVROUT2		bit4
#define MUTERPGA2INV	bit5

/********************** R44 **********************/
#define LIP2INPPGA		bit0
#define LIN2INPPGA		bit1
#define L2_2INPPGA		bit2
#define RIP2INPPGA		bit4
#define RIN2INPPGA		bit5
#define R2_2INPPGA		bit6
#define MBVSEL			bit8

/********************** R45 **********************/
#define INPPGAMUTEL		bit6
#define INPPGAZCL		bit7
#define INPPGAUPDATE	bit8

/********************** R46 **********************/
#define INPPGAMUTER		bit6
#define INPPGAZCR		bit7

/********************** R47 **********************/
#define PGABOOSTL	bit8

/********************** R48 **********************/
#define PGABOOSTR	bit8

/********************** R49 **********************/
#define VROI		bit0
#define TSDEN		bit1
#define SPKBOOST	bit2
#define OUT3BOOST	bit3
#define OUT4BOOST	bit4
#define DACR2LMIX	bit5
#define DACL2RMIX	bit6

/********************** R50 **********************/
#define DACL2LMIX	bit0
#define BYPL2LMIX	bit1
#define AUXL2LMIX	bit5

/********************** R51 **********************/
#define DACR2RMIX	bit0
#define BYPR2RMIX	bit1
#define AUXR2RMIX	bit5

/********************** R52 **********************/
#define LOUT1MUTE	bit6
#define LOUT1ZC		bit7
#define HPVU		bit8

/********************** R53 **********************/
#define ROUT1MUTE	bit6
#define ROUT1ZC		bit7

/********************** R54 **********************/
#define LOUT2MUTE	bit6
#define LOUT2ZC		bit7
#define SPKVU		bit8

/********************** R55 **********************/
#define ROUT2MUTE	bit6
#define ROUT2ZC		bit7

/********************** R56 **********************/
#define LDAC2OUT3	bit0
#define LMIX2OUT3	bit1
#define BYPL2OUT3	bit2
#define OUT4_2OUT3	bit3
#define OUT3MUTE	bit6

/********************** R57 **********************/
#define RDAC2OUT4	bit0
#define RMIX2OUT4	bit1
#define BYPL2OUT4	bit2
#define LDAC2OUT4	bit3
#define LMIX2OUT4	bit4
#define HALFSIG		bit5
#define OUT4MUTE	bit6

/**********************  register  **********************/
#define R0_RESET					0x00
#define R1_POWER_MANAGEMENT_1		0x01
#define R2_POWER_MANAGEMENT_2		0x02
#define R3_POWER_MANAGEMENT_3		0x03
#define R4_AUDIO_INTERFACE			0x04
#define R5_COMPANDING_CONTROL		0x05
#define R6_CLOCK_GEN_CONTROL		0x06
#define R7_ADDITIONAL_CONTROL		0x07
#define R8_GPIO_CONTROL				0x08
#define R9_JACK_DETECT_CONTROL_1	0x09
#define R10_DAC_CONTROL				0x0A
#define R11_LEFT_DAC_DIGITAL_VOLUME		0x0B
#define R12_RIGHT_DAC_DIGITAL_VOLUME	0x0C
#define R13_JACK_DETECT_CONTROL_2		0x0D
#define R14_ADC_CONTROL					0x0E
#define R15_LEFT_ADC_DIGITAL_VOLUME		0x0F
#define R16_RIGHT_ADC_DIGITAL_VOLUME	0x10
#define R18_EQ1						0x12
#define R19_EQ2						0x13
#define R20_EQ3						0x14
#define R21_EQ4						0x15
#define R22_EQ5						0x16
#define R24_DAC_LIMITER_1			0x18
#define R25_DAC_LIMITER_2			0x19
#define R27_NOTCH_FILTER_1			0x1b
#define R28_NOTCH_FILTER_2			0x1c
#define R29_NOTCH_FILTER_3			0x1d
#define R30_NOTCH_FILTER_4			0x1e
#define R32_ALC_CONTROL_1			0x20
#define R33_ALC_CONTROL_2			0x21
#define R34_ALC_CONTROL_3			0x22
#define R35_NOISE_GATE				0x23
#define R36_PLL_N					0x24
#define R37_PLL_K1					0x25
#define R38_PLL_K2					0x26
#define R39_PLL_K3					0x27
#define R41_3D_CONTROL				0x29
#define R43_BEEP_CONTROL			0x2b
#define R44_INPUT_CONTROL			0x2c
#define R45_LEFT_INP_PGA_CONTROL	0x2d
#define R46_RIGHT_INP_PGA_CONTROL	0x2e
#define R47_LEFT_ADC_BOOST_CONTROL	0x2f
#define R48_RIGHT_ADC_BOOST_CONTROL	0x30
#define R49_OUTPUT_CONTROL			0x31
#define R50_LEFT_MIXER_CONTROL		0x32
#define R51_RIGHT_MIXER_CONTROL		0x33
#define R52_LOUT1_HP_CONTROL		0x34
#define R53_ROUT1_HP_CONTROL		0x35
#define R54_LOUT2_SPK_CONTROL		0x36
#define R55_ROUT2_SPK_CONTROL		0x37
#define R56_OUT3_MIXER_CONTROL		0x38
#define R57_OUT4_MIXER_CONTROL		0x39


/*---------------------------------------------------------------------------------------------*/
/*------------------------  I2S接口模式  -------------------------------------------*/
typedef enum
{
	I2S_LSB	 		= 0x0000,	/* 右对齐  */
	I2S_MSB	 		= 0x0001,	/* 左对齐  */
	I2S_PHILLIPS	= 0x0002,	/* I2S */
	I2S_PCM	 		= 0x0003,	/* DSP/PCM  */

}I2SInterfaceMode_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  I2S字长  -------------------------------------------*/
typedef enum
{
	I2S_16BIT	= 0x0000,	/* 16位 */
	I2S_20BIT	= 0x0001,	/* 20位 */
	I2S_24BIT	= 0x0002,	/* 24位 */
	I2S_32BIT	= 0x0003,	/* 32位 */

}I2SWordLen_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  I2S控制模式  -------------------------------------------*/
typedef enum
{
	I2S_SLAVE	= 0x0000,	/* 从模式 */
	I2S_MASTER	= 0x0001,	/* 主模式 */

}I2SMode_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  时钟源  -------------------------------------------*/
typedef enum
{
	CLK_MCLK	= 0x0000,	/* MCLK */
	CLK_PLL		= 0x0001,	/* PLL输出 */

}ClocKSel_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  BCLK分频系数  -------------------------------------------*/
typedef enum
{
	BCLK_DIV1	= 0x0000,	/* 1分频 */
	BCLK_DIV2	= 0x0001,	/* 2分频 */
	BCLK_DIV4	= 0x0002,	/* 4分频 */
	BCLK_DIV8	= 0x0003,	/* 8分频 */
	BCLK_DIV16	= 0x0004,	/* 16分频 */
	BCLK_DIV32	= 0x0005,	/* 32分频 */

}BclkDiv_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  MCLK分频系数  -------------------------------------------*/
typedef enum
{
	MCLK_DIV1	= 0x0000,	/* 1分频 */
	MCLK_DIV1_5	= 0x0001,	/* 1.5分频 */
	MCLK_DIV2	= 0x0002,	/* 2分频 */
	MCLK_DIV3	= 0x0003,	/* 3分频 */
	MCLK_DIV4	= 0x0004,	/* 4分频 */
	MCLK_DIV6	= 0x0005,	/* 6分频 */
	MCLK_DIV8	= 0x0006,	/* 8分频 */
	MCLK_DIV12	= 0x0007,	/* 12分频 */

}MclkDiv_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  VMID串联电阻  -------------------------------------------*/
typedef enum
{
	VMID_OPEN	= 0x0000,	/* 开路 */
	VMID_75K	= 0x0001,	/* 75kΩ */
	VMID_300K	= 0x0002,	/* 300kΩ */
	VMID_5K		= 0x0003,	/* 5kΩ */

}VmidSel_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  音频采样率  -------------------------------------------*/
typedef enum
{
	SR_48KHZ	= 0x0000,	/* 48kHz */
	SR_32KHZ	= 0x0001,	/* 32kHz */
	SR_24KHZ	= 0x0002,	/* 24kHz */
	SR_16KHZ	= 0x0003,	/* 16kHz */
	SR_12KHZ	= 0x0004,	/* 12kHz */
	SR_8KHZ		= 0x0005,	/* 8kHz */
	SR_44KHZ	= 0x0008,	/* 44kHz */

}SampleRate_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  GPIO1引脚功能  -------------------------------------------*/
typedef enum
{
	INPUT		= 0x0000,	/* 输入 */
	RESERVED	= 0x0001,	/* 保留 */
	TEMP_OK		= 0x0002,	/* 温度正常 */
	AMUTE_ACTIVE	= 0x0003,	/* 静音 */
	PLL_CLK		= 0x0004,	/* PLL时钟输出 */
	PLL_LOCK	= 0x0005,	/* PLL锁定 */
	LOGIC_1		= 0x0006,	/* 逻辑1 */
	LOGIC_0		= 0x0007,	/* 逻辑0 */

}Gpio1Sel_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  插座检测引脚 -------------------------------------------*/
typedef enum
{
	GPIO1	= 0x0000,
	GPIO2	= 0x0001,
	GPIO3	= 0x0002,

}JDSel_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  插座检测输出使能 -------------------------------------------*/
typedef enum
{
	JD_OUT1_EN_0	= (0x01<<OUT1_EN_0),
	JD_OUT2_EN_0	= (0x01<<OUT2_EN_0),
	JD_OUT3_EN_0	= (0x01<<OUT3_EN_0),
	JD_OUT4_EN_0	= (0x01<<OUT4_EN_0),
	JD_OUT1_EN_1	= (0x01<<OUT1_EN_1),
	JD_OUT2_EN_1	= (0x01<<OUT2_EN_1),
	JD_OUT3_EN_1	= (0x01<<OUT3_EN_1),
	JD_OUT4_EN_1	= (0x01<<OUT4_EN_1),

}JD_OUT_EN_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  EQ3D模式  -------------------------------------------*/
typedef enum
{
	EQ3D_ADC	= 0x0000,	/* 应用于ADC */
	EQ3D_DAC	= 0x0001,	/* 应用于DAC */

}Eq3dMode_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  EQ1截止频率  -------------------------------------------*/
typedef enum
{
	EQ1_80HZ	= 0x0000,	/* 80Hz */
	EQ1_105HZ	= 0x0001,	/* 105Hz */
	EQ1_135HZ	= 0x0002,	/* 135Hz */
	EQ1_175HZ	= 0x0003,	/* 175Hz */

}Eq1Cutoff_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  EQ2截止频率  -------------------------------------------*/
typedef enum
{
	EQ2_230HZ	= 0x0000,	/* 230Hz */
	EQ2_300HZ	= 0x0001,	/* 300Hz */
	EQ2_385HZ	= 0x0002,	/* 385Hz */
	EQ2_500HZ	= 0x0003,	/* 500Hz */

}Eq2Cutoff_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  EQ3截止频率  -------------------------------------------*/
typedef enum
{
	EQ3_650HZ	= 0x0000,	/* 650Hz */
	EQ3_850HZ	= 0x0001,	/* 850Hz */
	EQ3_1100HZ	= 0x0002,	/* 1100Hz */
	EQ3_1400HZ	= 0x0003,	/* 1400Hz */

}Eq3Cutoff_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  EQ4截止频率  -------------------------------------------*/
typedef enum
{
	EQ4_1800HZ	= 0x0000,	/* 1800Hz */
	EQ4_2400HZ	= 0x0001,	/* 2400Hz */
	EQ4_3200HZ	= 0x0002,	/* 3200Hz */
	EQ4_4100HZ	= 0x0003,	/* 4100Hz */

}Eq4Cutoff_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  EQ5截止频率  -------------------------------------------*/
typedef enum
{
	EQ5_5300HZ	= 0x0000,	/* 5300Hz */
	EQ5_6900HZ	= 0x0001,	/* 6900Hz */
	EQ5_9000HZ	= 0x0002,	/* 9000Hz */
	EQ5_11700HZ	= 0x0003,	/* 11700Hz */

}Eq5Cutoff_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  OSR过采样率  -------------------------------------------*/
typedef enum
{
	OSR_64X		= 0x0000,	/* 64X过采样率 */
	OSR_128X	= 0x0001,	/* 128X过菜样率 */

}OverSampleRate_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  PGA增益放大器配置部分  -------------------------------------------*/
typedef enum
{
	INPUT_OFF		= 0x0000,	/* 无输入 */
	SING_ENDED	 	= 0x0001,	/* 单端输入  */
	DIFFERENTIAL	= 0x0002,	/* 差分输入  */
	DOUBLE_DIFF		= 0x0003, 	/* 双差分输入 */

}InpPgaMode_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  麦克风偏置电压  -------------------------------------------*/
typedef enum
{
	BIAS_09AVDD	= 0x0000,	/* 0.9*AVDD */
	BIAS_06AVDD	= 0x0001,	/* 0.6*AVDD  */

}MicBiasV_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  ALC使能配置部分  -------------------------------------------*/
typedef enum
{
	ALC_OFF		= 0x0000,	/* ALC不使能 */
	ALC_ENR	 	= 0x0001,	/* 右通道ALC使能  */
	ALC_ENL		= 0x0002,	/* 左通道ALC使能  */
	ALC_ENA		= 0x0003, 	/* 两个通道ALC使能 */

}AlcEnable_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  ALC模式配置部分  -------------------------------------------*/
typedef enum
{
	ALC_MODE	= 0x0000,	/* ALC模式 */
	LIM_MODE	= 0x0001,	/* 限幅模式  */

}AlcMode_E;

/*---------------------------------------------------------------------------------------------*/
/*------------------------  VREF到模拟输出的阻抗  -------------------------------------------*/
typedef enum
{
	VREF_1K		= 0x0000,	/* 1kΩ */
	VREF_30K	= 0x0001,	/* 30kΩ  */

}Vroi_E;

void WM8978_Reset(void);
void WM8978_Sleep(bool sleep_on);
void WM8978_setI2SInterface(I2SInterfaceMode_E i2s_mode, I2SWordLen_E word_len);
void WM8978_setLoopback(bool lpbk_on);
void WM8978_setClockControl(I2SMode_E ms_on, ClocKSel_E clk_sel, BclkDiv_E blck_div, MclkDiv_E mclk_div);
void WM8978_setVMID(bool bias_en, VmidSel_E vmid_sel);
void WM8978_setPLLEnable(bool pll_on);
void WM8978_setPLLClock(bool prescale_on, uint8_t pll_n, uint32_t pll_k);
void WM8978_setSampleRate(SampleRate_E sample_rate);
void WM8978_setADCEnable(bool l_en, bool r_en);
void WM8978_setADCMode(OverSampleRate_E adc_osr);
void WM8978_setADCVolume(uint8_t l_vol, uint8_t r_vol);
void WM8978_setInpPGAL(InpPgaMode_E inp_input);
void WM8978_setInpPGAR(InpPgaMode_E inp_input);
void WM8978_setPGAGainL(bool mute_on, uint8_t pga_vol);
void WM8978_setPGAGainR(bool mute_on, uint8_t pga_vol);
void WM8978_setBoostL(bool boost_on, uint8_t pga_vol, uint8_t aux_vol, uint8_t l_vol);
void WM8978_setBoostR(bool boost_on, uint8_t pga_vol, uint8_t aux_vol, uint8_t l_vol);
void WM8978_setMicBias(bool micb_on, MicBiasV_E mbvsel);
void WM8978_setALCGain(AlcEnable_E alc_on, uint8_t min_gain, uint8_t max_gain);
void WM8978_setALCMode(AlcMode_E alc_mode, uint8_t alc_lvl, uint8_t alc_hld, uint8_t alc_dcy, uint8_t alc_atk);
void WM8978_setALCNoiseGate(bool ngate_on, uint8_t ngate_th);
void WM8978_setEQ3DMode(Eq3dMode_E eq3d_mode);
void WM8978_setEQ1(uint8_t eq_gain, Eq1Cutoff_E eq1c);
void WM8978_setEQ2(uint8_t eq_gain, Eq2Cutoff_E eq2c, bool wb_on);
void WM8978_setEQ3(uint8_t eq_gain, Eq3Cutoff_E eq3c, bool wb_on);
void WM8978_setEQ4(uint8_t eq_gain, Eq4Cutoff_E eq4c, bool wb_on);
void WM8978_setEQ5(uint8_t eq_gain, Eq5Cutoff_E eq5c);
void WM8978_set3DDepth(uint8_t depth);
void WM8978_setNotchFilter(bool nf_on, uint16_t nf_a0, uint16_t nf_a1);
void WM8978_setDACEnable(bool l_en, bool r_en);
void WM8978_setDACMode(OverSampleRate_E dac_osr, bool a_mute, bool soft_mute);
void WM8978_setDACVolume(uint8_t l_vol, uint8_t r_vol);
void WM8978_setDACLimiter(bool lim_on, uint8_t lim_boost, uint8_t lim_lvl, uint8_t lim_dcy, uint8_t lim_atk);
void WM8978_setOUT1Enable(bool l_en, bool r_en);
void WM8978_setOUT2Enable(bool l_en, bool r_en);
void WM8978_setOUT3Enable(bool out_en);
void WM8978_setOUT4Enable(bool out_en);
void WM8978_setMIXEnable(bool l_en, bool r_en);
void WM8978_setLeftMixer(bool dac_on, bool byp_on, bool aux_on, uint8_t byp_vol, uint8_t aux_vol);
void WM8978_setRightMixer(bool dac_on, bool byp_on, bool aux_on, uint8_t byp_vol, uint8_t aux_vol);
void WM8978_setOUT3MIXEnable(bool mix_en);
void WM8978_setOUT3Mixer(bool mute_on, bool out4_on, bool adc_on, bool mix_on, bool dac_on);
void WM8978_setOUT4MIXEnable(bool mix_en);
void WM8978_setOUT4Mixer(bool mute_on, bool lmix_on, bool ldac_on, bool adc_on, bool rmix_on, bool rdac_on);
void WM8978_setLOUT1Volume(bool mute_on, uint8_t out_vol);
void WM8978_setROUT1Volume(bool mute_on, uint8_t out_vol);
void WM8978_setSpkBoost(bool boost_on);
void WM8978_setOUT3Boost(bool boost_on);
void WM8978_setOUT4Boost(bool boost_on);
void WM8978_setInvROUT2(bool inv_on, bool mute_on);
void WM8978_setBeepVol(bool beep_on, uint8_t beep_vol);
void WM8978_setLOUT2Volume(bool mute_on, uint8_t out_vol);
void WM8978_setROUT2Volume(bool mute_on, uint8_t out_vol);
void WM8978_setTSDEnable(bool tsd_en);
void WM8978_setBufIO(bool bufio_en, Vroi_E vroi);
void WM8978_setSlowClkEnable(bool enable);
void WM8978_setGPIO1(Gpio1Sel_E gpio_sel, bool gpio_pol);
void WM8978_setJDSel(JDSel_E jd_sel, bool jd_en);
void WM8978_setJDen(uint8_t en_0, uint8_t en_1);


#endif

