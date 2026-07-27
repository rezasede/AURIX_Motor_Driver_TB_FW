################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../External/pdl/ifx_dre.c \
../External/pdl/ifx_geth.c \
../External/pdl/ifx_hsphy.c \
../External/pdl/ifx_leth.c \
../External/pdl/ifx_mcmcan.c 

C_DEPS += \
./External/pdl/ifx_dre.d \
./External/pdl/ifx_geth.d \
./External/pdl/ifx_hsphy.d \
./External/pdl/ifx_leth.d \
./External/pdl/ifx_mcmcan.d 

OBJS += \
./External/pdl/ifx_dre.o \
./External/pdl/ifx_geth.o \
./External/pdl/ifx_hsphy.o \
./External/pdl/ifx_leth.o \
./External/pdl/ifx_mcmcan.o 


# Each subdirectory must supply rules for building sources it contributes
External/pdl/%.o: ../External/pdl/%.c External/pdl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-External-2f-pdl

clean-External-2f-pdl:
	-$(RM) ./External/pdl/ifx_dre.d ./External/pdl/ifx_dre.o ./External/pdl/ifx_geth.d ./External/pdl/ifx_geth.o ./External/pdl/ifx_hsphy.d ./External/pdl/ifx_hsphy.o ./External/pdl/ifx_leth.d ./External/pdl/ifx_leth.o ./External/pdl/ifx_mcmcan.d ./External/pdl/ifx_mcmcan.o

.PHONY: clean-External-2f-pdl

