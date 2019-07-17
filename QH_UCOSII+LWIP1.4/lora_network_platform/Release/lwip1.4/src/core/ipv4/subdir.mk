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
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


