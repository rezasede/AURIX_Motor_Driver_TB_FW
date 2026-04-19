################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Libraries/iLLD/TC4xx/Tricore/_PinMap/TC4Dx/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/_Impl/TC4Dx/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/_Impl/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Wtu/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Vmt/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Stm/Timer/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Stm/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Smu/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Smm/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Pms/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Cpu/Trap/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Cpu/Std/subdir.mk
-include Libraries/iLLD/TC4xx/Tricore/Cpu/Irq/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/_PinMap/TC4Dx/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/_PinMap/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/_Lib/DataHandling/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/_Impl/TC4Dx/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/_Impl/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Src/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Port/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Leth/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Hsphy/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Geth/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Geth/Eth/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Trigger/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Egtm/Pwm/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Dma/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Dma/Dma/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Clock/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Asclin/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Asclin/Asc/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Ap/Std/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Adc/Tmadc/subdir.mk
-include Libraries/iLLD/TC4xx/CpuGeneric/Adc/Std/subdir.mk
-include Libraries/Service/CpuGeneric/SysSe/Time/subdir.mk
-include Libraries/Service/CpuGeneric/SysSe/Math/subdir.mk
-include Libraries/Service/CpuGeneric/SysSe/General/subdir.mk
-include Libraries/Service/CpuGeneric/SysSe/Comm/subdir.mk
-include Libraries/Service/CpuGeneric/SysSe/Bsp/subdir.mk
-include Libraries/Service/CpuGeneric/StdIf/subdir.mk
-include Libraries/Service/CpuGeneric/If/subdir.mk
-include Libraries/Infra/Ssw/TC4xx/Tricore/subdir.mk
-include Libraries/Infra/Platform/Compilers/subdir.mk
-include Libraries/Infra/Ipc/Tricore/ipc/src/subdir.mk
-include External/phy/subdir.mk
-include External/pdl/subdir.mk
-include Configurations/subdir.mk
-include subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Motor_Driver
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
Motor_Driver.hex \

SECONDARY_LIST += \
Motor_Driver.lst \

SECONDARY_SIZE += \
Motor_Driver.siz \


# All Target
all: main-build

# Main-build Target
main-build: Motor_Driver.elf secondary-outputs

# Tool invocations
Motor_Driver.elf: $(OBJS) ../Lcf_Gcc_Tricore_Tc.lsl makefile $(OPTIONAL_TOOL_DEPS) .Motor_Driver.elf.opt
	@echo 'Building target: $@'
	@echo 'Invoking: AURIX GCC Linker'
	tricore-elf-gcc @.Motor_Driver.elf.opt
	@echo 'Finished building target: $@'
	@echo ' '

.Motor_Driver.elf.opt:
	$(file >$@, -T"../Lcf_Gcc_Tricore_Tc.lsl" -nocrt0 -Wl,--gc-sections -Wl,-Map,Motor_Driver.map -mcpu=tc4DAx -o "Motor_Driver.elf" $(OBJS))

.PHONY: .Motor_Driver.elf.opt

Motor_Driver.hex: Motor_Driver.elf makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: AURIX Object Copy'
	tricore-elf-objcopy -O ihex "Motor_Driver.elf" "Motor_Driver.hex"
	@echo 'Finished building: $@'
	@echo ' '

Motor_Driver.lst: Motor_Driver.elf makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: AURIX Create Listing'
	tricore-elf-objdump -h -S "Motor_Driver.elf" > "Motor_Driver.lst"
	@echo 'Finished building: $@'
	@echo ' '

Motor_Driver.siz: Motor_Driver.elf makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: AURIX Print Size'
	tricore-elf-size  --format=berkeley "Motor_Driver.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Motor_Driver.elf Motor_Driver.hex Motor_Driver.lst Motor_Driver.siz
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents main-build

-include ../makefile.targets
