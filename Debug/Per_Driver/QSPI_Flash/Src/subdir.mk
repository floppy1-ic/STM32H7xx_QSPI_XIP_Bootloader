################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Per_Driver/QSPI_Flash/Src/qspi_flash.c 

OBJS += \
./Per_Driver/QSPI_Flash/Src/qspi_flash.o 

C_DEPS += \
./Per_Driver/QSPI_Flash/Src/qspi_flash.d 


# Each subdirectory must supply rules for building sources it contributes
Per_Driver/QSPI_Flash/Src/%.o Per_Driver/QSPI_Flash/Src/%.su Per_Driver/QSPI_Flash/Src/%.cyclo: ../Per_Driver/QSPI_Flash/Src/%.c Per_Driver/QSPI_Flash/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Per_Driver/QSPI_Flash/Inc -I../Per_Driver/SPI4_LCD/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Per_Driver-2f-QSPI_Flash-2f-Src

clean-Per_Driver-2f-QSPI_Flash-2f-Src:
	-$(RM) ./Per_Driver/QSPI_Flash/Src/qspi_flash.cyclo ./Per_Driver/QSPI_Flash/Src/qspi_flash.d ./Per_Driver/QSPI_Flash/Src/qspi_flash.o ./Per_Driver/QSPI_Flash/Src/qspi_flash.su

.PHONY: clean-Per_Driver-2f-QSPI_Flash-2f-Src

