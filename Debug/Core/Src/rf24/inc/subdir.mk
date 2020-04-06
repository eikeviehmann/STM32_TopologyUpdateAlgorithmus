################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/rf24/inc/stm32f10x_md5.c 

OBJS += \
./Core/Src/rf24/inc/stm32f10x_md5.o 

C_DEPS += \
./Core/Src/rf24/inc/stm32f10x_md5.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/rf24/inc/stm32f10x_md5.o: ../Core/Src/rf24/inc/stm32f10x_md5.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/inc/stm32f10x_md5.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

