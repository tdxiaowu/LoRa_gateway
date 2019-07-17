################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ucosII/Ports/os_cpu_c.c 

ASM_SRCS += \
../ucosII/Ports/os_cpu_a.asm 

OBJS += \
./ucosII/Ports/os_cpu_a.o \
./ucosII/Ports/os_cpu_c.o 

C_DEPS += \
./ucosII/Ports/os_cpu_c.d 

ASM_DEPS += \
./ucosII/Ports/os_cpu_a.d 


# Each subdirectory must supply rules for building sources it contributes
ucosII/Ports/%.o: ../ucosII/Ports/%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -x assembler-with-cpp -I"/home/zm/workspace/Text Workspace/QH_UCOSII+LWIP1.4/lora_network_platform/lwip1.4/src/arch" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

ucosII/Ports/%.o: ../ucosII/Ports/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


