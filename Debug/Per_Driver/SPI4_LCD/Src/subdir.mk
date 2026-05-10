################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Per_Driver/SPI4_LCD/Src/lcd_app.c \
../Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.c \
../Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.c \
../Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.c \
../Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.c \
../Per_Driver/SPI4_LCD/Src/lcd_mcal.c 

OBJS += \
./Per_Driver/SPI4_LCD/Src/lcd_app.o \
./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.o \
./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.o \
./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.o \
./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.o \
./Per_Driver/SPI4_LCD/Src/lcd_mcal.o 

C_DEPS += \
./Per_Driver/SPI4_LCD/Src/lcd_app.d \
./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.d \
./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.d \
./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.d \
./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.d \
./Per_Driver/SPI4_LCD/Src/lcd_mcal.d 


# Each subdirectory must supply rules for building sources it contributes
Per_Driver/SPI4_LCD/Src/%.o Per_Driver/SPI4_LCD/Src/%.su Per_Driver/SPI4_LCD/Src/%.cyclo: ../Per_Driver/SPI4_LCD/Src/%.c Per_Driver/SPI4_LCD/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Per_Driver/QSPI_Flash/Inc -I../Per_Driver/SPI4_LCD/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Per_Driver-2f-SPI4_LCD-2f-Src

clean-Per_Driver-2f-SPI4_LCD-2f-Src:
	-$(RM) ./Per_Driver/SPI4_LCD/Src/lcd_app.cyclo ./Per_Driver/SPI4_LCD/Src/lcd_app.d ./Per_Driver/SPI4_LCD/Src/lcd_app.o ./Per_Driver/SPI4_LCD/Src/lcd_app.su ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.cyclo ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.d ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.o ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_128_160.su ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.cyclo ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.d ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.o ./Per_Driver/SPI4_LCD/Src/lcd_asset_logo_160_80.su ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.cyclo ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.d ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.o ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735.su ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.cyclo ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.d ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.o ./Per_Driver/SPI4_LCD/Src/lcd_driver_st7735_reg.su ./Per_Driver/SPI4_LCD/Src/lcd_mcal.cyclo ./Per_Driver/SPI4_LCD/Src/lcd_mcal.d ./Per_Driver/SPI4_LCD/Src/lcd_mcal.o ./Per_Driver/SPI4_LCD/Src/lcd_mcal.su

.PHONY: clean-Per_Driver-2f-SPI4_LCD-2f-Src

