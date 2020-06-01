#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "imuso3.h"
#include "imu.h"
#include "euler.h"

#define TAG "EULER:"


void imu_task(void* pvParameters)
{
    imu.caliFlag = 1;
    IMU_Init();

    while (1) {

    	ReadIMUSensorHandle();
    	if(imu.caliFlag){
    		imu.caliFlag = IMUCalibrate();
    	}else{
    		IMUSO3Update();
    	}
        vTaskDelay(3 / portTICK_RATE_MS);
    }
}


float imu_getYaw()
{
    return imu.yaw;
}


float imu_getPitch()
{
    return imu.pitch;
}


float imu_getRoll()
{
    return imu.roll;
}

