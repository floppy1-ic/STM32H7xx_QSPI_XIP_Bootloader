################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Features/qspi_app_load/src/qspi_app_load.c 

OBJS += \
./Features/qspi_app_load/src/qspi_app_load.o 

C_DEPS += \
./Features/qspi_app_load/src/qspi_app_load.d 


# Each subdirectory must supply rules for building sources it contributes
Features/qspi_app_load/src/%.o Features/qspi_app_load/src/%.su Features/qspi_app_load/src/%.cyclo: ../Features/qspi_app_load/src/%.c Features/qspi_app_load/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Features-2f-qspi_app_load-2f-src

clean-Features-2f-qspi_app_load-2f-src:
	-$(RM) ./Features/qspi_app_load/src/qspi_app_load.cyclo ./Features/qspi_app_load/src/qspi_app_load.d ./Features/qspi_app_load/src/qspi_app_load.o ./Features/qspi_app_load/src/qspi_app_load.su

.PHONY: clean-Features-2f-qspi_app_load-2f-src

