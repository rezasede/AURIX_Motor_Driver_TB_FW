################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAdcCdspFw_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAp_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAsclin_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxDma_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxEgtm_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGeth_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGpt12_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGtm_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxHsphy_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxLeth_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxPcie_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRdma_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRif_cfg.c \
../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxSpu_cfg.c 

C_DEPS += \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAdcCdspFw_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAp_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAsclin_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxDma_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxEgtm_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGeth_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGpt12_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGtm_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxHsphy_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxLeth_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxPcie_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRdma_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRif_cfg.d \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxSpu_cfg.d 

OBJS += \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAdcCdspFw_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAp_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAsclin_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxDma_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxEgtm_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGeth_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGpt12_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGtm_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxHsphy_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxLeth_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxPcie_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRdma_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRif_cfg.o \
./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxSpu_cfg.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/iLLD/TC4xx/CpuGeneric/_Impl/%.o: ../Libraries/iLLD/TC4xx/CpuGeneric/_Impl/%.c Libraries/iLLD/TC4xx/CpuGeneric/_Impl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AURIX GCC Compiler'
	tricore-elf-gcc -std=c99 "@C:/Users/rezas/AURIX-v1.10.28-workspace/Motor_Driver/TriCore Debug (GCC)/AURIX_GCC_Compiler-Include_paths__-I_.opt" -Og -g3 -gdwarf-3 -Wall -c -fmessage-length=0 -fno-common -fstrict-volatile-bitfields -fdata-sections -ffunction-sections -mcpu=tc4DAx -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-_Impl

clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-_Impl:
	-$(RM) ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAdcCdspFw_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAdcCdspFw_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAp_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAp_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAsclin_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxAsclin_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxDma_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxDma_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxEgtm_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxEgtm_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGeth_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGeth_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGpt12_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGpt12_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGtm_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxGtm_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxHsphy_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxHsphy_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxLeth_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxLeth_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxPcie_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxPcie_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRdma_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRdma_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRif_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxRif_cfg.o ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxSpu_cfg.d ./Libraries/iLLD/TC4xx/CpuGeneric/_Impl/IfxSpu_cfg.o

.PHONY: clean-Libraries-2f-iLLD-2f-TC4xx-2f-CpuGeneric-2f-_Impl

