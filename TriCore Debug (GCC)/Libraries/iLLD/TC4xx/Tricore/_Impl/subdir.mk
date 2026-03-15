################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxAudio_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCan_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCanxl_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCpu_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxDre_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxEray_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxFlash_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxHssl_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxI2c_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxMsc_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxNvmr_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5s_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxQspi_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSdmmc_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSent_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxStm_cfg.c \
../Libraries/iLLD/TC4xx/Tricore/_Impl/IfxXspi_cfg.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxAudio_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCan_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCanxl_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCpu_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxDre_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxEray_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxFlash_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxHssl_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxI2c_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxMsc_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxNvmr_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5s_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxQspi_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSdmmc_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSent_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxStm_cfg.d \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxXspi_cfg.d 

OBJS += \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxAudio_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCan_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCanxl_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCpu_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxDre_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxEray_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxFlash_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxHssl_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxI2c_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxMsc_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxNvmr_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5s_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxQspi_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSdmmc_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSent_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxStm_cfg.o \
./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxXspi_cfg.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/Tricore/_Impl/%.o: ../Libraries/iLLD/TC4xx/Tricore/_Impl/%.c Libraries/iLLD/TC4xx/Tricore/_Impl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-_Impl

clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-_Impl:
	-$(RM) ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxAudio_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxAudio_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCan_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCan_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCanxl_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCanxl_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCpu_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxCpu_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxDre_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxDre_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxEray_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxEray_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxFlash_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxFlash_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxHssl_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxHssl_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxI2c_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxI2c_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxMsc_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxMsc_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxNvmr_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxNvmr_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5s_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxPsi5s_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxQspi_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxQspi_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSdmmc_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSdmmc_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSent_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxSent_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxStm_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxStm_cfg.o ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxXspi_cfg.d ./Libraries/iLLD/TC4xx/Tricore/_Impl/IfxXspi_cfg.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-Tricore-2f-_Impl

