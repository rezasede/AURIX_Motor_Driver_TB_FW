################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UserLibs/Control/Control_Loops.c \
../UserLibs/Control/FOC.c 

C_DEPS += \
./UserLibs/Control/Control_Loops.d \
./UserLibs/Control/FOC.d 

OBJS += \
./UserLibs/Control/Control_Loops.o \
./UserLibs/Control/FOC.o 


# Each subdirectory must supply rules for building sources it contributes
UserLibs/Control/%.o: ../UserLibs/Control/%.c UserLibs/Control/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-UserLibs-2f-Control

clean-UserLibs-2f-Control:
	-$(RM) ./UserLibs/Control/Control_Loops.d ./UserLibs/Control/Control_Loops.o ./UserLibs/Control/FOC.d ./UserLibs/Control/FOC.o

.PHONY: clean-UserLibs-2f-Control

