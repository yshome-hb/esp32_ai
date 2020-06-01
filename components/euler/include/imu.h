#ifndef __IMU_H__
#define __IMU_H__

#include "stdint.h"

#define IMU_SAMPLE_RATE           250.0f
#define IMU_FILTER_CUTOFF_FREQ    30.0f

//校准时间
#define GYRO_CALC_COUNT   	500  //ms
#define ACC_OFFSET_MAX    	0.45
#define ACCZ_ERR_MAX      	0.05    //m/s^2

#define RAD_PI          	57.29578f   //180/PI_F
#define RAD_PI_REC      	0.01745329252f  //PI_F/180
#define CONSTANTS_ONE_G 	9.80665f    /* m/s^2    */
#define SENSOR_MAX_G    	4.0f        //constant g
#define SENSOR_MAX_W    	2000.0f   //deg/s
#define ACC_SCALE       	(SENSOR_MAX_G/32768.0f)
#define GYRO_SCALE      	(SENSOR_MAX_W/32768.0f)

enum{X=0,Y,Z};

typedef struct{
  uint8_t caliFlag;
  uint8_t caliPass;
  int16_t accADC[3];
  int16_t gyroADC[3];
  float  accRaw[3];   //m/s^2
  float  gyroRaw[3];    //rad/s   //
  float  accb[3];   //filted, in body frame
  float  gyro[3];
  float  accOffset[3];    //m/s^2
  float  gyroOffset[3];
  float  DCMgb[3][3];
  float  roll;        //deg
  float  pitch;
  float  yaw;
}imu_t;

extern imu_t imu;

void IMU_Init(void);
uint8_t IMUCalibrate(void);
void ReadIMUSensorHandle(void);
uint8_t IMUCheck(void);

#endif

