# New Header File Instruction

When a new header (`.h`) file is created, it MUST start with the standard file
banner, then the file name, then the rest of the header guard / include block,
following the existing style in `Peripherals/MPU6050/Inc/mpu6050.h`.

## 1. File banner (top of every header)

Always begin the file with this Doxygen banner block. Update the `@file` and
`@brief` lines to match the new file, keep the rest of the layout identical:

```c
/**
  ******************************************************************************
  * @file    <filename>.h
  * @brief   <One-line summary of what this header provides.>
  *          <Optional continuation line for the brief.>
  *
  * <Transport / ownership notes, e.g. which bus, who owns the HAL handle.>
  *
  * <Scope note: what is and is not provided in this header.>
  ******************************************************************************
  */
```

- The `@file` value MUST be exactly the file's own name (e.g. `bmp280.h`).
- Keep the alignment: `@file`, `@brief`, etc. line up with the existing files.
- Wrapped `@brief` continuation lines are indented to line up under the text.

## 2. File name

The `@file` field is the first thing to set, and it must match the real file
name exactly (case sensitive). If the file is renamed, update `@file` too.

## 3. Header guard and includes (the rest of the format)

Directly after the banner, add a blank line, then the include guard, includes,
and the C++ `extern "C"` guard, matching this layout:

```c

#ifndef <FILENAME>_H
#define <FILENAME>_H

#include <stdint.h>
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ... definitions / prototypes ... */

#ifdef __cplusplus
}
#endif

#endif /* <FILENAME>_H */
```

- The guard macro is the file name in UPPER_CASE with `_H` (e.g. `MPU6050_H`).
- Group related defines under aligned section banners, e.g.
  `/* ====================== I2C slave address ====================== */`.
