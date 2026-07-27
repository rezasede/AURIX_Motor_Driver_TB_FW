################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/IfxVmt_cfg_TC4Dx.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/IfxVmt_cfg_TC4Dx.d 

OBJS += \
./Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/IfxVmt_cfg_TC4Dx.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/%.o: ../Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/%.c Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-_Impl-2f-TC4Dx

clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-_Impl-2f-TC4Dx:
	-$(RM) ./Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/IfxVmt_cfg_TC4Dx.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/IfxVmt_cfg_TC4Dx.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-_Impl-2f-TC4Dx

