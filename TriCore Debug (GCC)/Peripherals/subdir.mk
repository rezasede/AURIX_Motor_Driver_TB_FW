################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripherals/GPIO_LED_Button.c 

C_DEPS += \
./Peripherals/GPIO_LED_Button.d 

OBJS += \
./Peripherals/GPIO_LED_Button.o 


# Each subdirectory must supply rules for building sources it contributes
Peripherals/%.o: ../Peripherals/%.c Peripherals/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Peripherals

clean-Peripherals:
	-$(RM) ./Peripherals/GPIO_LED_Button.d ./Peripherals/GPIO_LED_Button.o

.PHONY: clean-Peripherals

