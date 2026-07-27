################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/Tricore/Stm/Timer/IfxStm_Timer.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/Tricore/Stm/Timer/IfxStm_Timer.d 

OBJS += \
./Libraries/iLLD/TC4xx/Tricore/Stm/Timer/IfxStm_Timer.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/Tricore/Stm/Timer/%.o: ../Libraries/iLLD/TC4xx/Tricore/Stm/Timer/%.c Libraries/iLLD/TC4xx/Tricore/Stm/Timer/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-Stm-2f-Timer

clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-Stm-2f-Timer:
	-$(RM) ./Libraries/iLLD/TC4xx/Tricore/Stm/Timer/IfxStm_Timer.d ./Libraries/iLLD/TC4xx/Tricore/Stm/Timer/IfxStm_Timer.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-Stm-2f-Timer

