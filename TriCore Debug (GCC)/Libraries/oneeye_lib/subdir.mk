################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/oneeye_lib/ifx_oe_circularbuffer.c \
../Libraries/oneeye_lib/ifx_oe_dasfifo.c \
../Libraries/oneeye_lib/ifx_oe_daspipe.c \
../Libraries/oneeye_lib/ifx_oe_fifo.c \
../Libraries/oneeye_lib/ifx_oe_fifodpipe.c \
../Libraries/oneeye_lib/ifx_oe_linkedlist.c \
../Libraries/oneeye_lib/ifx_oe_log.c \
../Libraries/oneeye_lib/ifx_oe_malloc.c \
../Libraries/oneeye_lib/ifx_oe_osci.c \
../Libraries/oneeye_lib/ifx_oe_oscibb.c \
../Libraries/oneeye_lib/ifx_oe_shell.c \
../Libraries/oneeye_lib/ifx_oe_shellbb.c \
../Libraries/oneeye_lib/ifx_oe_stdif_dpipe.c \
../Libraries/oneeye_lib/ifx_oe_syncprotocol.c \
../Libraries/oneeye_lib/ifx_oe_time.c 

C_DEPS += \
./Libraries/oneeye_lib/ifx_oe_circularbuffer.d \
./Libraries/oneeye_lib/ifx_oe_dasfifo.d \
./Libraries/oneeye_lib/ifx_oe_daspipe.d \
./Libraries/oneeye_lib/ifx_oe_fifo.d \
./Libraries/oneeye_lib/ifx_oe_fifodpipe.d \
./Libraries/oneeye_lib/ifx_oe_linkedlist.d \
./Libraries/oneeye_lib/ifx_oe_log.d \
./Libraries/oneeye_lib/ifx_oe_malloc.d \
./Libraries/oneeye_lib/ifx_oe_osci.d \
./Libraries/oneeye_lib/ifx_oe_oscibb.d \
./Libraries/oneeye_lib/ifx_oe_shell.d \
./Libraries/oneeye_lib/ifx_oe_shellbb.d \
./Libraries/oneeye_lib/ifx_oe_stdif_dpipe.d \
./Libraries/oneeye_lib/ifx_oe_syncprotocol.d \
./Libraries/oneeye_lib/ifx_oe_time.d 

OBJS += \
./Libraries/oneeye_lib/ifx_oe_circularbuffer.o \
./Libraries/oneeye_lib/ifx_oe_dasfifo.o \
./Libraries/oneeye_lib/ifx_oe_daspipe.o \
./Libraries/oneeye_lib/ifx_oe_fifo.o \
./Libraries/oneeye_lib/ifx_oe_fifodpipe.o \
./Libraries/oneeye_lib/ifx_oe_linkedlist.o \
./Libraries/oneeye_lib/ifx_oe_log.o \
./Libraries/oneeye_lib/ifx_oe_malloc.o \
./Libraries/oneeye_lib/ifx_oe_osci.o \
./Libraries/oneeye_lib/ifx_oe_oscibb.o \
./Libraries/oneeye_lib/ifx_oe_shell.o \
./Libraries/oneeye_lib/ifx_oe_shellbb.o \
./Libraries/oneeye_lib/ifx_oe_stdif_dpipe.o \
./Libraries/oneeye_lib/ifx_oe_syncprotocol.o \
./Libraries/oneeye_lib/ifx_oe_time.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/oneeye_lib/%.o: ../Libraries/oneeye_lib/%.c Libraries/oneeye_lib/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@D:/Projects/GitHub/AURIX_Motor_Driver_FW/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-oneeye_lib

clean-Libraries-2f-oneeye_lib:
	-$(RM) ./Libraries/oneeye_lib/ifx_oe_circularbuffer.d ./Libraries/oneeye_lib/ifx_oe_circularbuffer.o ./Libraries/oneeye_lib/ifx_oe_dasfifo.d ./Libraries/oneeye_lib/ifx_oe_dasfifo.o ./Libraries/oneeye_lib/ifx_oe_daspipe.d ./Libraries/oneeye_lib/ifx_oe_daspipe.o ./Libraries/oneeye_lib/ifx_oe_fifo.d ./Libraries/oneeye_lib/ifx_oe_fifo.o ./Libraries/oneeye_lib/ifx_oe_fifodpipe.d ./Libraries/oneeye_lib/ifx_oe_fifodpipe.o ./Libraries/oneeye_lib/ifx_oe_linkedlist.d ./Libraries/oneeye_lib/ifx_oe_linkedlist.o ./Libraries/oneeye_lib/ifx_oe_log.d ./Libraries/oneeye_lib/ifx_oe_log.o ./Libraries/oneeye_lib/ifx_oe_malloc.d ./Libraries/oneeye_lib/ifx_oe_malloc.o ./Libraries/oneeye_lib/ifx_oe_osci.d ./Libraries/oneeye_lib/ifx_oe_osci.o ./Libraries/oneeye_lib/ifx_oe_oscibb.d ./Libraries/oneeye_lib/ifx_oe_oscibb.o ./Libraries/oneeye_lib/ifx_oe_shell.d ./Libraries/oneeye_lib/ifx_oe_shell.o ./Libraries/oneeye_lib/ifx_oe_shellbb.d ./Libraries/oneeye_lib/ifx_oe_shellbb.o ./Libraries/oneeye_lib/ifx_oe_stdif_dpipe.d ./Libraries/oneeye_lib/ifx_oe_stdif_dpipe.o ./Libraries/oneeye_lib/ifx_oe_syncprotocol.d ./Libraries/oneeye_lib/ifx_oe_syncprotocol.o ./Libraries/oneeye_lib/ifx_oe_time.d ./Libraries/oneeye_lib/ifx_oe_time.o

.PHONY: clean-Libraries-2f-oneeye_lib

