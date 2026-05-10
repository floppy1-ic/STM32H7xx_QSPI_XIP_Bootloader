################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../QSPI_Flash/Src/qspi_flash.c 

OBJS += \
./QSPI_Flash/Src/qspi_flash.o 

C_DEPS += \
./QSPI_Flash/Src/qspi_flash.d 


# Each subdirectory must supply rules for building sources it contributes
QSPI_Flash/Src/%.o QSPI_Flash/Src/%.su QSPI_Flash/Src/%.cyclo: ../QSPI_Flash/Src/%.c QSPI_Flash/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../QSPI_Flash/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-QSPI_Flash-2f-Src

clean-QSPI_Flash-2f-Src:
	-$(RM) ./QSPI_Flash/Src/qspi_flash.cyclo ./QSPI_Flash/Src/qspi_flash.d ./QSPI_Flash/Src/qspi_flash.o ./QSPI_Flash/Src/qspi_flash.su

.PHONY: clean-QSPI_Flash-2f-Src

