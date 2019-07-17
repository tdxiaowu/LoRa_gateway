################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip1.4/src/core/snmp/asn1_dec.c \
../lwip1.4/src/core/snmp/asn1_enc.c \
../lwip1.4/src/core/snmp/mib2.c \
../lwip1.4/src/core/snmp/mib_structs.c \
../lwip1.4/src/core/snmp/msg_in.c \
../lwip1.4/src/core/snmp/msg_out.c 

OBJS += \
./lwip1.4/src/core/snmp/asn1_dec.o \
./lwip1.4/src/core/snmp/asn1_enc.o \
./lwip1.4/src/core/snmp/mib2.o \
./lwip1.4/src/core/snmp/mib_structs.o \
./lwip1.4/src/core/snmp/msg_in.o \
./lwip1.4/src/core/snmp/msg_out.o 

C_DEPS += \
./lwip1.4/src/core/snmp/asn1_dec.d \
./lwip1.4/src/core/snmp/asn1_enc.d \
./lwip1.4/src/core/snmp/mib2.d \
./lwip1.4/src/core/snmp/mib_structs.d \
./lwip1.4/src/core/snmp/msg_in.d \
./lwip1.4/src/core/snmp/msg_out.d 


# Each subdirectory must supply rules for building sources it contributes
lwip1.4/src/core/snmp/%.o: ../lwip1.4/src/core/snmp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


