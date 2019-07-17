################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/netapp/dhcp_thread.c \
../lwip1.4/netapp/http_server.c \
../lwip1.4/netapp/lwipconfig.c \
../lwip1.4/netapp/ping.c \
../lwip1.4/netapp/tcp_client.c \
../lwip1.4/netapp/udp_demo.c 

OBJS += \
./lwip1.4/netapp/dhcp_thread.o \
./lwip1.4/netapp/http_server.o \
./lwip1.4/netapp/lwipconfig.o \
./lwip1.4/netapp/ping.o \
./lwip1.4/netapp/tcp_client.o \
./lwip1.4/netapp/udp_demo.o 

C_DEPS += \
./lwip1.4/netapp/dhcp_thread.d \
./lwip1.4/netapp/http_server.d \
./lwip1.4/netapp/lwipconfig.d \
./lwip1.4/netapp/ping.d \
./lwip1.4/netapp/tcp_client.d \
./lwip1.4/netapp/udp_demo.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/netapp/%.o: ../lwip1.4/netapp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/arch" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/user" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/os_app" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Ports" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Source" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/bsp" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/startup" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/CMSIS" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include/ipv4" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


