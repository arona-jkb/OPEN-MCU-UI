/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32_u8g2.h"
#include "u8g2.h"
#include "app_ui.h"
#include "Key.h"

/* forward declarations — must precede PV menu definitions */
static void test_action(void);
static void show_info_action(void);
static void brightness_action(void);
static void power_action(void);
static void reset_action(void);
static void custom_screen1_action(void);
static void custom_screen2_action(void);
static void my_custom_render(u8g2_t *u8g2, uint8_t id);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* demo variables referenced by action callbacks */
static int16_t demo_brightness = 50;
static bool    demo_power = true;

/* ---- 24x24 全白测试图标 (共用同一份位图数据) ---- */
#define ICON_W  24
#define ICON_H  24

static const uint8_t icon_white_bits[] U8X8_PROGMEM = {
    0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,
};

/* ---- 页面前向声明 (供 parent 指针交叉引用) ---- */
static menu_page_t settings_page;
static menu_page_t display_page;
static menu_page_t about_page;
static menu_page_t icon_page;
static menu_page_t root_page;

/* ---- 子菜单页 (使用宏, parent 直接写入) ---- */
static menu_page_t settings_page =
    MENU_PAGE_TEXT("Settings", &root_page,
        { "Brightness Adjustment Level", {0}, brightness_action, NULL },
        { "Power Save Mode Config",      {0}, power_action,      NULL },
        { "Reset",                       {0}, reset_action,      NULL },
    );

static menu_page_t display_page =
    MENU_PAGE_TEXT("Display", &root_page,
        { "Flip Display 180 Degrees",    {0}, NULL, NULL },
        { "Invert Display Color Mode",   {0}, NULL, NULL },
    );

static menu_page_t about_page =
    MENU_PAGE_TEXT("About", &root_page,
        { "STM32F103 GUI Demo Project v3.0",          {0}, NULL, NULL },
        { "u8g2 Library + SSD1306 OLED Display",      {0}, NULL, NULL },
        { "Build Date: 2026-05-30",                   {0}, NULL, NULL },
    );

/* ---- 图标菜单页 (水平排列, 全白测试位图) ---- */
static menu_page_t icon_page =
    MENU_PAGE_ICON("Icon Menu", &root_page,
        { "Home",     {icon_white_bits, ICON_W, ICON_H}, NULL, NULL },
        { "Search",   {icon_white_bits, ICON_W, ICON_H}, NULL, NULL },
        { "Settings", {icon_white_bits, ICON_W, ICON_H}, NULL, NULL },
        { "Tools",    {icon_white_bits, ICON_W, ICON_H}, NULL, NULL },
        { "About",    {icon_white_bits, ICON_W, ICON_H}, NULL, NULL },
    );

/* ---- 根菜单 ---- */
static menu_page_t root_page =
    MENU_PAGE_TEXT("Main Menu", NULL,
        { "Custom Screen 1 - Long Name Test", {0}, custom_screen1_action, NULL },
        { "Custom Screen 2",                  {0}, custom_screen2_action, NULL },
        { "Test Animation Effects",           {0}, test_action,           NULL },
        { "Show System Information Panel",    {0}, show_info_action,      NULL },
        { "Icon Menu",                        {0}, NULL,                 &icon_page },
        { "Settings",                         {0}, NULL,                 &settings_page },
        { "Display",                          {0}, NULL,                 &display_page },
        { "About",                            {0}, NULL,                 &about_page },
    );
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void test_action(void)            { __NOP(); }
static void show_info_action(void)       { __NOP(); }

static void brightness_action(void) {
    app_ui_value_open("Brightness", &demo_brightness, 0, 100, 5);
}

static void power_action(void) {
    app_ui_toggle_open("Power Save", &demo_power, "ON", "OFF");
}

static void reset_action(void) {
    app_ui_toast_show("Settings cleared");
}

static void custom_screen1_action(void) { app_ui_custom_screen_enter(1); }
static void custom_screen2_action(void) { app_ui_custom_screen_enter(2); }

static void my_custom_render(u8g2_t *u8g2, uint8_t id) {
    u8g2_SetFontMode(u8g2, 1);
    if (id == 1) {
        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, 10, 25, "Custom Screen 1");
        u8g2_DrawHLine(u8g2, 10, 30, 108);
        u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
        u8g2_DrawStr(u8g2, 10, 45, "Press BACK to return.");
    } else {
        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, 10, 20, "Custom Screen 2");
        u8g2_DrawHLine(u8g2, 10, 26, 108);
        u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
        u8g2_DrawStr(u8g2, 10, 40, "Another screen.");
        u8g2_DrawStr(u8g2, 10, 52, "Back key exits.");
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  u8g2_t u8g2;
  MD_OLED_RST_Set();
  u8g2Init(&u8g2);

  app_ui_init(&u8g2, &root_page);
  app_ui_set_custom_render(my_custom_render);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      anim_manager_update();

      int8_t key = Key();
      app_ui_update(key);
      app_ui_render(&u8g2);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
