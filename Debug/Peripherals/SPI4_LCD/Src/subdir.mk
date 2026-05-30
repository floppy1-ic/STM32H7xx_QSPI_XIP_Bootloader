################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripherals/SPI4_LCD/Src/lcd_app.c \
../Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.c \
../Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.c \
../Peripherals/SPI4_LCD/Src/lcd_driver_st7735.c \
../Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.c \
../Peripherals/SPI4_LCD/Src/lcd_mcal.c \
../Peripherals/SPI4_LCD/Src/national_logo.c 

OBJS += \
./Peripherals/SPI4_LCD/Src/lcd_app.o \
./Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.o \
./Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.o \
./Peripherals/SPI4_LCD/Src/lcd_driver_st7735.o \
./Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.o \
./Peripherals/SPI4_LCD/Src/lcd_mcal.o \
./Peripherals/SPI4_LCD/Src/national_logo.o 

C_DEPS += \
./Peripherals/SPI4_LCD/Src/lcd_app.d \
./Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.d \
./Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.d \
./Peripherals/SPI4_LCD/Src/lcd_driver_st7735.d \
./Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.d \
./Peripherals/SPI4_LCD/Src/lcd_mcal.d \
./Peripherals/SPI4_LCD/Src/national_logo.d 


# Each subdirectory must supply rules for building sources it contributes
Peripherals/SPI4_LCD/Src/%.o Peripherals/SPI4_LCD/Src/%.su Peripherals/SPI4_LCD/Src/%.cyclo: ../Peripherals/SPI4_LCD/Src/%.c Peripherals/SPI4_LCD/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Peripherals/QSPI_Flash/Inc -I../Peripherals/SPI4_LCD/Inc -I../Peripherals/UART1/Inc -I../Features/qspi_app_jump/inc -I../Features/app_shared_ram/inc -I../Features/qspi_app_load/inc -I../App_Feature/main_lcd_message/inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Peripherals-2f-SPI4_LCD-2f-Src

clean-Peripherals-2f-SPI4_LCD-2f-Src:
	-$(RM) ./Peripherals/SPI4_LCD/Src/lcd_app.cyclo ./Peripherals/SPI4_LCD/Src/lcd_app.d ./Peripherals/SPI4_LCD/Src/lcd_app.o ./Peripherals/SPI4_LCD/Src/lcd_app.su ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.cyclo ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.d ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.o ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_128_160.su ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.cyclo ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.d ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.o ./Peripherals/SPI4_LCD/Src/lcd_asset_logo_160_80.su ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735.cyclo ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735.d ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735.o ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735.su ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.cyclo ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.d ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.o ./Peripherals/SPI4_LCD/Src/lcd_driver_st7735_reg.su ./Peripherals/SPI4_LCD/Src/lcd_mcal.cyclo ./Peripherals/SPI4_LCD/Src/lcd_mcal.d ./Peripherals/SPI4_LCD/Src/lcd_mcal.o ./Peripherals/SPI4_LCD/Src/lcd_mcal.su ./Peripherals/SPI4_LCD/Src/national_logo.cyclo ./Peripherals/SPI4_LCD/Src/national_logo.d ./Peripherals/SPI4_LCD/Src/national_logo.o ./Peripherals/SPI4_LCD/Src/national_logo.su

.PHONY: clean-Peripherals-2f-SPI4_LCD-2f-Src

