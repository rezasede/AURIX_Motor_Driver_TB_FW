################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/oneeye_lib/arduino/ifx_oe_al_arduino.c 

C_DEPS += \
./Libraries/oneeye_lib/arduino/ifx_oe_al_arduino.d 

OBJS += \
./Libraries/oneeye_lib/arduino/ifx_oe_al_arduino.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/oneeye_lib/arduino/%.o: ../Libraries/oneeye_lib/arduino/%.c Libraries/oneeye_lib/arduino/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-oneeye_lib-2f-arduino

clean-Libraries-2f-oneeye_lib-2f-arduino:
	-$(RM) ./Libraries/oneeye_lib/arduino/ifx_oe_al_arduino.d ./Libraries/oneeye_lib/arduino/ifx_oe_al_arduino.o

.PHONY: clean-Libraries-2f-oneeye_lib-2f-arduino

