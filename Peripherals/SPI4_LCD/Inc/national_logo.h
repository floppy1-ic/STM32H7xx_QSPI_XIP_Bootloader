/**
  ******************************************************************************
  * @file    national_logo.h
  * @brief   Indian national flag (Tiranga) with lcd_mcal backlight breath in/out.
  ******************************************************************************
  */

#ifndef NATIONAL_LOGO_H
#define NATIONAL_LOGO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  Full-screen Tiranga between lcd_stm32h7_breathin and lcd_stm32h7_breathout;
  *         backlight restored to 100 % afterward.
  */
void national_logo(void);

#ifdef __cplusplus
}
#endif

#endif /* NATIONAL_LOGO_H */
