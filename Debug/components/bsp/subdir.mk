################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../components/bsp/wm8978.c 

OBJS += \
./components/bsp/wm8978.o 

C_DEPS += \
./components/bsp/wm8978.d 


# Each subdirectory must supply rules for building sources it contributes
components/bsp/%.o: ../components/bsp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


