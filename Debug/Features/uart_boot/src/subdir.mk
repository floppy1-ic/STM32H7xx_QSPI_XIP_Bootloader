################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Features/uart_boot/src/uart_boot.c 

OBJS += \
./Features/uart_boot/src/uart_boot.o 

C_DEPS += \
./Features/uart_boot/src/uart_boot.d 

# Each subdirectory must supply rules for building sources.
Features/uart_boot/src/%.o Features/uart_boot/src/%.su Features/uart_boot/src/%.cyclo: ../Features/uart_boot/src/%.c Features/uart_boot/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Features/qspi_app_load/inc -I../Features/uart_boot/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Features-2f-uart_boot-2f-src

clean-Features-2f-uart_boot-2f-src:
	-$(RM) ./Features/uart_boot/src/uart_boot.cyclo ./Features/uart_boot/src/uart_boot.d ./Features/uart_boot/src/uart_boot.o ./Features/uart_boot/src/uart_boot.su

.PHONY: clean-Features-2f-uart_boot-2f-src

