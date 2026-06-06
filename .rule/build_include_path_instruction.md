# Build Include Path Instruction

**Agents and contributors:** When you add or change a user module (`Inc/` + `Src/`), you
**must** apply every step in this document **and** update the
[Registered modules](#registered-modules-in-this-repo) section at the bottom so the rule
stays accurate after CubeMX regeneration.

Related: `new_file_instruction.md` (header layout).

---

## 1. Folder layout convention

Use the same layout as existing modules:

```text
<Area>/<module_name>/
  Inc/     <- public headers (e.g. mpu_6050_angle.h)
  Src/     <- source files (e.g. mpu_6050_angle.c)
```

Examples:

- `Peripherals/MPU6050/Inc`, `Peripherals/MPU6050/Src`
- `Physics/imu_mpu_angle/Inc`, `Physics/imu_mpu_angle/Src`

The include path added to the build is always the **`Inc`** directory:

```text
-I../<Area>/<module_name>/Inc
```

Example: `-I../Physics/imu_mpu_angle/Inc`

---

## 2. Register a **new** module in the build (four steps)

Replace `<Area>`, `<module>`, and paths below with your module.

### Step A — `Debug/sources.mk`

Add the `Src` folder to `SUBDIRS` (one line, keep alphabetical or area grouping):

```makefile
SUBDIRS := \
...
Physics/imu_mpu_angle/Src \
...
```

### Step B — `Debug/makefile`

Add an `-include` for the module’s `subdir.mk` (with the other `-include` lines, after
`sources.mk`):

```makefile
-include sources.mk
-include Physics/imu_mpu_angle/Src/subdir.mk
...
```

### Step C — Create `Debug/<Area>/<module>/Src/subdir.mk`

Copy an existing `subdir.mk` (e.g. `Debug/Peripherals/USART1/Src/subdir.mk`) and update:

- `C_SRCS`, `OBJS`, `C_DEPS` — paths to your `.c` file
- Compile rule — full `-I` list must include **your** `-I../<Area>/<module>/Inc`
- `%.c` rule path prefix and `clean-*` target names

### Step D — `Debug/objects.list` (if link fails with undefined references)

If the IDE linker step uses `objects.list` and your `.o` is missing, add a line:

```text
"./Physics/imu_mpu_angle/Src/mpu_6050_angle.o"
```

A normal `make all` from `Debug/` often rebuilds this from `subdir.mk`; re-check after
CubeMX regen if you see `undefined reference to imu_mpu_*`.

---

## 3. Update include paths in **all** `subdir.mk` files

**Critical:** Each `Debug/**/subdir.mk` has its **own** copy of the `-I` list on the
`arm-none-eabi-gcc` line. Adding `-I` only in the new module’s `subdir.mk` is **not
enough** when `Core/Src/main.c` (or any other unit) does `#include "your_header.h"`.

### Search anchor (insert new path here)

Find this fragment (no Physics path = broken build for headers under `Physics/`):

```text
-I../Features/app_shared_ram/inc -I../Peripherals
```

Insert **between** `app_shared_ram/inc` and `Peripherals`:

```text
-I../Features/app_shared_ram/inc -I../Physics/imu_mpu_angle/Inc -I../Peripherals
```

For a **new** module `Foo/bar`, use `-I../Foo/bar/Inc` in the same position (after
`app_shared_ram`, before `Peripherals`).

### Files that must carry the shared `-I` block

Update **every** file below when you add a module whose headers can be included from
`main.c` or cross-module code. See
[Registered modules](#registered-modules-in-this-repo) for the current `-I` list.

| File |
|------|
| `Debug/Core/Src/subdir.mk` |
| `Debug/Core/Startup/subdir.mk` (if it compiles C; asm-only may omit) |
| `Debug/Features/app_shared_ram/src/subdir.mk` |
| `Debug/Physics/imu_mpu_angle/Src/subdir.mk` |
| `Debug/Peripherals/BMP280/Src/subdir.mk` (if present in tree) |
| `Debug/Peripherals/MPU6050/Src/subdir.mk` |
| `Debug/Peripherals/QSPI_Flash/Src/subdir.mk` |
| `Debug/Peripherals/SPI4_LCD/Src/subdir.mk` |
| `Debug/Peripherals/USART1/Src/subdir.mk` |
| `Debug/Drivers/STM32H7xx_HAL_Driver/Src/subdir.mk` |

After adding a module, run the search again and patch **all** matches — not only
`Debug/Core/Src/subdir.mk`.

---

## 4. After STM32CubeMX / CubeIDE code generation

`Debug/sources.mk`, `Debug/makefile`, `Debug/**/subdir.mk`, and sometimes
`Debug/objects.list` are marked *Automatically-generated*. Regeneration often **removes**
custom `SUBDIRS`, `-include` lines, and `-I../Physics/...` entries.

### Recovery checklist (run in order)

1. **`Debug/sources.mk`** — confirm each custom `SUBDIRS` line (see registry below).
2. **`Debug/makefile`** — confirm each `-include <Area>/<module>/Src/subdir.mk`.
3. **`Debug/**/subdir.mk`** — grep for `app_shared_ram/inc -I../Peripherals` (missing
   Physics/other paths) and re-insert every required `-I../<Area>/<module>/Inc`.
4. **`Debug/objects.list`** — confirm each custom `.o` path if link errors appear.
5. **Rebuild** from `Debug/` and confirm compile line for `main.c` contains your `-I`.

### Verify with grep (repo root)

```bash
# Every subdir.mk that compiles C should list Physics (example)
grep -l "Physics/imu_mpu_angle/Inc" Debug/**/subdir.mk

# Broken: shared block without Physics between app_shared_ram and Peripherals
grep "app_shared_ram/inc -I../Peripherals" Debug/**/subdir.mk
```

The second command should return **no** files once paths are correct.

### IDE path (survives regen better)

**Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths**

Add: `../Physics/imu_mpu_angle/Inc` (and each new module’s `Inc` folder).

Still re-check Steps A–C after regen; IDE settings do not always restore `SUBDIRS` or
`-include`.

---

## 5. Quick checklist (new module)

| Step | Action |
|------|--------|
| 1 | Create `<Area>/<module>/Inc/*.h` and `Src/*.c` (`new_file_instruction.md`) |
| 2 | Add `SUBDIRS` entry in `Debug/sources.mk` |
| 3 | Add `-include .../subdir.mk` in `Debug/makefile` |
| 4 | Create `Debug/.../Src/subdir.mk` for the module |
| 5 | Add `-I../<Area>/<module>/Inc` to **all** `Debug/**/subdir.mk` compile lines (§3) |
| 6 | Add `.o` to `Debug/objects.list` if linker omits the object |
| 7 | **Update [Registered modules](#registered-modules-in-this-repo) in this file** |
| 8 | Rebuild; fix `No such file or directory` → step 5; fix `undefined reference` → 2–4, 6 |

---

## 6. Typical errors

### `fatal error: xxx.h: No such file or directory`

**Example:**

```text
fatal error: mpu_6050_angle.h: No such file or directory
   28 | #include "mpu_6050_angle.h"
```

**Cause:** `#include` in `main.c` (or another unit), but that unit’s `subdir.mk` is
missing `-I../Physics/imu_mpu_angle/Inc` (often `Debug/Core/Src/subdir.mk` after regen).

**Fix:** Re-apply §3 to all `subdir.mk` files; confirm with grep in §4.

### `undefined reference to imu_mpu_init` (or similar)

**Cause:** Header found but `.c` not compiled — missing `SUBDIRS`, `-include`, or
`objects.list` entry.

**Fix:** §2 Steps A–D.

---

## Registered modules in this repo

**Maintain this table** whenever you add, rename, or remove a user module. CubeMX regen
must be restored to match these rows.

| Area / module | Source tree | Include flag (`-I`) | `Debug/sources.mk` SUBDIRS | `Debug/makefile` `-include` |
|---------------|-------------|-------------------|----------------------------|-------------------------------|
| Features / app_shared_ram | `Features/app_shared_ram/src` | `-I../Features/app_shared_ram/inc` | `Features/app_shared_ram/src` | `Features/app_shared_ram/src/subdir.mk` |
| Physics / imu_mpu_angle | `Physics/imu_mpu_angle/Src/mpu_6050_angle.c` | `-I../Physics/imu_mpu_angle/Inc` | `Physics/imu_mpu_angle/Src` | `Physics/imu_mpu_angle/Src/subdir.mk` |
| Peripherals / MPU6050 | `Peripherals/MPU6050/Src` | `-I../Peripherals/MPU6050/Inc` | `Peripherals/MPU6050/Src` | `Peripherals/MPU6050/Src/subdir.mk` |
| Peripherals / QSPI_Flash | `Peripherals/QSPI_Flash/Src` | `-I../Peripherals/QSPI_Flash/Inc` | `Peripherals/QSPI_Flash/Src` | `Peripherals/QSPI_Flash/Src/subdir.mk` |
| Peripherals / SPI4_LCD | `Peripherals/SPI4_LCD/Src` | `-I../Peripherals/SPI4_LCD/Inc` | `Peripherals/SPI4_LCD/Src` | `Peripherals/SPI4_LCD/Src/subdir.mk` |
| Peripherals / USART1 | `Peripherals/USART1/Src` | `-I../Peripherals/USART1/Inc` | `Peripherals/USART1/Src` | `Peripherals/USART1/Src/subdir.mk` |

**Shared `-I` block** (order on `arm-none-eabi-gcc` line — all C `subdir.mk` files above):

```text
-I../Core/Inc
-I../Features/app_shared_ram/inc
-I../Physics/imu_mpu_angle/Inc
-I../Peripherals/BMP280/Inc
-I../Peripherals/MPU6050/Inc
-I../Peripherals/QSPI_Flash/Inc
-I../Peripherals/SPI4_LCD/Inc
-I../Peripherals/USART1/Inc
-I../Drivers/STM32H7xx_HAL_Driver/Inc
...
```

Note: `Peripherals/BMP280` may still appear in `-I` lists for headers; it is not always
in `SUBDIRS` if that driver is not linked. Do not remove BMP280 `-I` unless you intend to
drop BMP280 includes project-wide.

**`Debug/objects.list` — user objects (add line if missing after regen):**

```text
"./Physics/imu_mpu_angle/Src/mpu_6050_angle.o"
```

When you add a **new** row to the table, also add its `-I` to the shared block in §3 and
document the exact grep anchor if the insert position changes.
