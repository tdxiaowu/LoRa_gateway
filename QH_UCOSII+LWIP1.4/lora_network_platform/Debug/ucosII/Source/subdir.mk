################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ucosII/Source/os_core.c \
../ucosII/Source/os_flag.c \
../ucosII/Source/os_mbox.c \
../ucosII/Source/os_mem.c \
../ucosII/Source/os_mutex.c \
../ucosII/Source/os_q.c \
../ucosII/Source/os_sem.c \
../ucosII/Source/os_task.c \
../ucosII/Source/os_time.c \
../ucosII/Source/os_tmr.c 

OBJS += \
./ucosII/Source/os_core.o \
./ucosII/Source/os_flag.o \
./ucosII/Source/os_mbox.o \
./ucosII/Source/os_mem.o \
./ucosII/Source/os_mutex.o \
./ucosII/Source/os_q.o \
./ucosII/Source/os_sem.o \
./ucosII/Source/os_task.o \
./ucosII/Source/os_time.o \
./ucosII/Source/os_tmr.o 

C_DEPS += \
./ucosII/Source/os_core.d \
./ucosII/Source/os_flag.d \
./ucosII/Source/os_mbox.d \
./ucosII/Source/os_mem.d \
./ucosII/Source/os_mutex.d \
./ucosII/Source/os_q.d \
./ucosII/Source/os_sem.d \
./ucosII/Source/os_task.d \
./ucosII/Source/os_time.d \
./ucosII/Source/os_tmr.d 


# Each subdirectory must supply rules for building sources it contributes
ucosII/Source/%.o: ../ucosII/Source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/arch" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/user" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/os_app" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Ports" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/ucosII/Source" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/bsp" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/startup" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/libraries/CMSIS" -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/include/ipv4" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


