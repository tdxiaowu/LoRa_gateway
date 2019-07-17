################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/api/api_lib.c \
../lwip1.4/src/api/api_msg.c \
../lwip1.4/src/api/err.c \
../lwip1.4/src/api/netbuf.c \
../lwip1.4/src/api/netdb.c \
../lwip1.4/src/api/netifapi.c \
../lwip1.4/src/api/sockets.c \
../lwip1.4/src/api/tcpip.c 

OBJS += \
./lwip1.4/src/api/api_lib.o \
./lwip1.4/src/api/api_msg.o \
./lwip1.4/src/api/err.o \
./lwip1.4/src/api/netbuf.o \
./lwip1.4/src/api/netdb.o \
./lwip1.4/src/api/netifapi.o \
./lwip1.4/src/api/sockets.o \
./lwip1.4/src/api/tcpip.o 

C_DEPS += \
./lwip1.4/src/api/api_lib.d \
./lwip1.4/src/api/api_msg.d \
./lwip1.4/src/api/err.d \
./lwip1.4/src/api/netbuf.d \
./lwip1.4/src/api/netdb.d \
./lwip1.4/src/api/netifapi.d \
./lwip1.4/src/api/sockets.d \
./lwip1.4/src/api/tcpip.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/api/%.o: ../lwip1.4/src/api/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/arch" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/user" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/os_app" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Ports" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Source" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/bsp" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/startup" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/CMSIS" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include/ipv4" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


