################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UserLibs/Peripherals/ASCLIN_UART.c \
../UserLibs/Peripherals/Adc_MotorSense.c \
../UserLibs/Peripherals/Adc_MotorSense_Isr.c \
../UserLibs/Peripherals/EGTM_ATOM_3_Phase_Inverter_PWM.c \
../UserLibs/Peripherals/Eth_Core.c \
../UserLibs/Peripherals/EthernetLib.c \
../UserLibs/Peripherals/GPIO_LED_Button.c \
../UserLibs/Peripherals/encoder_calib.c \
../UserLibs/Peripherals/encoder_egtm.c 

C_DEPS += \
./UserLibs/Peripherals/ASCLIN_UART.d \
./UserLibs/Peripherals/Adc_MotorSense.d \
./UserLibs/Peripherals/Adc_MotorSense_Isr.d \
./UserLibs/Peripherals/EGTM_ATOM_3_Phase_Inverter_PWM.d \
./UserLibs/Peripherals/Eth_Core.d \
./UserLibs/Peripherals/EthernetLib.d \
./UserLibs/Peripherals/GPIO_LED_Button.d \
./UserLibs/Peripherals/encoder_calib.d \
./UserLibs/Peripherals/encoder_egtm.d 

OBJS += \
./UserLibs/Peripherals/ASCLIN_UART.o \
./UserLibs/Peripherals/Adc_MotorSense.o \
./UserLibs/Peripherals/Adc_MotorSense_Isr.o \
./UserLibs/Peripherals/EGTM_ATOM_3_Phase_Inverter_PWM.o \
./UserLibs/Peripherals/Eth_Core.o \
./UserLibs/Peripherals/EthernetLib.o \
./UserLibs/Peripherals/GPIO_LED_Button.o \
./UserLibs/Peripherals/encoder_calib.o \
./UserLibs/Peripherals/encoder_egtm.o 


# Each subdirectory must supply rules for building sources it contributes
UserLibs/Peripherals/%.o: ../UserLibs/Peripherals/%.c UserLibs/Peripherals/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-UserLibs-2f-Peripherals

clean-UserLibs-2f-Peripherals:
	-$(RM) ./UserLibs/Peripherals/ASCLIN_UART.d ./UserLibs/Peripherals/ASCLIN_UART.o ./UserLibs/Peripherals/Adc_MotorSense.d ./UserLibs/Peripherals/Adc_MotorSense.o ./UserLibs/Peripherals/Adc_MotorSense_Isr.d ./UserLibs/Peripherals/Adc_MotorSense_Isr.o ./UserLibs/Peripherals/EGTM_ATOM_3_Phase_Inverter_PWM.d ./UserLibs/Peripherals/EGTM_ATOM_3_Phase_Inverter_PWM.o ./UserLibs/Peripherals/Eth_Core.d ./UserLibs/Peripherals/Eth_Core.o ./UserLibs/Peripherals/EthernetLib.d ./UserLibs/Peripherals/EthernetLib.o ./UserLibs/Peripherals/GPIO_LED_Button.d ./UserLibs/Peripherals/GPIO_LED_Button.o ./UserLibs/Peripherals/encoder_calib.d ./UserLibs/Peripherals/encoder_calib.o ./UserLibs/Peripherals/encoder_egtm.d ./UserLibs/Peripherals/encoder_egtm.o

.PHONY: clean-UserLibs-2f-Peripherals

