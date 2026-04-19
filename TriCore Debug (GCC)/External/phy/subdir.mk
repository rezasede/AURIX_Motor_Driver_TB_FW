################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../External/phy/dp83825i.c \
../External/phy/pef7071.c \
../External/phy/phy.c \
../External/phy/rtl8201f.c \
../External/phy/rtl8211f.c \
../External/phy/rtl8211fsi.c \
../External/phy/rtl8221b.c \
../External/phy/rtl8251b.c 

C_DEPS += \
./External/phy/dp83825i.d \
./External/phy/pef7071.d \
./External/phy/phy.d \
./External/phy/rtl8201f.d \
./External/phy/rtl8211f.d \
./External/phy/rtl8211fsi.d \
./External/phy/rtl8221b.d \
./External/phy/rtl8251b.d 

OBJS += \
./External/phy/dp83825i.o \
./External/phy/pef7071.o \
./External/phy/phy.o \
./External/phy/rtl8201f.o \
./External/phy/rtl8211f.o \
./External/phy/rtl8211fsi.o \
./External/phy/rtl8221b.o \
./External/phy/rtl8251b.o 


# Each subdirectory must supply rules for building sources it contributes
External/phy/%.o: ../External/phy/%.c External/phy/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-External-2f-phy

clean-External-2f-phy:
	-$(RM) ./External/phy/dp83825i.d ./External/phy/dp83825i.o ./External/phy/pef7071.d ./External/phy/pef7071.o ./External/phy/phy.d ./External/phy/phy.o ./External/phy/rtl8201f.d ./External/phy/rtl8201f.o ./External/phy/rtl8211f.d ./External/phy/rtl8211f.o ./External/phy/rtl8211fsi.d ./External/phy/rtl8211fsi.o ./External/phy/rtl8221b.d ./External/phy/rtl8221b.o ./External/phy/rtl8251b.d ./External/phy/rtl8251b.o

.PHONY: clean-External-2f-phy

