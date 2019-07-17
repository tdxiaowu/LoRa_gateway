################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/arch/enc28j60.c \
../lwip1.4/src/arch/spi.c \
../lwip1.4/src/arch/sys_arch.c 

OBJS += \
./lwip1.4/src/arch/enc28j60.o \
./lwip1.4/src/arch/spi.o \
./lwip1.4/src/arch/sys_arch.o 

C_DEPS += \
./lwip1.4/src/arch/enc28j60.d \
./lwip1.4/src/arch/spi.d \
./lwip1.4/src/arch/sys_arch.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/arch/%.o: ../lwip1.4/src/arch/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


