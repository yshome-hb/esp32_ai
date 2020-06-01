################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../components/http/http.c \
../components/http/url_parser.c 

OBJS += \
./components/http/http.o \
./components/http/url_parser.o 

C_DEPS += \
./components/http/http.d \
./components/http/url_parser.d 


# Each subdirectory must supply rules for building sources it contributes
components/http/%.o: ../components/http/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


