################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App_Feature/main_lcd_message/src/main_lcd_message.c 

OBJS += \
./App_Feature/main_lcd_message/src/main_lcd_message.o 

C_DEPS += \
./App_Feature/main_lcd_message/src/main_lcd_message.d 


# Each subdirectory must supply rules for building sources it contributes
App_Feature/main_lcd_message/src/%.o App_Feature/main_lcd_message/src/%.su App_Feature/main_lcd_message/src/%.cyclo: ../App_Feature/main_lcd_message/src/%.c App_Feature/main_lcd_message/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Peripherals/UART1/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Features/qspi_app_load/inc -I../App_Feature/main_lcd_message/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App_Feature-2f-main_lcd_message-2f-src

clean-App_Feature-2f-main_lcd_message-2f-src:
	-$(RM) ./App_Feature/main_lcd_message/src/main_lcd_message.cyclo ./App_Feature/main_lcd_message/src/main_lcd_message.d ./App_Feature/main_lcd_message/src/main_lcd_message.o ./App_Feature/main_lcd_message/src/main_lcd_message.su

.PHONY: clean-App_Feature-2f-main_lcd_message-2f-src

