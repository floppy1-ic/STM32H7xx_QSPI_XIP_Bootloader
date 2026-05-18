################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripherals/QSPI_Flash/Src/qspi_flash.c 

OBJS += \
./Peripherals/QSPI_Flash/Src/qspi_flash.o 

C_DEPS += \
./Peripherals/QSPI_Flash/Src/qspi_flash.d 


# Each subdirectory must supply rules for building sources it contributes
Peripherals/QSPI_Flash/Src/%.o Peripherals/QSPI_Flash/Src/%.su Peripherals/QSPI_Flash/Src/%.cyclo: ../Peripherals/QSPI_Flash/Src/%.c Peripherals/QSPI_Flash/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Peripherals/UART1/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Features/qspi_app_load/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Peripherals-2f-QSPI_Flash-2f-Src

clean-Peripherals-2f-QSPI_Flash-2f-Src:
	-$(RM) ./Peripherals/QSPI_Flash/Src/qspi_flash.cyclo ./Peripherals/QSPI_Flash/Src/qspi_flash.d ./Peripherals/QSPI_Flash/Src/qspi_flash.o ./Peripherals/QSPI_Flash/Src/qspi_flash.su

.PHONY: clean-Peripherals-2f-QSPI_Flash-2f-Src

