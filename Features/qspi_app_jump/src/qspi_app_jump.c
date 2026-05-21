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
#include "stm32h7xx_hal.h"

/* Cortex-M vector table: word0 = initial SP, word1 = Reset_Handler -------- */
#define QSPI_APP_JUMP_VECTOR_SP_OFFSET (0U)
#define QSPI_APP_JUMP_VECTOR_PC_OFFSET (4U)

/* Thumb mode: reset vector must have bit0 set -------------------------------- */
#define QSPI_APP_JUMP_THUMB_BIT_MASK   (1UL)

typedef void (*QSPI_App_Jump_ResetHandlerTypeDef)(void);

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

qspi_app_status_t qspi_app_jump_to_application(uint32_t appBaseAddress)
{
  const uint32_t *vectors = (const uint32_t *)appBaseAddress;
  uint32_t initialSp;
  uint32_t resetHandler;
  QSPI_App_Jump_ResetHandlerTypeDef appReset;

  /* Reject jump if the vector table at appBaseAddress does not look like a valid app. */
  if (!qspi_app_is_valid(appBaseAddress))
  {
    return QSPI_APP_JUMP_INVALID_VECTOR;
  }

  /* Read initial stack pointer and reset handler from the application vector table. */
  initialSp = vectors[QSPI_APP_JUMP_VECTOR_SP_OFFSET / sizeof(uint32_t)];
  resetHandler = vectors[QSPI_APP_JUMP_VECTOR_PC_OFFSET / sizeof(uint32_t)];
  appReset = (QSPI_App_Jump_ResetHandlerTypeDef)resetHandler;

  /* Stop all interrupts before changing core state for the handoff. */
  __disable_irq();

  /* Stop and clear SysTick so the app does not inherit bootloader tick timing. */
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL  = 0U;
  NVIC_ClearPendingIRQ(SysTick_IRQn);

  /* Disable every NVIC line and clear pending IRQs left by bootloader drivers. */
  SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
  for (uint32_t i = 0U; i < 8U; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFFUL;
    NVIC->ICPR[i] = 0xFFFFFFFFUL;
  }
  SCB->ICSR |= (SCB_ICSR_PENDSVCLR_Msk | SCB_ICSR_PENDSTCLR_Msk);

  /* Point the core at the app vector table and load the application stack pointer. */
  SCB->VTOR = appBaseAddress;
  __set_MSP(initialSp);

  /* VTOR/MSP visible before handoff; leave IRQs masked (PRIMASK=1 from __disable_irq). */
  __DSB();
  __ISB();

  /* Transfer control to the application Reset_Handler (does not return).
   * Do not __enable_irq() here: an IRQ before app init would use the new VTOR
   * while handlers/peripherals are not ready. The app enables IRQs after init. */
  appReset();

  /* Not reached */
  return QSPI_APP_JUMP_ERROR;
}
