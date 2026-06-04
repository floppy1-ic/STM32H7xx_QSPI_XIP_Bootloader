# Function Definition Instruction

When the user wants to create a function definition, first make sure a prototype
exists. Where the prototype goes depends on whether the function is `static`
(private) or public.

## 1. Check for a prototype first

Before writing the body, check whether a prototype already exists:

- **Public function** -> the prototype must be in the matching header file
  (e.g. a function in `mpu6050.c` is declared in `mpu6050.h`). If it is not
  there, add it (follow `function_prototype_instruction.md`).
- **Static (private) function** -> the prototype must be at the **top of the
  same `.c` file**, not in the header. If it is missing, add it there first.

## 2. Definition style (public functions)

Public function definitions should look like the ones in
`Peripherals/MPU6050/Src/mpu6050.c`: a short one-line comment above the
function, then a readable body that groups declarations and separates logical
steps with single blank lines.

```c
/* ---- Read calculated rotation rate in rad/s or deg/s. ------------------ */
HAL_StatusTypeDef mpu_6050_gyro(mpu6050_axes_t *gyro, mpu6050_gyro_unit_t unit)
{
  int16_t r[3];
  HAL_StatusTypeDef status;
  float sens;
  float scale;

  if (gyro == NULL)
  {
    return HAL_ERROR;
  }

  status = mpu6050_read_axes16(MPU6050_REG_GYRO_XOUT_H, r);

  if (status != HAL_OK)
  {
    return status;
  }

  sens = mpu6050_gyro_sensitivity(s_gyro_range); /* LSB per deg/s */
  scale = 1.0f / sens; /* counts -> deg/s, then optionally -> rad/s */

  if (unit == MPU6050_GYRO_UNIT_RAD)
  {
    scale *= MPU6050_DEG_TO_RAD;
  }

  gyro->x = mpu6050_round2((float)r[0] * scale);
  gyro->y = mpu6050_round2((float)r[1] * scale);
  gyro->z = mpu6050_round2((float)r[2] * scale);

  return HAL_OK;
}
```

## 3. Formatting rules

- **Declarations**: group all local variable declarations together at the top
  of the function with **no blank lines between them**.
- **Blank lines**: leave exactly **one** blank line between logical sections
  (declarations, each statement/operation, conditional blocks, the final
  `return`). Do not cram multiple operations together, and do not stack two or
  more blank lines.
- **Conditionals / blocks**: never use single-line bodies like
  `if (x) { return y; }`. The opening brace goes on its **own line**, the body
  on its **own indented line(s)**, and the closing brace on its own line:

  ```c
  if (gyro == NULL)
  {
    return HAL_ERROR;
  }
  ```

- **Indentation**: 2 spaces per level. No tabs. No trailing whitespace.
- **Comment header**: separate two functions with **one** one-line comment in
  the `/* ---- <short description>. ---- */` style. This comment briefly says
  what the next function does.
- **Prototypes**: a `static` function's prototype goes at the top of the `.c`
  file; a public function's prototype goes in the header.
