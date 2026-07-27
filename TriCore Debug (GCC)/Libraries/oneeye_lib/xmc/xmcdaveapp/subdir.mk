################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/oneeye_lib/xmc/xmcdaveapp/ifx_oe_al_xmc_xmcdaveapp_uartdpipe.c 

C_DEPS += \
./Libraries/oneeye_lib/xmc/xmcdaveapp/ifx_oe_al_xmc_xmcdaveapp_uartdpipe.d 

OBJS += \
./Libraries/oneeye_lib/xmc/xmcdaveapp/ifx_oe_al_xmc_xmcdaveapp_uartdpipe.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/oneeye_lib/xmc/xmcdaveapp/%.o: ../Libraries/oneeye_lib/xmc/xmcdaveapp/%.c Libraries/oneeye_lib/xmc/xmcdaveapp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-oneeye_lib-2f-xmc-2f-xmcdaveapp

clean-Libraries-2f-oneeye_lib-2f-xmc-2f-xmcdaveapp:
	-$(RM) ./Libraries/oneeye_lib/xmc/xmcdaveapp/ifx_oe_al_xmc_xmcdaveapp_uartdpipe.d ./Libraries/oneeye_lib/xmc/xmcdaveapp/ifx_oe_al_xmc_xmcdaveapp_uartdpipe.o

.PHONY: clean-Libraries-2f-oneeye_lib-2f-xmc-2f-xmcdaveapp

