################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Core/Startup/subdir.mk
-include Core/Src/rf24/rf24_urci/subdir.mk
-include Core/Src/rf24/rf24_stm32f1xx/subdir.mk
-include Core/Src/rf24/rf24_network/subdir.mk
-include Core/Src/rf24/rf24_module/subdir.mk
-include Core/Src/rf24/inc/subdir.mk
-include Core/Src/rf24/subdir.mk
-include Core/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
STM32_TopologyUpdateAlgorithm.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
STM32_TopologyUpdateAlgorithm.list \

OBJCOPY_HEX += \
STM32_TopologyUpdateAlgorithm.hex \

OBJCOPY_BIN += \
STM32_TopologyUpdateAlgorithm.bin \


# All Target
all: STM32_TopologyUpdateAlgorithm.elf secondary-outputs

# Tool invocations
STM32_TopologyUpdateAlgorithm.elf: $(OBJS) $(USER_OBJS) C:\Projekte\TopologyUpdateAlgorithmus\STM32_ToplogyUpdateAlgorithmus\STM32F103C8TX_FLASH.ld
	arm-none-eabi-gcc -o "STM32_TopologyUpdateAlgorithm.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m3 -T"C:\Projekte\TopologyUpdateAlgorithmus\STM32_ToplogyUpdateAlgorithmus\STM32F103C8TX_FLASH.ld" --specs=nosys.specs -Wl,-Map="STM32_TopologyUpdateAlgorithm.map" -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

STM32_TopologyUpdateAlgorithm.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "STM32_TopologyUpdateAlgorithm.list"
	@echo 'Finished building: $@'
	@echo ' '

STM32_TopologyUpdateAlgorithm.hex: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "STM32_TopologyUpdateAlgorithm.hex"
	@echo 'Finished building: $@'
	@echo ' '

STM32_TopologyUpdateAlgorithm.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "STM32_TopologyUpdateAlgorithm.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
