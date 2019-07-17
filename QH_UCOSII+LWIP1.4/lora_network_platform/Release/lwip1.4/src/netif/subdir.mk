################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/netif/etharp.c \
../lwip1.4/src/netif/ethernetif.c \
../lwip1.4/src/netif/loopif.c \
../lwip1.4/src/netif/slipif.c 

OBJS += \
./lwip1.4/src/netif/etharp.o \
./lwip1.4/src/netif/ethernetif.o \
./lwip1.4/src/netif/loopif.o \
./lwip1.4/src/netif/slipif.o 

C_DEPS += \
./lwip1.4/src/netif/etharp.d \
./lwip1.4/src/netif/ethernetif.d \
./lwip1.4/src/netif/loopif.d \
./lwip1.4/src/netif/slipif.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/netif/%.o: ../lwip1.4/src/netif/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


