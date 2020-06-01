#include <math.h>
#include "imu.h"
#include "imuso3.h"
#include "esp32delay.h"

#define so3_comp_params_Kp   1.0f
#define so3_comp_params_Ki   0.05f

//! Auxiliary variables to reduce number of repeated operations
static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;  /** quaternion of sensor frame relative to auxiliary frame */
static float dq0 = 0.0f, dq1 = 0.0f, dq2 = 0.0f, dq3 = 0.0f;  /** quaternion of sensor frame relative to auxiliary frame */
static float gyro_bias[3] = {0.0f, 0.0f, 0.0f}; /** bias estimation */
static float q0q0, q0q1, q0q2, q0q3;
static float q1q1, q1q2, q1q3;
static float q2q2, q2q3;
static float q3q3;
//---------------------------------------------------------------------------------------------------
 
float invSqrt(float x)
{
	float halfx = x * 0.5f;
	float y = x;
	long i = *(long *)&y;
	i = 0x5f3759df - (i >> 1);
	y = * ((float *)&i);
	y = y * (1.5f - (halfx * y * y ));
	return y;
}

//! Using accelerometer, sense the gravity vector.
//! Using magnetometer, sense yaw.
void NonlinearSO3AHRSinit(float ax, float ay, float az)
{
  float initialRoll, initialPitch;
  float cosRoll, sinRoll, cosPitch, sinPitch;

  initialRoll = atan2(-ay, -az);
  initialPitch = atan2(ax, -az);

  cosRoll = cosf(initialRoll);
  sinRoll = sinf(initialRoll);
  cosPitch = cosf(initialPitch);
  sinPitch = sinf(initialPitch);

  cosRoll = cosf(initialRoll * 0.5f);
  sinRoll = sinf(initialRoll * 0.5f);

  cosPitch = cosf(initialPitch * 0.5f);
  sinPitch = sinf(initialPitch * 0.5f);

  q0 = cosRoll * cosPitch;
  q1 = sinRoll * cosPitch;
  q2 = cosRoll * sinPitch;
  q3 = -sinRoll * sinPitch;

  // auxillary variables to reduce number of repeated operations, for 1st pass
  q0q0 = q0 * q0;
  q0q1 = q0 * q1;
  q0q2 = q0 * q2;
  q0q3 = q0 * q3;
  q1q1 = q1 * q1;
  q1q2 = q1 * q2;
  q1q3 = q1 * q3;
  q2q2 = q2 * q2;
  q2q3 = q2 * q3;
  q3q3 = q3 * q3;
}

void NonlinearSO3AHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float twoKp, float twoKi, float dt)
{
  static uint8_t bFilterInit = 0;
  float recipNorm;
  float halfex = 0.0f, halfey = 0.0f, halfez = 0.0f;

  // Make filter converge to initial solution faster
  // This function assumes you are in static position.
  // WARNING : in case air reboot, this can cause problem. But this is very unlikely happen.
  if(bFilterInit == 0) {
    NonlinearSO3AHRSinit(ax,ay,az);
    bFilterInit = 1;
  }

  //澧炲姞涓�涓潯浠讹細  鍔犻�熷害鐨勬ā閲忎笌G鐩稿樊涓嶈繙鏃躲�� 0.75*G < normAcc < 1.25*G
  // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
  if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))   
  {
    float halfvx, halfvy, halfvz;
//    float accNorm=0;
  
    // Normalise accelerometer measurement
    recipNorm = invSqrt(ax * ax + ay * ay + az * az);
    //--added!!!
//    accNorm=1.0f/recipNorm;
//    if(accNorm > 0.75 * CONSTANTS_ONE_G && accNorm < 1.25 * CONSTANTS_ONE_G )  //鍔犻�熷害杩囧ぇ鏃�
//    {
      ax *= recipNorm;
      ay *= recipNorm;
      az *= recipNorm;

      // Estimated direction of gravity and magnetic field
      halfvx = q1q3 - q0q2;
      halfvy = q0q1 + q2q3;
      halfvz = q0q0 - 0.5f + q3q3;
    
      // Error is sum of cross product between estimated direction and measured direction of field vectors
      halfex += ay * halfvz - az * halfvy;
      halfey += az * halfvx - ax * halfvz;
      halfez += ax * halfvy - ay * halfvx;
//    }
  }

  // Apply feedback only when valid data has been gathered from the accelerometer or magnetometer
  if(halfex != 0.0f && halfey != 0.0f && halfez != 0.0f) {
    // Compute and apply integral feedback if enabled
    if(twoKi > 0.0f) {
      gyro_bias[0] += twoKi * halfex * dt;  // integral error scaled by Ki
      gyro_bias[1] += twoKi * halfey * dt;
      gyro_bias[2] += twoKi * halfez * dt;
      
      // apply integral feedback
      gx += gyro_bias[0];
      gy += gyro_bias[1];
      gz += gyro_bias[2];
    }
    else {
      gyro_bias[0] = 0.0f;  // prevent integral windup
      gyro_bias[1] = 0.0f;
      gyro_bias[2] = 0.0f;
    }

    // Apply proportional feedback
    gx += twoKp * halfex;
    gy += twoKp * halfey;
    gz += twoKp * halfez;
  }

  // Time derivative of quaternion. q_dot = 0.5*q\otimes omega.
  //! q_k = q_{k-1} + dt*\dot{q}
  //! \dot{q} = 0.5*q \otimes P(\omega)
  dq0 = 0.5f*(-q1 * gx - q2 * gy - q3 * gz);
  dq1 = 0.5f*(q0 * gx + q2 * gz - q3 * gy);
  dq2 = 0.5f*(q0 * gy - q1 * gz + q3 * gx);
  dq3 = 0.5f*(q0 * gz + q1 * gy - q2 * gx); 

  q0 += dt*dq0;
  q1 += dt*dq1;
  q2 += dt*dq2;
  q3 += dt*dq3;
  
  // Normalise quaternion
  recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
  q0 *= recipNorm;
  q1 *= recipNorm;
  q2 *= recipNorm;
  q3 *= recipNorm;

  // Auxiliary variables to avoid repeated arithmetic
  q0q0 = q0 * q0;
  q0q1 = q0 * q1;
  q0q2 = q0 * q2;
  q0q3 = q0 * q3;
  q1q1 = q1 * q1;
  q1q2 = q1 * q2;
  q1q3 = q1 * q3;
  q2q2 = q2 * q2;
  q2q3 = q2 * q3;
  q3q3 = q3 * q3;   
}

 

void IMUSO3Update(void)
{
  //! Time constant
  static uint32_t tPrev=0;  //us
  uint32_t now;
  float dt = 0.01f;   //s
  
  /* output euler angles */
  float euler[3] = {0.0f, 0.0f, 0.0f};  //rad 
  /* Initialization */
  float Rot_matrix[9] = {1.f,  0.0f,  0.0f, 0.0f,  1.f,  0.0f, 0.0f,  0.0f,  1.f };   /**< init: identity matrix */

  now = micros();
  dt = (tPrev>0)?(float)(now-tPrev)*0.000001f:0;
  tPrev = now;

  // NOTE : Accelerometer is reversed.
  // Because proper mount of PX4 will give you a reversed accelerometer readings.
  NonlinearSO3AHRSupdate(imu.gyro[0], imu.gyro[1], imu.gyro[2],
              imu.accb[0], imu.accb[1], imu.accb[2],
              so3_comp_params_Kp,
              so3_comp_params_Ki, 
              dt);

    // Convert q->R, This R converts inertial frame to body frame.
  Rot_matrix[0] = q0q0 + q1q1 - q2q2 - q3q3;// 11
  Rot_matrix[1] = 2.f * (q1*q2 + q0*q3);  // 12
  Rot_matrix[2] = 2.f * (q1*q3 - q0*q2);  // 13
  Rot_matrix[3] = 2.f * (q1*q2 - q0*q3);  // 21
  Rot_matrix[4] = q0q0 - q1q1 + q2q2 - q3q3;// 22
  Rot_matrix[5] = 2.f * (q2*q3 + q0*q1);  // 23
  Rot_matrix[6] = 2.f * (q1*q3 + q0*q2);  // 31
  Rot_matrix[7] = 2.f * (q2*q3 - q0*q1);  // 32
  Rot_matrix[8] = q0q0 - q1q1 - q2q2 + q3q3;// 33

  //1-2-3 Representation.
  //Equation (290) 
  //Representing Attitude: Euler Angles, Unit Quaternions, and Rotation Vectors, James Diebel.
  // Existing PX4 EKF code was generated by MATLAB which uses coloum major order matrix.
  euler[0] = atan2f(Rot_matrix[5], Rot_matrix[8]);  //! Roll
  euler[1] = -asinf(Rot_matrix[2]);                 //! Pitch
  euler[2] = atan2f(Rot_matrix[1], Rot_matrix[0]);   
    
  //DCM . ground to body
  for(uint8_t i=0;i<9;i++)
  {
    *(&(imu.DCMgb[0][0]) + i)=Rot_matrix[i];
  }
    
  imu.roll=euler[0] * RAD_PI;
  imu.pitch=euler[1] * RAD_PI;
  imu.yaw=euler[2] * RAD_PI;   
} 

