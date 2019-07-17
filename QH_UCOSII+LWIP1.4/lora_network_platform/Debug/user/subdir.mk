################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../user/main.c \
../user/stm32f10x_it.c \
../user/system_stm32f10x.c 

OBJS += \
./user/main.o \
./user/stm32f10x_it.o \
./user/system_stm32f10x.o 

C_DEPS += \
./user/main.d \
./user/stm32f10x_it.d \
./user/system_stm32f10x.d 


# Each subdirectory must supply rules for building sources it contributes
user/%.o: ../user/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/arch" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/user" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/os_app" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Ports" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Source" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/bsp" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/startup" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/CMSIS" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include/ipv4" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


