################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/rf24/rf24.c \
../Core/Src/rf24/rf24_debug.c 

OBJS += \
./Core/Src/rf24/rf24.o \
./Core/Src/rf24/rf24_debug.o 

C_DEPS += \
./Core/Src/rf24/rf24.d \
./Core/Src/rf24/rf24_debug.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/rf24/rf24.o: ../Core/Src/rf24/rf24.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/rf24.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/rf24/rf24_debug.o: ../Core/Src/rf24/rf24_debug.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/rf24_debug.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

