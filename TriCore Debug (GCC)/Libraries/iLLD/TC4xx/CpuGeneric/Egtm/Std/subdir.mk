################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Atom.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Cmu.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Dtm.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Spe.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tbu.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tim.c \
../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tom.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Atom.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Cmu.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Dtm.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Spe.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tbu.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tim.d \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tom.d 

OBJS += \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Atom.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Cmu.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Dtm.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Spe.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tbu.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tim.o \
./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tom.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/%.o: ../Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/%.c Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Egtm-2f-Std

clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Egtm-2f-Std:
	-$(RM) ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Atom.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Atom.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Cmu.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Cmu.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Dtm.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Dtm.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Spe.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Spe.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tbu.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tbu.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tim.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tim.o ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tom.d ./Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/IfxEgtm_Tom.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-Egtm-2f-Std

