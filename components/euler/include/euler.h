#ifndef EULER_H
#define EULER_H


void imu_task(void* pvParameters);
float imu_getYaw();
float imu_getPitch();
float imu_getRoll();


#endif