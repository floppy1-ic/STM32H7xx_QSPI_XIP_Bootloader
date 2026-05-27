/**
  ******************************************************************************
  * @file    qspi_app_jump.c
  * @author  Sibun
  * @brief   Jump from bootloader to application in external QSPI (XIP) flash.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#include "qspi_app_jump.h"
#include "qspi_flash.h"
#include "lcd_mcal.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* Cortex-M vector table: word0 = initial SP, word1 = Reset_Handler -------- */
#define QSPI_APP_JUMP_VECTOR_SP_OFFSET (0U)
#define QSPI_APP_JUMP_VECTOR_PC_OFFSET (4U)

/* Thumb mode: reset vector must have bit0 set -------------------------------- */
#define QSPI_APP_JUMP_THUMB_BIT_MASK   (1UL)

typedef void (*QSPI_App_Jump_ResetHandlerTypeDef)(void);

extern SPI_HandleTypeDef hspi4;
extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart1;

static bool QSPI_App_Jump_IsThumbAddress(uint32_t address)
{
  return ((address & QSPI_APP_JUMP_THUMB_BIT_MASK) != 0U);
}

static bool QSPI_App_Jump_IsStackPointerValid(uint32_t stackPointer)
{
  /* H7: DTCM / AXI SRAM typical ranges; reject obviously invalid SP values. */
  if (stackPointer < 0x20000000UL)
  {
    return false;
  }

  if ((stackPointer & 0x3UL) != 0U)
  {
    return false;
  }

  return true;
}

static bool QSPI_App_Jump_ValidateVectors(uint32_t initialSp, uint32_t resetHandler)
{
  if (!QSPI_App_Jump_IsStackPointerValid(initialSp))
  {
    return false;
  }

  if (!QSPI_App_Jump_IsThumbAddress(resetHandler))
  {
    return false;
  }

  return true;
}

bool qspi_app_is_valid(uint32_t appBaseAddress)
{
  const uint32_t *vectors = (const uint32_t *)appBaseAddress;
  uint32_t initialSp;
  uint32_t resetHandler;

  if ((appBaseAddress & 0x1FFUL) != 0U)
  {
    return false;
  }

  initialSp = vectors[QSPI_APP_JUMP_VECTOR_SP_OFFSET / sizeof(uint32_t)];
  resetHandler = vectors[QSPI_APP_JUMP_VECTOR_PC_OFFSET / sizeof(uint32_t)];

  return QSPI_App_Jump_ValidateVectors(initialSp, resetHandler);
}

bool qspi_app_is_valid_at_flash(QSPI_HandleTypeDef *hqspi, uint32_t flashByteOffset)
{
  uint8_t vectors[8];
  uint32_t initialSp;
  uint32_t resetHandler;

  if (hqspi == NULL)
  {
    return false;
  }

  if (QSPI_Flash_Read(hqspi, flashByteOffset, vectors, sizeof(vectors)) != QSPI_FLASH_OK)
  {
    return false;
  }

  memcpy(&initialSp, &vectors[0], sizeof(initialSp));
  memcpy(&resetHandler, &vectors[4], sizeof(resetHandler));

  return QSPI_App_Jump_ValidateVectors(initialSp, resetHandler);
}

/**
 * @brief Jump from bootloader to QSPI XIP application at appBaseAddress.
 * @param appBaseAddress Application vector table base (e.g. 0x90000000).
 * @retval QSPI_APP_JUMP_* status (does not return on success).
 */
 qspi_app_status_t qspi_app_jump_to_application(uint32_t appBaseAddress)
 {
   const uint32_t *vectors = (const uint32_t *)appBaseAddress;
   uint32_t initialSp;
   uint32_t resetHandler;
   QSPI_App_Jump_ResetHandlerTypeDef appReset;
 
   /* --- 1) Validate application vector table ------------------------------ */
   if (!qspi_app_is_valid(appBaseAddress))
   {
     return QSPI_APP_JUMP_INVALID_VECTOR;
   }
 
   initialSp = vectors[QSPI_APP_JUMP_VECTOR_SP_OFFSET / sizeof(uint32_t)];
   resetHandler = vectors[QSPI_APP_JUMP_VECTOR_PC_OFFSET / sizeof(uint32_t)];
 
   /* Cortex-M reset vector must be Thumb (LSB = 1). */
   if ((resetHandler & 0x1U) == 0U)
   {
     return QSPI_APP_JUMP_INVALID_VECTOR;
   }
 
   appReset = (QSPI_App_Jump_ResetHandlerTypeDef)resetHandler;
 
   /* --- 2) Stop interrupts during handoff --------------------------------- */
   /* Mask all IRQs until app re-enables them after HAL_Init(). */
   __disable_irq();
 
   /* --- 3) Stop bootloader SysTick (app will restart via HAL_InitTick) ---- */
   SysTick->CTRL = 0U;
   SysTick->LOAD = 0U;
   SysTick->VAL  = 0U;
   NVIC_ClearPendingIRQ(SysTick_IRQn);
 
   /* --- 4) Clear NVIC pending / disable external IRQ lines ---------------- */
   /* SysTick on Cortex-M is controlled by SysTick->CTRL, not NVIC->ISER.   */
   /* App HAL will re-enable UART/SPI/etc. IRQs as needed.                  */
   SCB->ICSR |= (SCB_ICSR_PENDSVCLR_Msk | SCB_ICSR_PENDSTCLR_Msk);
   for (uint32_t i = 0U; i < 8U; i++)
   {
     NVIC->ICER[i] = 0xFFFFFFFFUL;  /* disable */
     NVIC->ICPR[i] = 0xFFFFFFFFUL;  /* clear pending */
   }
 
   /* --- 5) Deinit bootloader-owned peripherals only --------------------- */
   /* Do NOT deinit QSPI / do NOT reset RCC — XIP fetch must stay alive.    */
   (void)HAL_UART_DeInit(&huart1);
   (void)HAL_SPI_DeInit(&hspi4);
   (void)HAL_TIM_Base_DeInit(&htim1);
   HAL_GPIO_DeInit(GPIOE, GPIO_PIN_3 | LCD_CS_Pin | LCD_WR_RS_Pin);
 
   /* --- 6) Point core at application vector table ----------------------- */
   SCB->VTOR = appBaseAddress;
   __set_MSP(initialSp);
 
   __DSB();
   __ISB();
 
   /* --- 7) Jump to application Reset_Handler ---------------------------- */
   /* Reset_Handler will: set SP, SystemInit, copy .data, zero .bss, main(). */
   /* PRIMASK stays 1 here — app should call __enable_irq() after HAL_Init(). */
   appReset();
 
   /* Not reached */
   return QSPI_APP_JUMP_ERROR;
 }
