################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl.c \
../Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl_sbcdmadpipe.c 

C_DEPS += \
./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl.d \
./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl_sbcdmadpipe.d 

OBJS += \
./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl.o \
./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl_sbcdmadpipe.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/oneeye_lib/cy/pdl/%.o: ../Libraries/oneeye_lib/cy/pdl/%.c Libraries/oneeye_lib/cy/pdl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-oneeye_lib-2f-cy-2f-pdl

clean-Libraries-2f-oneeye_lib-2f-cy-2f-pdl:
	-$(RM) ./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl.d ./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl.o ./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl_sbcdmadpipe.d ./Libraries/oneeye_lib/cy/pdl/ifx_oe_al_cy_pdl_sbcdmadpipe.o

.PHONY: clean-Libraries-2f-oneeye_lib-2f-cy-2f-pdl

