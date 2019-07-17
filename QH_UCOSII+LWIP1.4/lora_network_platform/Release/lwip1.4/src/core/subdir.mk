################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/core/def.c \
../lwip1.4/src/core/dhcp.c \
../lwip1.4/src/core/dns.c \
../lwip1.4/src/core/init.c \
../lwip1.4/src/core/mem.c \
../lwip1.4/src/core/memp.c \
../lwip1.4/src/core/netif.c \
../lwip1.4/src/core/pbuf.c \
../lwip1.4/src/core/raw.c \
../lwip1.4/src/core/stats.c \
../lwip1.4/src/core/sys.c \
../lwip1.4/src/core/tcp.c \
../lwip1.4/src/core/tcp_in.c \
../lwip1.4/src/core/tcp_out.c \
../lwip1.4/src/core/timers.c \
../lwip1.4/src/core/udp.c 

OBJS += \
./lwip1.4/src/core/def.o \
./lwip1.4/src/core/dhcp.o \
./lwip1.4/src/core/dns.o \
./lwip1.4/src/core/init.o \
./lwip1.4/src/core/mem.o \
./lwip1.4/src/core/memp.o \
./lwip1.4/src/core/netif.o \
./lwip1.4/src/core/pbuf.o \
./lwip1.4/src/core/raw.o \
./lwip1.4/src/core/stats.o \
./lwip1.4/src/core/sys.o \
./lwip1.4/src/core/tcp.o \
./lwip1.4/src/core/tcp_in.o \
./lwip1.4/src/core/tcp_out.o \
./lwip1.4/src/core/timers.o \
./lwip1.4/src/core/udp.o 

C_DEPS += \
./lwip1.4/src/core/def.d \
./lwip1.4/src/core/dhcp.d \
./lwip1.4/src/core/dns.d \
./lwip1.4/src/core/init.d \
./lwip1.4/src/core/mem.d \
./lwip1.4/src/core/memp.d \
./lwip1.4/src/core/netif.d \
./lwip1.4/src/core/pbuf.d \
./lwip1.4/src/core/raw.d \
./lwip1.4/src/core/stats.d \
./lwip1.4/src/core/sys.d \
./lwip1.4/src/core/tcp.d \
./lwip1.4/src/core/tcp_in.d \
./lwip1.4/src/core/tcp_out.d \
./lwip1.4/src/core/timers.d \
./lwip1.4/src/core/udp.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/core/%.o: ../lwip1.4/src/core/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


