#include <math.h>
#include "imu.h"
#include "filter.h"
#include "mpu6050.h"

imu_t imu = {0};
// ---
void IMU_Init(void)
{
	MPU6050_initialize();
	//filter rate
	LPF2pSetCutoffFreq_1(IMU_SAMPLE_RATE,IMU_FILTER_CUTOFF_FREQ);   //30Hz
	LPF2pSetCutoffFreq_2(IMU_SAMPLE_RATE,IMU_FILTER_CUTOFF_FREQ);
	LPF2pSetCutoffFreq_3(IMU_SAMPLE_RATE,IMU_FILTER_CUTOFF_FREQ);
	LPF2pSetCutoffFreq_4(IMU_SAMPLE_RATE,IMU_FILTER_CUTOFF_FREQ);
	LPF2pSetCutoffFreq_5(IMU_SAMPLE_RATE,IMU_FILTER_CUTOFF_FREQ);
	LPF2pSetCutoffFreq_6(IMU_SAMPLE_RATE,IMU_FILTER_CUTOFF_FREQ);

}


//should place to a level surface and keep it stop for 1~2 second
//return 1 when finish

uint8_t IMUCalibrate(void)
{ 
  static float accOffsetSum[3]={ 0.0f, 0.0f, 0.0f };// gyro_offsets[3] = { 0.0f, 0.0f, 0.0f },
  static float gyroOffsetsSum[3]={ 0.0f, 0.0f, 0.0f };// gyro_offsets[3] = { 0.0f, 0.0f, 0.0f },
  static uint16_t offsetCount = 0;  
        
  accOffsetSum[0] += imu.accRaw[0];
  accOffsetSum[1] += imu.accRaw[1];
  accOffsetSum[2] += imu.accRaw[2];
  gyroOffsetsSum[0] += imu.gyroRaw[0];
  gyroOffsetsSum[1] += imu.gyroRaw[1];
  gyroOffsetsSum[2] += imu.gyroRaw[2];
  offsetCount++;
      
  if(offsetCount > GYRO_CALC_COUNT - 1)
  {
    imu.accOffset[0] = accOffsetSum[0]/offsetCount;
    imu.accOffset[1] = accOffsetSum[1]/offsetCount;
    imu.accOffset[2] = accOffsetSum[2]/offsetCount;
    imu.gyroOffset[0] = gyroOffsetsSum[0]/offsetCount;
    imu.gyroOffset[1] = gyroOffsetsSum[1]/offsetCount;
    imu.gyroOffset[2] = gyroOffsetsSum[2]/offsetCount;
          
    accOffsetSum[0] = 0;
    accOffsetSum[1] = 0;
    accOffsetSum[2] = 0;
    gyroOffsetsSum[0] = 0;
    gyroOffsetsSum[1] = 0;
    gyroOffsetsSum[2] = 0;          
    offsetCount = 0;
    
    imu.accOffset[2] = imu.accOffset[2] - CONSTANTS_ONE_G;
    imu.caliPass= (fabs(imu.accOffset[0]) < ACC_OFFSET_MAX) && (fabs(imu.accOffset[1]) < ACC_OFFSET_MAX);     
    return 0;
  }
  return 1;
}


void ReadIMUSensorHandle(void)
{
  //read raw
  MPU6050_getMotion6(imu.accADC, imu.accADC+1, imu.accADC+2, imu.gyroADC, imu.gyroADC+1, imu.gyroADC+2);
  //tutn to physical

  imu.accRaw[0] =  (float)imu.accADC[0] *ACC_SCALE * CONSTANTS_ONE_G ;
  imu.gyroRaw[0] = (float)imu.gyroADC[0] * GYRO_SCALE * RAD_PI_REC;   //deg/s

  imu.accRaw[1] =  (float)imu.accADC[1] *ACC_SCALE * CONSTANTS_ONE_G ;
  imu.gyroRaw[1] = (float)imu.gyroADC[1] * GYRO_SCALE * RAD_PI_REC;   //deg/s

  imu.accRaw[2] =  (float)imu.accADC[2] *ACC_SCALE * CONSTANTS_ONE_G ;
  imu.gyroRaw[2] = (float)imu.gyroADC[2] * GYRO_SCALE * RAD_PI_REC;   //deg/s
  
  imu.accb[0] = LPF2pApply_1(imu.accRaw[0] - imu.accOffset[0]);
  imu.accb[1] = LPF2pApply_2(imu.accRaw[1] - imu.accOffset[1]);
  imu.accb[2] = LPF2pApply_3(imu.accRaw[2] - imu.accOffset[2]);
  imu.gyro[0] = LPF2pApply_4(imu.gyroRaw[0] - imu.gyroOffset[0]);
  imu.gyro[1] = LPF2pApply_5(imu.gyroRaw[1] - imu.gyroOffset[1]);
  imu.gyro[2] = LPF2pApply_6(imu.gyroRaw[2] - imu.gyroOffset[2]);
} 


uint8_t IMUCheck(void)
{
  float accZb = imu.accRaw[2] - imu.accOffset[2] - CONSTANTS_ONE_G;

  imu.caliPass = (fabs(accZb) < ACCZ_ERR_MAX);      
  return imu.caliPass;    
}


