################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/Service/CpuGeneric/SysSe/Bsp/Bsp.c \
../Libraries/Service/CpuGeneric/SysSe/Bsp/Ifx_illdAssert.c 

C_DEPS += \
./Libraries/Service/CpuGeneric/SysSe/Bsp/Bsp.d \
./Libraries/Service/CpuGeneric/SysSe/Bsp/Ifx_illdAssert.d 

OBJS += \
./Libraries/Service/CpuGeneric/SysSe/Bsp/Bsp.o \
./Libraries/Service/CpuGeneric/SysSe/Bsp/Ifx_illdAssert.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/Service/CpuGeneric/SysSe/Bsp/%.o: ../Libraries/Service/CpuGeneric/SysSe/Bsp/%.c Libraries/Service/CpuGeneric/SysSe/Bsp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-Service-2f-CpuGeneric-2f-SysSe-2f-Bsp

clean-Libraries-2f-Service-2f-CpuGeneric-2f-SysSe-2f-Bsp:
	-$(RM) ./Libraries/Service/CpuGeneric/SysSe/Bsp/Bsp.d ./Libraries/Service/CpuGeneric/SysSe/Bsp/Bsp.o ./Libraries/Service/CpuGeneric/SysSe/Bsp/Ifx_illdAssert.d ./Libraries/Service/CpuGeneric/SysSe/Bsp/Ifx_illdAssert.o

.PHONY: clean-Libraries-2f-Service-2f-CpuGeneric-2f-SysSe-2f-Bsp

