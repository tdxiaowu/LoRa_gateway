################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/netif/ppp/auth.c \
../lwip1.4/src/netif/ppp/chap.c \
../lwip1.4/src/netif/ppp/chpms.c \
../lwip1.4/src/netif/ppp/fsm.c \
../lwip1.4/src/netif/ppp/ipcp.c \
../lwip1.4/src/netif/ppp/lcp.c \
../lwip1.4/src/netif/ppp/magic.c \
../lwip1.4/src/netif/ppp/md5.c \
../lwip1.4/src/netif/ppp/pap.c \
../lwip1.4/src/netif/ppp/ppp.c \
../lwip1.4/src/netif/ppp/ppp_oe.c \
../lwip1.4/src/netif/ppp/randm.c \
../lwip1.4/src/netif/ppp/vj.c 

OBJS += \
./lwip1.4/src/netif/ppp/auth.o \
./lwip1.4/src/netif/ppp/chap.o \
./lwip1.4/src/netif/ppp/chpms.o \
./lwip1.4/src/netif/ppp/fsm.o \
./lwip1.4/src/netif/ppp/ipcp.o \
./lwip1.4/src/netif/ppp/lcp.o \
./lwip1.4/src/netif/ppp/magic.o \
./lwip1.4/src/netif/ppp/md5.o \
./lwip1.4/src/netif/ppp/pap.o \
./lwip1.4/src/netif/ppp/ppp.o \
./lwip1.4/src/netif/ppp/ppp_oe.o \
./lwip1.4/src/netif/ppp/randm.o \
./lwip1.4/src/netif/ppp/vj.o 

C_DEPS += \
./lwip1.4/src/netif/ppp/auth.d \
./lwip1.4/src/netif/ppp/chap.d \
./lwip1.4/src/netif/ppp/chpms.d \
./lwip1.4/src/netif/ppp/fsm.d \
./lwip1.4/src/netif/ppp/ipcp.d \
./lwip1.4/src/netif/ppp/lcp.d \
./lwip1.4/src/netif/ppp/magic.d \
./lwip1.4/src/netif/ppp/md5.d \
./lwip1.4/src/netif/ppp/pap.d \
./lwip1.4/src/netif/ppp/ppp.d \
./lwip1.4/src/netif/ppp/ppp_oe.d \
./lwip1.4/src/netif/ppp/randm.d \
./lwip1.4/src/netif/ppp/vj.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/netif/ppp/%.o: ../lwip1.4/src/netif/ppp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


