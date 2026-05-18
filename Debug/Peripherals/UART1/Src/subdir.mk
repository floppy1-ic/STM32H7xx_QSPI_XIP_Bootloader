################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripherals/UART1/Src/uart_mcal.c 

OBJS += \
./Peripherals/UART1/Src/uart_mcal.o 

C_DEPS += \
./Peripherals/UART1/Src/uart_mcal.d 


# Each subdirectory must supply rules for building sources it contributes
Peripherals/UART1/Src/%.o Peripherals/UART1/Src/%.su Peripherals/UART1/Src/%.cyclo: ../Peripherals/UART1/Src/%.c Peripherals/UART1/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Peripherals/UART1/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Features/qspi_app_load/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Peripherals-2f-UART1-2f-Src

clean-Peripherals-2f-UART1-2f-Src:
	-$(RM) ./Peripherals/UART1/Src/uart_mcal.cyclo ./Peripherals/UART1/Src/uart_mcal.d ./Peripherals/UART1/Src/uart_mcal.o ./Peripherals/UART1/Src/uart_mcal.su

.PHONY: clean-Peripherals-2f-UART1-2f-Src

