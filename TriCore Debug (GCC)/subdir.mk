################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ASCLIN_UART.c \
../Adc_MotorSense.c \
../Adc_MotorSense_Isr.c \
../Cpu0_Main.c \
../Cpu1_Main.c \
../Cpu2_Main.c \
../Cpu3_Main.c \
../Cpu4_Main.c \
../Cpu5_Main.c \
../EGTM_ATOM_3_Phase_Inverter_PWM.c \
../Eth_Core.c \
../Eth_OneEye.c \
../GPIO_LED_Button.c \
../UserFunctions.c 

LSL_SRCS += \
../Lcf_Gcc_Tricore_Tc.lsl 

C_DEPS += \
./ASCLIN_UART.d \
./Adc_MotorSense.d \
./Adc_MotorSense_Isr.d \
./Cpu0_Main.d \
./Cpu1_Main.d \
./Cpu2_Main.d \
./Cpu3_Main.d \
./Cpu4_Main.d \
./Cpu5_Main.d \
./EGTM_ATOM_3_Phase_Inverter_PWM.d \
./Eth_Core.d \
./Eth_OneEye.d \
./GPIO_LED_Button.d \
./UserFunctions.d 

OBJS += \
./ASCLIN_UART.o \
./Adc_MotorSense.o \
./Adc_MotorSense_Isr.o \
./Cpu0_Main.o \
./Cpu1_Main.o \
./Cpu2_Main.o \
./Cpu3_Main.o \
./Cpu4_Main.o \
./Cpu5_Main.o \
./EGTM_ATOM_3_Phase_Inverter_PWM.o \
./Eth_Core.o \
./Eth_OneEye.o \
./GPIO_LED_Button.o \
./UserFunctions.o 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean--2e-

clean--2e-:
	-$(RM) ./ASCLIN_UART.d ./ASCLIN_UART.o ./Adc_MotorSense.d ./Adc_MotorSense.o ./Adc_MotorSense_Isr.d ./Adc_MotorSense_Isr.o ./Cpu0_Main.d ./Cpu0_Main.o ./Cpu1_Main.d ./Cpu1_Main.o ./Cpu2_Main.d ./Cpu2_Main.o ./Cpu3_Main.d ./Cpu3_Main.o ./Cpu4_Main.d ./Cpu4_Main.o ./Cpu5_Main.d ./Cpu5_Main.o ./EGTM_ATOM_3_Phase_Inverter_PWM.d ./EGTM_ATOM_3_Phase_Inverter_PWM.o ./Eth_Core.d ./Eth_Core.o ./Eth_OneEye.d ./Eth_OneEye.o ./GPIO_LED_Button.d ./GPIO_LED_Button.o ./UserFunctions.d ./UserFunctions.o

.PHONY: clean--2e-

