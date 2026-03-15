################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/Infra/Ipc/Tricore/ipc/src/IfxTcIpc.c 

C_DEPS += \
./Libraries/Infra/Ipc/Tricore/ipc/src/IfxTcIpc.d 

OBJS += \
./Libraries/Infra/Ipc/Tricore/ipc/src/IfxTcIpc.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/Infra/Ipc/Tricore/ipc/src/%.o: ../Libraries/Infra/Ipc/Tricore/ipc/src/%.c Libraries/Infra/Ipc/Tricore/ipc/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-Infra-2f-Ipc-2f-Tricore-2f-ipc-2f-src

clean-Libraries-2f-Infra-2f-Ipc-2f-Tricore-2f-ipc-2f-src:
	-$(RM) ./Libraries/Infra/Ipc/Tricore/ipc/src/IfxTcIpc.d ./Libraries/Infra/Ipc/Tricore/ipc/src/IfxTcIpc.o

.PHONY: clean-Libraries-2f-Infra-2f-Ipc-2f-Tricore-2f-ipc-2f-src

