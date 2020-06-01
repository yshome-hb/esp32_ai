#include <string.h>
#include "I2Cdev.h"
#include "wm8978.h"

#ifndef BIT
#define BIT(x) (1ULL << (x))
#endif

#define bitRead(value, bit)  (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) 	 ((value) |= BIT(bit))
#define bitClear(value, bit) ((value) &= ~BIT(bit))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

//WM8978寄存器值缓存区(总共58个寄存器,0~57),占用116字节内存
//因为WM8978的IIC操作不支持读操作,所以在本地保存所有寄存器值
//写WM8978寄存器时,同步更新到本地寄存器值,读寄存器时,直接返回本地保存的寄存器值.
//注意:WM8978的寄存器值是9位的,所以要用uint16_t来存储
static uint16_t WM8978_REGVAL_TBL[58]=
{
	0X0000,0X0000,0X0000,0X0000,0X0050,0X0000,0X0140,0X0000,
	0X0000,0X0000,0X0000,0X00FF,0X00FF,0X0000,0X0100,0X00FF,
	0X00FF,0X0000,0X012C,0X002C,0X002C,0X002C,0X002C,0X0000,
	0X0032,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,
	0X0038,0X000B,0X0032,0X0000,0X0008,0X000C,0X0093,0X00E9,
	0X0000,0X0000,0X0000,0X0000,0X0003,0X0010,0X0010,0X0100,
	0X0100,0X0002,0X0001,0X0001,0X0039,0X0039,0X0039,0X0039,
	0X0001,0X0001
}; 

//WM8978 write register
// Write the value  of the local register
// reg: Register Address
// val: Register value
static void WM8978_writeReg(uint8_t reg, uint16_t val)
{
	uint8_t buf[2];
	buf[0] = ((val>>8)&0x01)|(reg<<1);
	buf[1] = (uint8_t)(val&0xff);

	WM8978_REGVAL_TBL[reg] = val;
	I2C_writeByte(WM8978_ADDR, buf[0], buf[1]);
}

// WM8978 read register
// Reads the value  of the local register buffer zone
// reg: Register Address
// Return Value: Register value
static uint16_t WM8978_readReg(uint8_t reg)
{
	return WM8978_REGVAL_TBL[reg];
}

/**
	* @brief  复位wm8978，所有的寄存器值恢复到缺省值
	* @param  无
	* @retval 无
	*/
void WM8978_Reset(void)
{
	/* wm8978寄存器缺省值 */
	const uint16_t reg_default[] = {
	0x000, 0x000, 0x000, 0x000, 0x050, 0x000, 0x140, 0x000,
	0x000, 0x000, 0x000, 0x0FF, 0x0FF, 0x000, 0x000, 0x0FF,
	0x0FF, 0x000, 0x12C, 0x02C, 0x02C, 0x02C, 0x02C, 0x000,
	0x032, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x038, 0x00B, 0x032, 0x000, 0x008, 0x00C, 0x093, 0x0E9,
	0x000, 0x000, 0x000, 0x000, 0x003, 0x010, 0x010, 0x100,
	0x100, 0x002, 0x001, 0x001, 0x039, 0x039, 0x039, 0x039,
	0x001, 0x001
	};

	WM8978_writeReg(R0_RESET, 0);
	memcpy(WM8978_REGVAL_TBL, reg_default, sizeof(WM8978_REGVAL_TBL));
}

/**
	* @brief  wm8978待机模式
	* @param  sleep_on: 待机模式使能
	* @retval 无
	*/
void WM8978_Sleep(bool sleep_on)
{
	uint16_t regval;
	regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
	bitWrite(regval, SLEEP, sleep_on);
	WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);
}

/**
	* @brief  配置WM8978的音频接口(I2S)
	* @param  i2s_mode : 接口模式， word_len: 字长
	* @retval 无
	*/
void WM8978_setI2SInterface(I2SInterfaceMode_E i2s_mode, I2SWordLen_E word_len)
{
	uint16_t regval;
	regval = WM8978_readReg(R4_AUDIO_INTERFACE);
	regval &= 0x0187;
	regval |= (i2s_mode<<3);
	regval |= (word_len<<5);
	WM8978_writeReg(R4_AUDIO_INTERFACE, regval);
}

/**
	* @brief  数字环回功能
	* @param  lpbk_on : 数字环回功能使能
	* @retval 无
	*/
void WM8978_setLoopback(bool lpbk_on)
{
	uint16_t regval;
	regval = WM8978_readReg(R5_COMPANDING_CONTROL);
	bitWrite(regval, LOOPBACK, lpbk_on);
	WM8978_writeReg(R5_COMPANDING_CONTROL, regval);
}

/**
	* @brief  配置WM8978的时钟
	* @param  ms_on : 主/从， clk_sel: 时钟源， bclk_div: bclk分频系数， mclk_div: mclk分频系数
	* @retval 无
	*/
void WM8978_setClockControl(I2SMode_E ms_on, ClocKSel_E clk_sel, BclkDiv_E bclk_div, MclkDiv_E mclk_div)
{
	uint16_t regval = 0x0000;
	bitWrite(regval, MS, ms_on);
	bitWrite(regval, CLKSEL, clk_sel);
	regval |= (bclk_div<<2);
	regval |= (mclk_div<<5);
	WM8978_writeReg(R6_CLOCK_GEN_CONTROL, regval);
}

/**
	* @brief  配置VMID使能
	* @param  bias_en: 偏置使能，vmid_sel: VMID串联电阻
	* @retval 无
	*/
void WM8978_setVMID(bool bias_en, VmidSel_E vmid_sel)
{
	uint16_t regval;
	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, BIASEN, bias_en);
	regval &= 0x01FC;
	regval |= vmid_sel;
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);
}

/**
	* @brief  配置PLL使能
	* @param  pll_on: pll使能
	* @retval 无
	*/
void WM8978_setPLLEnable(bool pll_on)
{
	uint16_t regval;
	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, PLLEN, pll_on);
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);
}

/**
	* @brief  配置PLL时钟
	* @param  prescale_on: 时钟2分频， pll_n: 比例整数部分， pll_k: 比例小数部分
	* @retval 无
	*/
void WM8978_setPLLClock(bool prescale_on, uint8_t pll_n, uint32_t pll_k)
{
	uint16_t regval = 0x0000;
	bitWrite(regval, PLLPRESCALE, prescale_on);
	if(pll_n < 5)
		pll_n = 5;
	else if(pll_n > 13)
		pll_n = 13;
	regval |= pll_n;
	WM8978_writeReg(R36_PLL_N, regval);

	regval = pll_k>>18;
	WM8978_writeReg(R37_PLL_K1, regval);
	regval = pll_k>>9;
	WM8978_writeReg(R38_PLL_K2, regval);
	regval = pll_k;
	WM8978_writeReg(R39_PLL_K3, regval);	
}

/**
	* @brief  配置WM8978采样率
	* @param  sample_rate: 采样率
	* @retval 无
	*/
void WM8978_setSampleRate(SampleRate_E sample_rate)
{
	uint16_t regval;
	regval = WM8978_readReg(R7_ADDITIONAL_CONTROL);
	regval &= 0x01F1;
	regval |= (sample_rate<<1);
	WM8978_writeReg(R7_ADDITIONAL_CONTROL, regval);
}

/**
	* @brief  ADC使能
	* @param  l_en: 左通道使能， r_en: 右通道使能
	* @retval 无
	*/
void WM8978_setADCEnable(bool l_en, bool r_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
	bitWrite(regval, ADCENL, l_en);
	bitWrite(regval, ADCENR, r_en);
	WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);
}

/**
	* @brief  ADC模式配置
	* @param  adc_osr: 过采样率选择
	* @retval 无
	*/
void WM8978_setADCMode(OverSampleRate_E adc_osr)
{
	uint16_t regval;
	regval = WM8978_readReg(R14_ADC_CONTROL);
	bitWrite(regval, ADCOSR128, adc_osr);
	WM8978_writeReg(R14_ADC_CONTROL, regval);
}

/**
	* @brief  ADC音量
	* @param  l_vol: 左通道音量， r_vol: 右通道音量
	* @retval 无
	*/
void WM8978_setADCVolume(uint8_t l_vol, uint8_t r_vol)
{
	uint16_t regval;
	regval = l_vol;
	WM8978_writeReg(R15_LEFT_ADC_DIGITAL_VOLUME, regval);
	regval = r_vol;
	bitSet(regval, ADCVU);
	WM8978_writeReg(R16_RIGHT_ADC_DIGITAL_VOLUME, regval);
}

/**
	* @brief  左PGA增益放大器设置
	* @param  inp_input: 输入类型
	* @retval 无
	*/
void WM8978_setInpPGAL(InpPgaMode_E inp_input)
{
	uint16_t regval;
	switch(inp_input)
	{
		case INPUT_OFF:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitClear(regval, INPPGAENL);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);
			break;
		case SING_ENDED:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitSet(regval, INPPGAENL);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

			regval = WM8978_readReg(R44_INPUT_CONTROL);
			bitClear(regval, LIP2INPPGA);
			bitSet(regval, LIN2INPPGA);
			bitClear(regval, L2_2INPPGA);
			WM8978_writeReg(R44_INPUT_CONTROL, regval);
			break;
		case DIFFERENTIAL:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitSet(regval, INPPGAENL);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

			regval = WM8978_readReg(R44_INPUT_CONTROL);
			bitSet(regval, LIP2INPPGA);
			bitSet(regval, LIN2INPPGA);
			bitClear(regval, L2_2INPPGA);
			WM8978_writeReg(R44_INPUT_CONTROL, regval);
			break;
		case DOUBLE_DIFF:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitSet(regval, INPPGAENL);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

			regval = WM8978_readReg(R44_INPUT_CONTROL);
			bitSet(regval, LIP2INPPGA);
			bitSet(regval, LIN2INPPGA);
			bitSet(regval, L2_2INPPGA);
			WM8978_writeReg(R44_INPUT_CONTROL, regval);
			break;

		default:
			break;
	}
}

/**
	* @brief  右PGA增益放大器设置
	* @param  inp_input: 输入类型
	* @retval 无
	*/
void WM8978_setInpPGAR(InpPgaMode_E inp_input)
{
	uint16_t regval;
	switch(inp_input)
	{
		case INPUT_OFF:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitClear(regval, INPPGAENR);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);
			break;
		case SING_ENDED:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitSet(regval, INPPGAENR);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

			regval = WM8978_readReg(R44_INPUT_CONTROL);
			bitClear(regval, RIP2INPPGA);
			bitSet(regval, RIN2INPPGA);
			bitClear(regval, R2_2INPPGA);
			WM8978_writeReg(R44_INPUT_CONTROL, regval);
			break;
		case DIFFERENTIAL:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitSet(regval, INPPGAENR);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

			regval = WM8978_readReg(R44_INPUT_CONTROL);
			bitSet(regval, RIP2INPPGA);
			bitSet(regval, RIN2INPPGA);
			bitClear(regval, R2_2INPPGA);
			WM8978_writeReg(R44_INPUT_CONTROL, regval);
			break;
		case DOUBLE_DIFF:
			regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
			bitSet(regval, INPPGAENR);
			WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

			regval = WM8978_readReg(R44_INPUT_CONTROL);
			bitSet(regval, RIP2INPPGA);
			bitSet(regval, RIN2INPPGA);
			bitSet(regval, R2_2INPPGA);
			WM8978_writeReg(R44_INPUT_CONTROL, regval);
			break;

		default:
			break;
	}
}

/**
	* @brief  左PGA增益放大器增益
	* @param  mute_on: PGA消声，pga_vol: PGA音量
	* @retval 无
	*/
void WM8978_setPGAGainL(bool mute_on, uint8_t pga_vol)
{
	uint16_t regval = 0x0000;

	if(pga_vol > 63)
		pga_vol = 63;
	regval |= pga_vol;
	bitWrite(regval, INPPGAMUTEL, mute_on);
	bitSet(regval, INPPGAZCL);
	bitSet(regval, INPPGAUPDATE);
	WM8978_writeReg(R45_LEFT_INP_PGA_CONTROL, regval);
}

/**
	* @brief  右PGA增益放大器增益
	* @param  mute_on: PGA消声，pga_vol: PGA音量
	* @retval 无
	*/
void WM8978_setPGAGainR(bool mute_on, uint8_t pga_vol)
{
	uint16_t regval = 0x0000;

	if(pga_vol > 63)
		pga_vol = 63;
	regval |= pga_vol;
	bitWrite(regval, INPPGAMUTER, mute_on);
	bitSet(regval, INPPGAZCR);
	bitSet(regval, INPPGAUPDATE);
	WM8978_writeReg(R46_RIGHT_INP_PGA_CONTROL, regval);
}

/**
	* @brief  左BOOST电路配置
	* @param  boost_on: boost电路开关，pga_vol: pga音量， aux_vol: aux音量, l_vol: L2音量
	* @retval 无
	*/
void WM8978_setBoostL(bool boost_on, uint8_t pga_vol, uint8_t aux_vol, uint8_t l_vol)
{
	uint16_t regval;
	
	regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
	bitWrite(regval, BOOSTENL, boost_on);
	WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);

	regval = 0x0000;
	bitWrite(regval, PGABOOSTL, pga_vol);
	if(aux_vol > 7)
		aux_vol = 7;
	regval |= aux_vol;
	if(l_vol > 7)
		l_vol = 7;
	regval |= (l_vol<<4);
	WM8978_writeReg(R47_LEFT_ADC_BOOST_CONTROL, regval);
}

/**
	* @brief  右BOOST电路配置
	* @param  boost_on: boost电路开关，pga_vol: pga音量， aux_vol: aux音量, l_vol: L2音量
	* @retval 无
	*/
void WM8978_setBoostR(bool boost_on, uint8_t pga_vol, uint8_t aux_vol, uint8_t l_vol)
{
	uint16_t regval;
		
	regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
	bitWrite(regval, BOOSTENR, boost_on);
	WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);
	
	regval = 0x0000;
	bitWrite(regval, PGABOOSTR, pga_vol);
	if(aux_vol > 7)
		aux_vol = 7;
	regval |= aux_vol;
	if(l_vol > 7)
		l_vol = 7;
	regval |= (l_vol<<4);
	WM8978_writeReg(R48_RIGHT_ADC_BOOST_CONTROL, regval);
}

/**
	* @brief  麦克风偏置电路配置
	* @param  
	* @retval 无
	*/
void WM8978_setMicBias(bool micb_on, MicBiasV_E mbvsel)
{
	uint16_t regval;
			
	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, MICBEN, micb_on);
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);
		
	regval = WM8978_readReg(R44_INPUT_CONTROL);
	bitWrite(regval, MBVSEL, mbvsel);
	WM8978_writeReg(R44_INPUT_CONTROL, regval);
} 

/**
	* @brief  自动电平控制ALC增益配置
	* @param  alc_on: ALC使能， min_gain: PGA最小增益， max_gain: PGA最大增益
	* @retval 无
	*/
void WM8978_setALCGain(AlcEnable_E alc_on, uint8_t min_gain, uint8_t max_gain)
{
	uint16_t regval = 0x0000;
			
	regval |= (alc_on<<7);
	if(min_gain > 7)
		min_gain = 7;
	regval |= min_gain;
	if(max_gain > 7)
		max_gain = 7;
	regval |= (max_gain<<3);
	WM8978_writeReg(R32_ALC_CONTROL_1, regval);	
}

/**
	* @brief  自动电平控制ALC时间配置
	* @param  alc_mode: ALC模式， alc_lvl: ALC目标， alc_hld: ALC保持时间， alc_dcy: ALC衰减时间， alc_atk: ALC上升时间
	* @retval 无
	*/
void WM8978_setALCMode(AlcMode_E alc_mode, uint8_t alc_lvl, uint8_t alc_hld, uint8_t alc_dcy, uint8_t alc_atk)
{
	uint16_t regval = 0x0000;
				
	if(alc_lvl > 15)
		alc_lvl = 15;
	regval |= alc_lvl;
	if(alc_hld > 15)
		alc_hld = 15;
	regval |= (alc_hld<<4);
	WM8978_writeReg(R33_ALC_CONTROL_2, regval);	

	regval = alc_mode<<8;
	if(alc_dcy > 15)
		alc_dcy = 15;
	regval |= (alc_dcy<<4);
	if(alc_atk > 15)
		alc_atk = 15;
	regval |= alc_atk;
	WM8978_writeReg(R34_ALC_CONTROL_3, regval);	
}

/**
	* @brief  ALC噪声门配置
	* @param  ngate_on: 噪声门使能， ngate_th: 噪声门极限
	* @retval 无
	*/
void WM8978_setALCNoiseGate(bool ngate_on, uint8_t ngate_th)
{
	uint16_t regval = 0x0000;
			
	bitWrite(regval, NGATEN, ngate_on);
	if(ngate_th > 7)
		ngate_th = 7;
	regval |= ngate_th;
	WM8978_writeReg(R35_NOISE_GATE, regval);	
}

/**
	* @brief  EQ3D模式选择
	* @param  eq3d_mode: EQ3D模式
	* @retval 无
	*/
void WM8978_setEQ3DMode(Eq3dMode_E eq3d_mode)
{			
	uint16_t regval;
	
	regval = WM8978_readReg(R18_EQ1);
	bitWrite(regval, EQ3DMODE, eq3d_mode);
	WM8978_writeReg(R18_EQ1, regval);		
}

/**
	* @brief  EQ1滤波器配置
	* @param  eq_gain: eq1增益， eq1c: eq1截止频率
	* @retval 无
	*/
void WM8978_setEQ1(uint8_t eq_gain, Eq1Cutoff_E eq1c)
{			
	uint16_t regval;
	
	regval = WM8978_readReg(R18_EQ1);
	regval &= 0x0180;
	if(eq_gain > 31)
		eq_gain = 31;
	regval |= eq_gain;
	regval |= (eq1c<<5);
	WM8978_writeReg(R18_EQ1, regval);		
}

/**
	* @brief  EQ2滤波器配置
	* @param  eq_gain: eq2增益， eq2c: eq2截止频率, wb_on: 带宽控制
	* @retval 无
	*/
void WM8978_setEQ2(uint8_t eq_gain, Eq2Cutoff_E eq2c, bool wb_on)
{			
	uint16_t regval = 0x0000;
	
	if(eq_gain > 31)
		eq_gain = 31;
	regval |= eq_gain;
	regval |= (eq2c<<5);
	bitWrite(regval, EQ2BW, wb_on);
	WM8978_writeReg(R19_EQ2, regval);		
}

/**
	* @brief  EQ3滤波器配置
	* @param  eq_gain: eq3增益， eq3c: eq3截止频率, wb_on: 带宽控制
	* @retval 无
	*/
void WM8978_setEQ3(uint8_t eq_gain, Eq3Cutoff_E eq3c, bool wb_on)
{			
	uint16_t regval = 0x0000;
	
	if(eq_gain > 31)
		eq_gain = 31;
	regval |= eq_gain;
	regval |= (eq3c<<5);
	bitWrite(regval, EQ3BW, wb_on);
	WM8978_writeReg(R20_EQ3, regval);		
}

/**
	* @brief  EQ4滤波器配置
	* @param  eq_gain: eq4增益， eq4c: eq4截止频率, wb_on: 带宽控制
	* @retval 无
	*/
void WM8978_setEQ4(uint8_t eq_gain, Eq4Cutoff_E eq4c, bool wb_on)
{			
	uint16_t regval = 0x0000;
	
	if(eq_gain > 31)
		eq_gain = 31;
	regval |= eq_gain;
	regval |= (eq4c<<5);
	bitWrite(regval, EQ4BW, wb_on);
	WM8978_writeReg(R21_EQ4, regval);		
}

/**
	* @brief  EQ5滤波器配置
	* @param  eq_gain: eq5增益， eq5c: eq5截止频率
	* @retval 无
	*/
void WM8978_setEQ5(uint8_t eq_gain, Eq5Cutoff_E eq5c)
{			
	uint16_t regval = 0x0000;
	
	if(eq_gain > 31)
		eq_gain = 31;
	regval |= eq_gain;
	regval |= (eq5c<<5);
	WM8978_writeReg(R22_EQ5, regval);		
}

/**
	* @brief  3D放大深度
	* @param  depth: 放大深度
	* @retval 无
	*/
void WM8978_set3DDepth(uint8_t depth)
{			
	uint16_t regval = 0x0000;
	
	if(depth > 15)
		depth = 15;
	regval |= depth;
	WM8978_writeReg(R41_3D_CONTROL, regval);		
}

/**
	* @brief  设置陷波滤波器（notch filter），主要用于抑制话筒声波正反馈，避免啸叫
	* @param  nf_on: 陷波滤波器使能，nf_a0[13:0] and nf_a1[13:0]
	* @retval 无
	*/
void WM8978_setNotchFilter(bool nf_on, uint16_t nf_a0, uint16_t nf_a1)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, NFEN, nf_on);
	regval |= (nf_a0 & 0x3F);
	WM8978_writeReg(R27_NOTCH_FILTER_1, regval);

	regval = 0x0000;
	regval |= ((nf_a0>>7) & 0x3F);
	WM8978_writeReg(R28_NOTCH_FILTER_2, regval);

	regval = 0x0000;
	regval |= (nf_a1 & 0x3F);
	WM8978_writeReg(R29_NOTCH_FILTER_3, regval);

	regval = 0x0000;
	bitSet(regval, NFU);	
	regval |= ((nf_a1>>7) & 0x3F);
	WM8978_writeReg(R30_NOTCH_FILTER_4, regval);
}

/**
	* @brief  DAC使能
	* @param  l_en: 左通道使能， r_en: 右通道使能
	* @retval 无
	*/
void WM8978_setDACEnable(bool l_en, bool r_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R3_POWER_MANAGEMENT_3);
	bitWrite(regval, DACENL, l_en);
	bitWrite(regval, DACENR, r_en);
	WM8978_writeReg(R3_POWER_MANAGEMENT_3, regval);
}

/**
	* @brief  DAC模式配置
	* @param  dac_osr: 过采样率， a_mute: 音频弱声， soft_mute: 软件弱声
	* @retval 无
	*/
void WM8978_setDACMode(OverSampleRate_E dac_osr, bool a_mute, bool soft_mute)
{
	uint16_t regval;
	regval = WM8978_readReg(R10_DAC_CONTROL);
	bitWrite(regval, DACOSR, dac_osr);

	bitWrite(regval, AMUTE, a_mute);
	bitWrite(regval, SOFTMUTE, soft_mute);
	WM8978_writeReg(R10_DAC_CONTROL, regval);
}

/**
	* @brief  DAC音量
	* @param  l_vol: 左通道音量， r_vol: 右通道音量
	* @retval 无
	*/
void WM8978_setDACVolume(uint8_t l_vol, uint8_t r_vol)
{
	uint16_t regval;
	regval = l_vol;
	WM8978_writeReg(R11_LEFT_DAC_DIGITAL_VOLUME, regval);
	regval = r_vol;
	bitSet(regval, DACVU);
	WM8978_writeReg(R12_RIGHT_DAC_DIGITAL_VOLUME, regval);
}

/**
	* @brief  DAC限幅器配置
	* @param  lim_on: 限幅器使能， lim_boost: 限幅器增益， lim_lvl: 限幅器上限， lim_dcy: 限幅器衰减时间， lim_atk: 限幅器上升时间
	* @retval 无
	*/
void WM8978_setDACLimiter(bool lim_on, uint8_t lim_boost, uint8_t lim_lvl, uint8_t lim_dcy, uint8_t lim_atk)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, LIMEN, lim_on);	
	if(lim_atk > 15)
		lim_atk = 15;
	regval |= lim_atk;
	if(lim_dcy > 15)
		lim_dcy = 15;
	regval |= (lim_dcy<<4);
	WM8978_writeReg(R24_DAC_LIMITER_1, regval);

	regval = 0x0000;
	if(lim_boost > 15)
		lim_boost = 15;
	regval |= lim_boost;
	if(lim_lvl > 7)
		lim_lvl = 7;
	regval |= (lim_lvl<<4);
	WM8978_writeReg(R25_DAC_LIMITER_2, regval);
}

/**
	* @brief  OUT1使能
	* @param  l_en: 左通道使能， r_en: 右通道使能
	* @retval 无
	*/
void WM8978_setOUT1Enable(bool l_en, bool r_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R2_POWER_MANAGEMENT_2);
	bitWrite(regval, LOUT1EN, l_en);
	bitWrite(regval, ROUT1EN, r_en);
	WM8978_writeReg(R2_POWER_MANAGEMENT_2, regval);
}

/**
	* @brief  OUT2使能
	* @param  l_en: 左通道使能， r_en: 右通道使能
	* @retval 无
	*/
void WM8978_setOUT2Enable(bool l_en, bool r_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R3_POWER_MANAGEMENT_3);
	bitWrite(regval, LOUT2EN, l_en);
	bitWrite(regval, ROUT2EN, r_en);
	WM8978_writeReg(R3_POWER_MANAGEMENT_3, regval);
}

/**
	* @brief  OUT3使能
	* @param  out_en:
	* @retval 无
	*/
void WM8978_setOUT3Enable(bool out_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R3_POWER_MANAGEMENT_3);
	bitWrite(regval, OUT3EN, out_en);
	WM8978_writeReg(R3_POWER_MANAGEMENT_3, regval);
}

/**
	* @brief  OUT4使能
	* @param  out_en:
	* @retval 无
	*/
void WM8978_setOUT4Enable(bool out_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R3_POWER_MANAGEMENT_3);
	bitWrite(regval, OUT4EN, out_en);
	WM8978_writeReg(R3_POWER_MANAGEMENT_3, regval);
}

/**
	* @brief  MIX使能
	* @param  l_en: 左通道使能， r_en: 右通道使能
	* @retval 无
	*/
void WM8978_setMIXEnable(bool l_en, bool r_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R3_POWER_MANAGEMENT_3);
	bitWrite(regval, LMIXEN, l_en);
	bitWrite(regval, RMIXEN, r_en);
	WM8978_writeReg(R3_POWER_MANAGEMENT_3, regval);
}

/**
	* @brief  左输出混合器配置
	* @param  dac_on: 左DAC选通， byp_on: 左旁路选通， aux_on: 左AUX选通， byp_vol: 左旁路音量， aux_vol: 左AUX音量
	* @retval 无
	*/
void WM8978_setLeftMixer(bool dac_on, bool byp_on, bool aux_on, uint8_t byp_vol, uint8_t aux_vol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, DACL2LMIX, dac_on);		
	bitWrite(regval, BYPL2LMIX, byp_on);
	bitWrite(regval, AUXL2LMIX, aux_on);
	if(byp_vol > 7)
		byp_vol = 7;
	regval |= (byp_vol<<2);
	if(aux_vol > 7)
		aux_vol = 7;
	regval |= (((uint16_t)aux_vol)<<6);
	WM8978_writeReg(R50_LEFT_MIXER_CONTROL, regval);
}

/**
	* @brief  右输出混合器配置
	* @param  dac_on: 右DAC选通， byp_on: 右旁路选通， aux_on: 右AUX选通， byp_vol: 右旁路音量， aux_vol: 右AUX音量
	* @retval 无
	*/
void WM8978_setRightMixer(bool dac_on, bool byp_on, bool aux_on, uint8_t byp_vol, uint8_t aux_vol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, DACR2RMIX, dac_on);		
	bitWrite(regval, BYPR2RMIX, byp_on);
	bitWrite(regval, AUXR2RMIX, aux_on);
	if(byp_vol > 7)
		byp_vol = 7;
	regval |= (byp_vol<<2);
	if(aux_vol > 7)
		aux_vol = 7;
	regval |= (((uint16_t)aux_vol)<<6);
	WM8978_writeReg(R51_RIGHT_MIXER_CONTROL, regval);
}

/**
	* @brief  OUT3MIX使能
	* @param  mix_en: OUT3混合器使能
	* @retval 无
	*/
void WM8978_setOUT3MIXEnable(bool mix_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, OUT3MIXEN, mix_en);
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);
}

/**
	* @brief  OUT3输出混合器配置
	* @param  mute_on: 输出消音， out4_on: out4输出， adc_on: 左adc， mix_on: 左混合器， dac_on: 左dac
	* @retval 无
	*/
void WM8978_setOUT3Mixer(bool mute_on, bool out4_on, bool adc_on, bool mix_on, bool dac_on)
{
	uint16_t regval;

	regval = WM8978_readReg(R56_OUT3_MIXER_CONTROL);
	bitWrite(regval, LDAC2OUT3, dac_on);
	bitWrite(regval, LMIX2OUT3, mix_on);
	bitWrite(regval, BYPL2OUT3, adc_on);
	bitWrite(regval, OUT4_2OUT3, out4_on);
	bitWrite(regval, OUT3MUTE, mute_on);
	WM8978_writeReg(R56_OUT3_MIXER_CONTROL, regval);
}

/**
	* @brief  OUT4MIX使能
	* @param  mix_en: OUT4混合器使能
	* @retval 无
	*/
void WM8978_setOUT4MIXEnable(bool mix_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, OUT4MIXEN, mix_en);
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);
}

/**
	* @brief  OUT4输出混合器配置
	* @param  mute_on: 输出消音， lmix_on: 左混合器输出， ldac_on: 左dac，adc_on: 右adc， rmix_on: 右混合器， rdac_on: 右dac
	* @retval 无
	*/
void WM8978_setOUT4Mixer(bool mute_on, bool lmix_on, bool ldac_on, bool adc_on, bool rmix_on, bool rdac_on)
{
	uint16_t regval;

	regval = WM8978_readReg(R57_OUT4_MIXER_CONTROL);
	bitWrite(regval, RDAC2OUT4, rdac_on);
	bitWrite(regval, RMIX2OUT4, rmix_on);
	bitWrite(regval, BYPL2OUT4, adc_on);
	bitWrite(regval, LDAC2OUT4, ldac_on);
	bitWrite(regval, LMIX2OUT4, lmix_on);
	bitWrite(regval, OUT4MUTE, mute_on);
	WM8978_writeReg(R57_OUT4_MIXER_CONTROL, regval);
}

/**
	* @brief  修改左输出通道1音量
	* @param  mute_on：消声, out_vol：输出音量
	* @retval 无
	*/
void WM8978_setLOUT1Volume(bool mute_on, uint8_t out_vol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, LOUT1MUTE, mute_on);
	if(out_vol > 63)
		out_vol = 63;
	regval |= out_vol;
	bitSet(regval, HPVU);
	WM8978_writeReg(R52_LOUT1_HP_CONTROL, regval);
}

/**
	* @brief  修改右输出通道1音量
	* @param  mute_on：消声, out_vol：输出音量
	* @retval 无
	*/
void WM8978_setROUT1Volume(bool mute_on, uint8_t out_vol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, ROUT1MUTE, mute_on);
	if(out_vol > 63)
		out_vol = 63;
	regval |= out_vol;
	bitSet(regval, HPVU);
	WM8978_writeReg(R53_ROUT1_HP_CONTROL, regval);
}

/**
	* @brief  扬声器增益推动配置
	* @param  boost_on：增益推动
	* @retval 无
	*/
void WM8978_setSpkBoost(bool boost_on)
{
	uint16_t regval;

	regval = WM8978_readReg(R49_OUTPUT_CONTROL);
	bitWrite(regval, SPKBOOST, boost_on);
	WM8978_writeReg(R49_OUTPUT_CONTROL, regval);

	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, BUFDCOPEN, boost_on);
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);
}

/**
	* @brief  OUT3增益推动配置
	* @param  boost_on：增益推动
	* @retval 无
	*/
void WM8978_setOUT3Boost(bool boost_on)
{
	uint16_t regval;

	regval = WM8978_readReg(R49_OUTPUT_CONTROL);
	bitWrite(regval, OUT3BOOST, boost_on);
	WM8978_writeReg(R49_OUTPUT_CONTROL, regval);
}

/**
	* @brief  OUT4增益推动配置
	* @param  boost_on：增益推动
	* @retval 无
	*/
void WM8978_setOUT4Boost(bool boost_on)
{
	uint16_t regval;

	regval = WM8978_readReg(R49_OUTPUT_CONTROL);
	bitWrite(regval, OUT4BOOST, boost_on);
	WM8978_writeReg(R49_OUTPUT_CONTROL, regval);
}

/**
	* @brief  ROUT2反转输出
	* @param  inv_on：反转输出
	* @retval 无
	*/
void WM8978_setInvROUT2(bool inv_on, bool mute_on)
{
	uint16_t regval;

	regval = WM8978_readReg(R43_BEEP_CONTROL);
	bitWrite(regval, INVROUT2, inv_on);
	bitWrite(regval, MUTERPGA2INV, mute_on);
	WM8978_writeReg(R43_BEEP_CONTROL, regval);
}

/**
	* @brief  beep音量设置
	* @param  beep_on：beep使能， beep_vol： beep音量
	* @retval 无
	*/
void WM8978_setBeepVol(bool beep_on, uint8_t beep_vol)
{
	uint16_t regval;

	regval = WM8978_readReg(R43_BEEP_CONTROL);
	bitWrite(regval, BEEPEN, beep_on);
	if(beep_vol > 7)
		beep_vol = 7;
	regval &= 0x01F1;
	regval |= (beep_vol<<1);
	WM8978_writeReg(R43_BEEP_CONTROL, regval);
}

/**
	* @brief  修改左输出通道2音量
	* @param  mute_on：消声, out_vol：输出音量
	* @retval 无
	*/
void WM8978_setLOUT2Volume(bool mute_on, uint8_t out_vol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, LOUT2MUTE, mute_on);
	if(out_vol > 63)
		out_vol = 63;
	regval |= out_vol;
	bitSet(regval, SPKVU);
	WM8978_writeReg(R54_LOUT2_SPK_CONTROL, regval);
}

/**
	* @brief  修改右输出通道2音量
	* @param  mute_on：消声, out_vol：输出音量
	* @retval 无
	*/
void WM8978_setROUT2Volume(bool mute_on, uint8_t out_vol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, ROUT2MUTE, mute_on);
	if(out_vol > 63)
		out_vol = 63;
	regval |= out_vol;
	bitSet(regval, SPKVU);
	WM8978_writeReg(R55_ROUT2_SPK_CONTROL, regval);
}

/**
	* @brief  过热保护使能
	* @param  tsd_en: 过热保护使能
	* @retval 无
	*/
void WM8978_setTSDEnable(bool tsd_en)
{
	uint16_t regval;
	regval = WM8978_readReg(R49_OUTPUT_CONTROL);
	bitWrite(regval, TSDEN, tsd_en);
	WM8978_writeReg(R49_OUTPUT_CONTROL, regval);
}

/**
	* @brief 未用的输入/输出打结使能
	* @param  bufio_en: 打结使能使能, vroi：VREF到模拟输出的阻抗
	* @retval 无
	*/
void WM8978_setBufIO(bool bufio_en, Vroi_E vroi)
{
	uint16_t regval;

	regval = WM8978_readReg(R1_POWER_MANAGEMENT_1);
	bitWrite(regval, BUFIOEN, bufio_en);
	WM8978_writeReg(R1_POWER_MANAGEMENT_1, regval);

	regval = WM8978_readReg(R49_OUTPUT_CONTROL);
	bitWrite(regval, VROI, vroi);
	WM8978_writeReg(R49_OUTPUT_CONTROL, regval);
}

/**
	* @brief  低速时钟使能
	* @param  enable: 使能
	* @retval 无
	*/
void WM8978_setSlowClkEnable(bool enable)
{
	uint16_t regval;
	regval = WM8978_readReg(R7_ADDITIONAL_CONTROL);
	bitWrite(regval, SLOWCLKEN, enable);
	WM8978_writeReg(R7_ADDITIONAL_CONTROL, regval);
}

/**
	* @brief 设置GPIO1引脚功能
	* @param  gpio_sel: GPIO1引脚功能, gpio_pol：GPIO极性反转
	* @retval 无
	*/
void WM8978_setGPIO1(Gpio1Sel_E gpio_sel, bool gpio_pol)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, GPIO1POL, gpio_pol);
	regval |= gpio_sel;
	WM8978_writeReg(R8_GPIO_CONTROL, regval);
}

/**
	* @brief 设置插座检测引脚
	* @param  jd_sel: 插座检测引脚, jd_en：插座检测使能
	* @retval 无
	*/
void WM8978_setJDSel(JDSel_E jd_sel, bool jd_en)
{
	uint16_t regval = 0x0000;

	bitWrite(regval, JD_EN, jd_en);
	regval |= (jd_sel<<4);
	WM8978_writeReg(R9_JACK_DETECT_CONTROL_1, regval);
}

/**
	* @brief 设置插座检测输出使能
	* @param  en_0: 插座检测输入0时使能, en_1：插座检测输入1时使能
	* @retval 无
	*/
void WM8978_setJDen(uint8_t en_0, uint8_t en_1)
{
	uint16_t regval = 0x0000;
	
	en_0 &= 0x0F;
	en_1 &= 0xF0;
	regval |= (en_0|en_1);
	WM8978_writeReg(R13_JACK_DETECT_CONTROL_2, regval);
}
