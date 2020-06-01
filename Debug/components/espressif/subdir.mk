################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../components/espressif/eth.c \
../components/espressif/event.c \
../components/espressif/wifi.c 

OBJS += \
./components/espressif/eth.o \
./components/espressif/event.o \
./components/espressif/wifi.o 

C_DEPS += \
./components/espressif/eth.d \
./components/espressif/event.d \
./components/espressif/wifi.d 


# Each subdirectory must supply rules for building sources it contributes
components/espressif/%.o: ../components/espressif/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


