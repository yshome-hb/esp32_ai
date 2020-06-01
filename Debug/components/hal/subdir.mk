################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../components/hal/hal_eth.c \
../components/hal/hal_i2c.c \
../components/hal/hal_i2s.c 

OBJS += \
./components/hal/hal_eth.o \
./components/hal/hal_i2c.o \
./components/hal/hal_i2s.o 

C_DEPS += \
./components/hal/hal_eth.d \
./components/hal/hal_i2c.d \
./components/hal/hal_i2s.d 


# Each subdirectory must supply rules for building sources it contributes
components/hal/%.o: ../components/hal/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


