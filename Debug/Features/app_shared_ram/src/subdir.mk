################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Features/app_shared_ram/src/app_shared_ram.c 

OBJS += \
./Features/app_shared_ram/src/app_shared_ram.o 

C_DEPS += \
./Features/app_shared_ram/src/app_shared_ram.d 


# Each subdirectory must supply rules for building sources it contributes
Features/app_shared_ram/src/%.o Features/app_shared_ram/src/%.su Features/app_shared_ram/src/%.cyclo: ../Features/app_shared_ram/src/%.c Features/app_shared_ram/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Peripherals/UART1/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Features/qspi_app_load/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Features-2f-app_shared_ram-2f-src

clean-Features-2f-app_shared_ram-2f-src:
	-$(RM) ./Features/app_shared_ram/src/app_shared_ram.cyclo ./Features/app_shared_ram/src/app_shared_ram.d ./Features/app_shared_ram/src/app_shared_ram.o ./Features/app_shared_ram/src/app_shared_ram.su

.PHONY: clean-Features-2f-app_shared_ram-2f-src

