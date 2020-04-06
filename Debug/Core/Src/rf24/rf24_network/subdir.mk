################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/rf24/rf24_network/rf24_csma_ca.c \
../Core/Src/rf24/rf24_network/rf24_mac.c \
../Core/Src/rf24/rf24_network/rf24_network.c \
../Core/Src/rf24/rf24_network/rf24_worker.c 

OBJS += \
./Core/Src/rf24/rf24_network/rf24_csma_ca.o \
./Core/Src/rf24/rf24_network/rf24_mac.o \
./Core/Src/rf24/rf24_network/rf24_network.o \
./Core/Src/rf24/rf24_network/rf24_worker.o 

C_DEPS += \
./Core/Src/rf24/rf24_network/rf24_csma_ca.d \
./Core/Src/rf24/rf24_network/rf24_mac.d \
./Core/Src/rf24/rf24_network/rf24_network.d \
./Core/Src/rf24/rf24_network/rf24_worker.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/rf24/rf24_network/rf24_csma_ca.o: ../Core/Src/rf24/rf24_network/rf24_csma_ca.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/rf24_network/rf24_csma_ca.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/rf24/rf24_network/rf24_mac.o: ../Core/Src/rf24/rf24_network/rf24_mac.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/rf24_network/rf24_mac.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/rf24/rf24_network/rf24_network.o: ../Core/Src/rf24/rf24_network/rf24_network.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/rf24_network/rf24_network.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/rf24/rf24_network/rf24_worker.o: ../Core/Src/rf24/rf24_network/rf24_worker.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DSTM32F103xB -DDEBUG -c -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rf24/rf24_network/rf24_worker.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

