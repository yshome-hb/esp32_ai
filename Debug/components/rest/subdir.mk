################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../components/rest/baidu_rest.c 

OBJS += \
./components/rest/baidu_rest.o 

C_DEPS += \
./components/rest/baidu_rest.d 


# Each subdirectory must supply rules for building sources it contributes
components/rest/%.o: ../components/rest/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


