################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include user/subdir.mk
-include ucosII/Source/subdir.mk
-include ucosII/Ports/subdir.mk
-include os_app/subdir.mk
-include newlib/subdir.mk
-include lwip1.4/src/netif/ppp/subdir.mk
-include lwip1.4/src/netif/subdir.mk
-include lwip1.4/src/core/snmp/subdir.mk
-include lwip1.4/src/core/ipv4/subdir.mk
-include lwip1.4/src/core/subdir.mk
-include lwip1.4/src/arch/subdir.mk
-include lwip1.4/src/api/subdir.mk
-include lwip1.4/netapp/subdir.mk
-include libraries/startup/subdir.mk
-include libraries/STM32F10x_StdPeriph_Driver/src/subdir.mk
-include libraries/CMSIS/subdir.mk
-include bsp/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
lora_network_platform.hex \

SECONDARY_SIZE += \
lora_network_platform.siz \


# All Target
all: lora_network_platform.elf secondary-outputs

# Tool invocations
lora_network_platform.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM GNU C Linker'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -T "/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/stm32_flash.ld" -Xlinker --gc-sections -Wl,-Map,"lora_network_platform.map" -o "lora_network_platform.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

lora_network_platform.hex: lora_network_platform.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex "lora_network_platform.elf"  "lora_network_platform.hex"
	@echo 'Finished building: $@'
	@echo ' '

lora_network_platform.siz: lora_network_platform.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "lora_network_platform.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(S_UPPER_DEPS) lora_network_platform.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
