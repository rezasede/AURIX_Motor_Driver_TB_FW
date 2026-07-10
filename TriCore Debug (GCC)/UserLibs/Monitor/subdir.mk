################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UserLibs/Monitor/Monitor_Json.c \
../UserLibs/Monitor/Stm_Monitor.c \
../UserLibs/Monitor/UserFunctions.c \
../UserLibs/Monitor/dap_tune.c 

C_DEPS += \
./UserLibs/Monitor/Monitor_Json.d \
./UserLibs/Monitor/Stm_Monitor.d \
./UserLibs/Monitor/UserFunctions.d \
./UserLibs/Monitor/dap_tune.d 

OBJS += \
./UserLibs/Monitor/Monitor_Json.o \
./UserLibs/Monitor/Stm_Monitor.o \
./UserLibs/Monitor/UserFunctions.o \
./UserLibs/Monitor/dap_tune.o 


# Each subdirectory must supply rules for building sources it contributes
UserLibs/Monitor/%.o: ../UserLibs/Monitor/%.c UserLibs/Monitor/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-UserLibs-2f-Monitor

clean-UserLibs-2f-Monitor:
	-$(RM) ./UserLibs/Monitor/Monitor_Json.d ./UserLibs/Monitor/Monitor_Json.o ./UserLibs/Monitor/Stm_Monitor.d ./UserLibs/Monitor/Stm_Monitor.o ./UserLibs/Monitor/UserFunctions.d ./UserLibs/Monitor/UserFunctions.o ./UserLibs/Monitor/dap_tune.d ./UserLibs/Monitor/dap_tune.o

.PHONY: clean-UserLibs-2f-Monitor

