/**
  ******************************************************************************
  * @file    national_logo.c
  * @brief   Tiranga: saffron / white / green bands + Ashoka Chakra (stylized).
  ******************************************************************************
  */

#include "national_logo.h"
#include "lcd_mcal.h"
#include "lcd.h"

static void nl_rect_abs(uint32_t x, uint32_t y, uint32_t ww, uint32_t hh, uint16_t c)
{
  if ((ww == 0U) || (hh == 0U))
  {
    return;
  }
  ST7735_LCD_Driver.FillRect(&st7735_pObj, x, y, ww, hh, c);
}

static void nl_ring_abs(uint32_t cx, uint32_t cy, int32_t ro, int32_t ri, uint16_t outer, uint16_t inner,
                        uint32_t W_scr, uint32_t H_scr)
{
  for (int32_t dy = -ro; dy <= ro; dy++)
  {
    for (int32_t dx = -ro; dx <= ro; dx++)
    {
      int32_t d2 = dx * dx + dy * dy;
      if (d2 <= (ro * ro))
      {
        int32_t px = (int32_t)cx + dx;
        int32_t py = (int32_t)cy + dy;
        if ((px >= 0) && (py >= 0) && ((uint32_t)px < W_scr) && ((uint32_t)py < H_scr))
        {
          uint16_t col = (d2 <= (ri * ri)) ? inner : outer;
          (void)ST7735_LCD_Driver.SetPixel(&st7735_pObj, (uint32_t)px, (uint32_t)py, col);
        }
      }
    }
  }
}

static void draw_tiranga_at(uint32_t x0, uint32_t y0, uint32_t fw, uint32_t fh)
{
  uint32_t W_scr = ST7735Ctx.Width;
  uint32_t H_scr = ST7735Ctx.Height;

  if ((fw == 0U) || (fh == 0U) || (W_scr == 0U) || (H_scr == 0U))
  {
    return;
  }

  uint16_t saff = lcd_mcal_color565(ORA);
  uint16_t wht = WHITE;
  uint16_t grn = GREEN;
  uint16_t navy = DARKBLUE;

  uint32_t bh = fh / 3U;
  if (bh == 0U)
  {
    bh = 1U;
  }
  uint32_t rem = (fh >= 2U * bh) ? (fh - 2U * bh) : 0U;

  nl_rect_abs(x0, y0, fw, bh, saff);
  nl_rect_abs(x0, y0 + bh, fw, bh, wht);
  nl_rect_abs(x0, y0 + 2U * bh, fw, rem, grn);

  if (bh < 3U)
  {
    return;
  }

  uint32_t cx = x0 + (fw / 2U);
  uint32_t cy = y0 + bh + (bh / 2U);
  uint32_t r_out = (bh * 2U) / 5U;
  if (r_out < 3U)
  {
    r_out = 3U;
  }
  if (r_out > 24U)
  {
    r_out = 24U;
  }
  if (r_out > fw / 2U)
  {
    r_out = fw / 2U;
  }
  if (r_out > bh / 2U)
  {
    r_out = bh / 2U;
  }
  uint32_t r_in = r_out / 3U;
  if (r_in < 2U)
  {
    r_in = 2U;
  }
  nl_ring_abs(cx, cy, (int32_t)r_out, (int32_t)r_in, navy, wht, W_scr, H_scr);
}

static void draw_tiranga_scaled_centered(uint32_t W, uint32_t H, uint32_t scale_permille)
{
  uint32_t fw = (W * scale_permille) / 1000U;
  uint32_t fh = (H * scale_permille) / 1000U;

  if (fw < 2U)
  {
    fw = 2U;
  }
  if (fh < 2U)
  {
    fh = 2U;
  }
  draw_tiranga_at((W - fw) / 2U, (H - fh) / 2U, fw, fh);
}

/**
  * @brief  Grow-from-center + exhale on the framebuffer (optional; not called from national_logo).
  */
#if defined(__GNUC__)
__attribute__((unused))
#endif
static void national_logo_scaled_framebuffer_transition(void)
{
  uint32_t W = ST7735Ctx.Width;
  uint32_t H = ST7735Ctx.Height;

  if ((W == 0U) || (H == 0U))
  {
    return;
  }

  for (uint32_t s = 280U; s <= 1000U; s += 40U)
  {
    ST7735_LCD_Driver.FillRect(&st7735_pObj, 0U, 0U, W, H, BLACK);
    draw_tiranga_scaled_centered(W, H, s);
    HAL_Delay(20U);
  }

  {
    static const uint32_t exhale_permille[] = { 1000U, 940U, 880U, 920U, 960U, 1000U };
    for (uint32_t i = 0U; i < (sizeof(exhale_permille) / sizeof(exhale_permille[0])); i++)
    {
      ST7735_LCD_Driver.FillRect(&st7735_pObj, 0U, 0U, W, H, BLACK);
      draw_tiranga_scaled_centered(W, H, exhale_permille[i]);
      HAL_Delay(28U);
    }
  }

  draw_tiranga_at(0U, 0U, W, H);
}

void national_logo(void)
{
  uint32_t W = ST7735Ctx.Width;
  uint32_t H = ST7735Ctx.Height;

  if ((W == 0U) || (H == 0U))
  {
    return;
  }

  draw_tiranga_at(0U, 0U, W, H);
  lcd_stm32h7_breathin();
  lcd_stm32h7_backlight(100U);
}
