#include "lcd_mcal.h"
#include "lcd.h"
#include "main.h"

/* Matches TIM1 ARR=65535; LCD_SetBrightness treats values >100 as raw compare. */
#define LCD_BL_MAX_COMPARE 65535U

static uint8_t s_text_level = 2U;

static uint32_t s_screen_w;
static uint32_t s_screen_h;

static uint16_t s_bg = BLACK;
static uint16_t s_fg = WHITE;

static uint8_t s_size_mode = 2;
static uint8_t s_font_px = 16;
static uint8_t s_char_w = 8;
static uint8_t s_char_h = 16;

static uint16_t s_cur_x;
static uint16_t s_cur_y;

static uint16_t s_dyn_x;
static uint16_t s_dyn_y;
static uint8_t s_dyn_armed;

static uint8_t s_bl_percent = 100;

static void mcal_refresh_metrics(void)
{
  if (s_size_mode == 1)
  {
    s_font_px = 12;
  }
  else
  {
    s_size_mode = 2;
    s_font_px = 16;
  }

  s_char_w = (uint8_t)(s_font_px / 2);
  s_char_h = s_font_px;
}

static void mcal_wrap_if_needed(void)
{
  if (s_screen_w == 0 || s_screen_h == 0)
  {
    return;
  }

  if ((uint32_t)s_cur_x + (uint32_t)s_char_w > s_screen_w)
  {
    s_cur_x = 0;
    s_cur_y = (uint16_t)(s_cur_y + s_char_h);
  }

  if ((uint32_t)s_cur_y + (uint32_t)s_char_h > s_screen_h)
  {
    s_cur_y = 0;
  }
}

static void mcal_advance_cursor(void)
{
  s_cur_x = (uint16_t)(s_cur_x + s_char_w);
  mcal_wrap_if_needed();
}

uint16_t lcd_mcal_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint16_t)((((uint16_t)(r & 0xF8U)) << 8) | (((uint16_t)(g & 0xFCU)) << 3) |
                    (((uint16_t)(b & 0xF8U)) >> 3));
}

void lcd_bg_fg(uint16_t bg, uint16_t fg)
{
  s_bg = bg;
  s_fg = fg;

  BACK_COLOR = s_bg;
  POINT_COLOR = s_fg;
}

void lcd_size(uint8_t size)
{
  if (size == 1)
  {
    s_size_mode = 1;
  }
  else
  {
    s_size_mode = 2;
  }
  mcal_refresh_metrics();
}

void lcd_backlight(uint8_t percent)
{
  if (percent > 100)
  {
    percent = 100;
  }

  s_bl_percent = percent;

  uint32_t cmp = ((uint32_t)percent * (uint32_t)LCD_BL_MAX_COMPARE) / 100U;

  LCD_SetBrightness(cmp);
}

void lcd_breath_in(void)
{
  for (uint8_t p = 0; p <= 100; p += 2)
  {
    lcd_backlight(p);
    HAL_Delay(12);
  }
}

void lcd_breath_out(void)
{
  int p = (int)s_bl_percent;
  for (; p >= 0; p -= 2)
  {
    lcd_backlight((uint8_t)p);
    HAL_Delay(12);
  }
}

void lcd_clear(void)
{
  if (s_screen_w == 0 || s_screen_h == 0)
  {
    return;
  }

  ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 0, s_screen_w, s_screen_h, s_bg);

  s_cur_x = 0;
  s_cur_y = 0;
  s_dyn_armed = 0U;
}

void lcd_print_char(char c)
{
  if (s_screen_w == 0 || s_screen_h == 0)
  {
    return;
  }

  if (c == '\r')
  {
    return;
  }

  if (c == '\n')
  {
    s_cur_x = 0;
    s_cur_y = (uint16_t)(s_cur_y + s_char_h);
    mcal_wrap_if_needed();
    return;
  }

  mcal_wrap_if_needed();
  LCD_ShowChar(s_cur_x, s_cur_y, (uint8_t)c, s_font_px, 0);
  mcal_advance_cursor();
}

void lcd_print_message(const char *msg)
{
  if (msg == NULL)
  {
    return;
  }

  while (*msg != '\0')
  {
    lcd_print_char(*msg);
    msg++;
  }
}

void lcd_init(void)
{
  s_cur_x = 0;
  s_cur_y = 0;
  s_size_mode = 2;
  mcal_refresh_metrics();
  lcd_bg_fg(BLACK, WHITE);

#if defined(TFT96)
  ST7735Ctx.Orientation = ST7735_ORIENTATION_LANDSCAPE_ROT180;
  ST7735Ctx.Panel = HannStar_Panel;
  ST7735Ctx.Type = ST7735_0_9_inch_screen;
#elif defined(TFT18)
  ST7735Ctx.Orientation = ST7735_ORIENTATION_PORTRAIT;
  ST7735Ctx.Panel = BOE_Panel;
  ST7735Ctx.Type = ST7735_1_8a_inch_screen;
#else
#error "Unknown Screen: define TFT96 or TFT18"
#endif

  ST7735_RegisterBusIO(&st7735_pObj, &st7735_pIO);
  ST7735_LCD_Driver.Init(&st7735_pObj, ST7735_FORMAT_RBG565, &ST7735Ctx);
  ST7735_LCD_Driver.ReadID(&st7735_pObj, &st7735_id);

  s_screen_w = ST7735Ctx.Width;
  s_screen_h = ST7735Ctx.Height;

  lcd_clear();
  lcd_backlight(100);
}

uint16_t lcd_mcal_color565(lcd_stm32h7_color_t color)
{
  /* Integer cases: lcd.h macros (RED, MAG, GRE, …) must not be used as case labels. */
  switch ((unsigned int)color)
  {
    case 0U:
      return BLACK;
    case 1U:
      return WHITE;
    case 2U:
      return RED;
    case 3U:
      return GREEN;
    case 4U:
      return BLUE;
    case 5U:
      return YELLOW;
    case 6U:
      return lcd_mcal_rgb888_to_rgb565(255U, 140U, 40U);
    case 7U:
      return CYAN;
    case 8U:
      return MAGENTA;
    case 9U:
      return BROWN;
    case 10U:
      return BRRED;
    case 11U:
      return GRAY;
    case 12U:
      return DARKBLUE;
    case 13U:
      return LIGHTBLUE;
    case 14U:
      return GRAYBLUE;
    case 15U:
      return DARKGRAY;
    case 16U:
      return LIGHTGRAY;
    case 17U:
      return NAVY;
    case 18U:
      return GOLD;
    case 19U:
      return PURPLE;
    case 20U:
      return PINK;
    case 21U:
      return TEAL;
    case 22U:
      return OLIVE;
    case 23U:
      return MAROON;
    case 24U:
      return LIME;
    default:
      return WHITE;
  }
}

void lcd_stm32h7_backlight(uint32_t percent)
{
  if (percent > 100U)
  {
    percent = 100U;
  }
  lcd_backlight((uint8_t)percent);
}

void lcd_stm32h7_breathin(void)
{
  lcd_breath_in();
}

void lcd_stm32h7_breathout(void)
{
  lcd_breath_out();
}

void lcd_stm32h7_color(lcd_stm32h7_color_t bg, lcd_stm32h7_color_t fg)
{
  lcd_bg_fg(lcd_mcal_color565(bg), lcd_mcal_color565(fg));
  lcd_clear();
}

uint8_t lcd_stm32h7_size(uint8_t level)
{
  if ((level >= 1U) && (level <= 3U))
  {
    s_text_level = level;
    lcd_size((level == 1U) ? 1U : 2U);
  }

  return (s_text_level == 1U) ? 12U : 16U;
}

void lcd_stm32h7_char(char ch)
{
  if ((ch == '\r') || (ch == '\n'))
  {
    lcd_print_char('\n');
    return;
  }
  lcd_print_char(ch);
}

void lcd_stm32h7_message(const char *msg)
{
  lcd_print_message(msg);
}

void lcd_stm32h7_dynamic_update(const char *msg)
{
  const char *p;
  uint32_t nchars;
  uint32_t w;

  if ((msg == NULL) || (s_screen_w == 0U) || (s_screen_h == 0U))
  {
    return;
  }

  if ((s_dyn_armed == 0U) || (s_cur_y != s_dyn_y))
  {
    s_dyn_x = s_cur_x;
    s_dyn_y = s_cur_y;
    s_dyn_armed = 1U;
  }

  if (s_dyn_x >= s_screen_w)
  {
    return;
  }

  w = s_screen_w - (uint32_t)s_dyn_x;
  ST7735_LCD_Driver.FillRect(&st7735_pObj, s_dyn_x, s_dyn_y, w, (uint32_t)s_char_h, s_bg);
  (void)LCD_ShowString(s_dyn_x, s_dyn_y, (uint16_t)w, s_char_h, s_font_px, (uint8_t *)msg);

  nchars = 0U;
  for (p = msg; *p != '\0'; p++)
  {
    nchars++;
  }
  s_cur_x = (uint16_t)(s_dyn_x + (uint16_t)(nchars * (uint32_t)s_char_w));
  if (s_cur_x >= s_screen_w)
  {
    s_cur_x = (uint16_t)((s_screen_w > (uint32_t)s_char_w) ? (s_screen_w - (uint32_t)s_char_w) : 0U);
  }
  s_cur_y = s_dyn_y;
}

void lcd_stm32h7_clear(void)
{
  lcd_clear();
}

void lcd_stm32h7_init(void)
{
  s_text_level = 2U;
  lcd_init();
}
