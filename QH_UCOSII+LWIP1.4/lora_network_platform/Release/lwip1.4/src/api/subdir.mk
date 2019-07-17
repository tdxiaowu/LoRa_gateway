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
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


