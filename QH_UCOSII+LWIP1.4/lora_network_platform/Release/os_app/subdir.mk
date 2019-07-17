################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../os_app/app_cfg.c \
../os_app/app_task.c 

OBJS += \
./os_app/app_cfg.o \
./os_app/app_task.o 

C_DEPS += \
./os_app/app_cfg.d \
./os_app/app_task.d 


# Each subdirectory must supply rules for building sources it contributes
os_app/%.o: ../os_app/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


