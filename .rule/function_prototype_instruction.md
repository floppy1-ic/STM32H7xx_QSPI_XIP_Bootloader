# Function Prototype Instruction

When the user wants to create a new function prototype, write a 3-line Doxygen
comment describing the function, followed by `@param` lines (only if the
function takes parameters) and a `@retval`/`@return` line for the return type.
Follow the style used in `Peripherals/MPU6050/Inc/mpu6050.h`.

## Format

```c
/**
 * @brief  <One short sentence: what the function does.>
 *         <Second line continuing the description if needed.>
 *         <Third line for any extra behavior / side effect.>
 * @param  <name>  <What the parameter is / allowed values.>
 * @retval <Return value meaning, e.g. HAL_OK on success, HAL error otherwise.>
 */
<return_type> <function_name>(<parameters>);
```

## Rules

- The comment is roughly **3 lines** of description after `@brief` (keep it
  short and aligned, like the examples below).
- Add one `@param` line per parameter, aligned with the others. If the function
  has **no parameters**, omit the `@param` lines entirely.
- Always document the return with `@retval` (or `@return`), describing what the
  value means.
- Place the prototype immediately after its comment block.

## Examples (from `mpu6050.h`)

No-parameter function:

```c
/**
 * @brief  Wake the MPU6050 and select a stable clock source.
 *         Clears the SLEEP bit in PWR_MGMT_1 and selects the X-gyro PLL clock.
 * @retval HAL_OK on success, HAL error code otherwise.
 */
HAL_StatusTypeDef mpu_6050_init(void);
```

Function with parameters:

```c
/**
 * @brief  Configure accelerometer and gyroscope full-scale ranges.
 * @param  accel_range  One of mpu6050_accel_range_t (LOW/MID/HIGH/MAX or exact).
 * @param  gyro_range   One of mpu6050_gyro_range_t  (LOW/MID/HIGH/MAX or exact).
 * @retval HAL_OK on success, HAL error code otherwise.
 */
HAL_StatusTypeDef mpu_6050_config(mpu6050_accel_range_t accel_range,
                                  mpu6050_gyro_range_t gyro_range);
```
