################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/core/ipv4/autoip.c \
../lwip1.4/src/core/ipv4/icmp.c \
../lwip1.4/src/core/ipv4/igmp.c \
../lwip1.4/src/core/ipv4/inet.c \
../lwip1.4/src/core/ipv4/inet_chksum.c \
../lwip1.4/src/core/ipv4/ip.c \
../lwip1.4/src/core/ipv4/ip_addr.c \
../lwip1.4/src/core/ipv4/ip_frag.c 

OBJS += \
./lwip1.4/src/core/ipv4/autoip.o \
./lwip1.4/src/core/ipv4/icmp.o \
./lwip1.4/src/core/ipv4/igmp.o \
./lwip1.4/src/core/ipv4/inet.o \
./lwip1.4/src/core/ipv4/inet_chksum.o \
./lwip1.4/src/core/ipv4/ip.o \
./lwip1.4/src/core/ipv4/ip_addr.o \
./lwip1.4/src/core/ipv4/ip_frag.o 

C_DEPS += \
./lwip1.4/src/core/ipv4/autoip.d \
./lwip1.4/src/core/ipv4/icmp.d \
./lwip1.4/src/core/ipv4/igmp.d \
./lwip1.4/src/core/ipv4/inet.d \
./lwip1.4/src/core/ipv4/inet_chksum.d \
./lwip1.4/src/core/ipv4/ip.d \
./lwip1.4/src/core/ipv4/ip_addr.d \
./lwip1.4/src/core/ipv4/ip_frag.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/core/ipv4/%.o: ../lwip1.4/src/core/ipv4/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/arch" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/user" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/os_app" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Ports" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Source" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/bsp" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/startup" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/CMSIS" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include/ipv4" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


