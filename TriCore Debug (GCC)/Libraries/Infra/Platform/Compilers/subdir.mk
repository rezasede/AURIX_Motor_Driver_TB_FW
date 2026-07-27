################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/Infra/Platform/Compilers/CompilerGcc.c \
../Libraries/Infra/Platform/Compilers/CompilerGhs.c \
../Libraries/Infra/Platform/Compilers/CompilerGnuc.c \
../Libraries/Infra/Platform/Compilers/CompilerHighTec.c \
../Libraries/Infra/Platform/Compilers/CompilerMw.c \
../Libraries/Infra/Platform/Compilers/CompilerTasking.c \
../Libraries/Infra/Platform/Compilers/CompilerWindriver.c 

C_DEPS += \
./Libraries/Infra/Platform/Compilers/CompilerGcc.d \
./Libraries/Infra/Platform/Compilers/CompilerGhs.d \
./Libraries/Infra/Platform/Compilers/CompilerGnuc.d \
./Libraries/Infra/Platform/Compilers/CompilerHighTec.d \
./Libraries/Infra/Platform/Compilers/CompilerMw.d \
./Libraries/Infra/Platform/Compilers/CompilerTasking.d \
./Libraries/Infra/Platform/Compilers/CompilerWindriver.d 

OBJS += \
./Libraries/Infra/Platform/Compilers/CompilerGcc.o \
./Libraries/Infra/Platform/Compilers/CompilerGhs.o \
./Libraries/Infra/Platform/Compilers/CompilerGnuc.o \
./Libraries/Infra/Platform/Compilers/CompilerHighTec.o \
./Libraries/Infra/Platform/Compilers/CompilerMw.o \
./Libraries/Infra/Platform/Compilers/CompilerTasking.o \
./Libraries/Infra/Platform/Compilers/CompilerWindriver.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/Infra/Platform/Compilers/%.o: ../Libraries/Infra/Platform/Compilers/%.c Libraries/Infra/Platform/Compilers/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-Infra-2f-Platform-2f-Compilers

clean-Libraries-2f-Infra-2f-Platform-2f-Compilers:
	-$(RM) ./Libraries/Infra/Platform/Compilers/CompilerGcc.d ./Libraries/Infra/Platform/Compilers/CompilerGcc.o ./Libraries/Infra/Platform/Compilers/CompilerGhs.d ./Libraries/Infra/Platform/Compilers/CompilerGhs.o ./Libraries/Infra/Platform/Compilers/CompilerGnuc.d ./Libraries/Infra/Platform/Compilers/CompilerGnuc.o ./Libraries/Infra/Platform/Compilers/CompilerHighTec.d ./Libraries/Infra/Platform/Compilers/CompilerHighTec.o ./Libraries/Infra/Platform/Compilers/CompilerMw.d ./Libraries/Infra/Platform/Compilers/CompilerMw.o ./Libraries/Infra/Platform/Compilers/CompilerTasking.d ./Libraries/Infra/Platform/Compilers/CompilerTasking.o ./Libraries/Infra/Platform/Compilers/CompilerWindriver.d ./Libraries/Infra/Platform/Compilers/CompilerWindriver.o

.PHONY: clean-Libraries-2f-Infra-2f-Platform-2f-Compilers

