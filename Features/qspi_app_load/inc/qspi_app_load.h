/**
  ******************************************************************************
  * @file    qspi_app_load.h
  * @author  Sibun
  * @brief   Load application binary into external QSPI flash.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#ifndef QSPI_APP_LOAD_H
#define QSPI_APP_LOAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Stub display time until QSPI program path is implemented */
#define QSPI_APP_LOAD_STUB_DISPLAY_MS    (5000U)

/**
  * @brief  Run app-load session (program external flash when implemented).
  * @param  None
  * @retval None (does not return; soft reset after stub or real load).
  */
void qspi_new_app_load(void);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_APP_LOAD_H */
