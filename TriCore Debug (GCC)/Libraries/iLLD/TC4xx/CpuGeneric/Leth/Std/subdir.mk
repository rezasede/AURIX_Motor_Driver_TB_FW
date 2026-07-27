################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/IfxLeth.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/IfxLeth.d 

OBJS += \
./Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/IfxLeth.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/%.o: ../Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/%.c Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Leth-2f-Std

clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Leth-2f-Std:
	-$(RM) ./Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/IfxLeth.d ./Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/IfxLeth.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Leth-2f-Std

