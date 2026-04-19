################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/IfxGeth_Eth.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/IfxGeth_Eth.d 

OBJS += \
./Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/IfxGeth_Eth.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/%.o: ../Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/%.c Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Geth-2f-Eth

clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Geth-2f-Eth:
	-$(RM) ./Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/IfxGeth_Eth.d ./Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/IfxGeth_Eth.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Geth-2f-Eth

