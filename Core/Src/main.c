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
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32_u8g2.h"
#include "u8g2.h"
#include "menu.h"
#include "Key.h"
#include <stdint.h>
#include <stdio.h>

static void test_action(void);
static void show_info_action(void);
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
/* --- submenu pages --- */
static const menu_item_t settings_items[] = {
    {"Brightness", NULL, NULL},
    {"Contrast",   NULL, NULL},
    {"Reset",      NULL, NULL},
};

static menu_page_t settings_page = {
    .title  = "Settings",
    .items  = settings_items,
    .count  = sizeof(settings_items) / sizeof(settings_items[0]),
    .parent = NULL,  /* set at runtime */
};

static const menu_item_t display_items[] = {
    {"Flip 180", NULL, NULL},
    {"Invert",   NULL, NULL},
};

static menu_page_t display_page = {
    .title  = "Display",
    .items  = display_items,
    .count  = sizeof(display_items) / sizeof(display_items[0]),
    .parent = NULL,
};

static const menu_item_t about_items[] = {
    {"STM32F103 GUI", NULL, NULL},
    {"u8g2 + SSD1306", NULL, NULL},
    {"v1.0  2026-05", NULL, NULL},
};

static menu_page_t about_page = {
    .title  = "About",
    .items  = about_items,
    .count  = sizeof(about_items) / sizeof(about_items[0]),
    .parent = NULL,
};

/* --- root menu --- */
static menu_item_t root_items[] = {
    {"Test Animation", test_action,      NULL},
    {"Show Info",      show_info_action, NULL},
    {"Settings",       NULL,             &settings_page},
    {"Display",        NULL,             &display_page},
    {"About",          NULL,             &about_page},
};

static menu_page_t root_page = {
    .title  = "Main Menu",
    .items  = root_items,
    .count  = sizeof(root_items) / sizeof(root_items[0]),
    .parent = NULL,
};

/* --- global menu state --- */
static menu_state_t menu_state;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void test_action(void);
static void show_info_action(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1);
  u8g2_t u8g2;
  u8g2Init(&u8g2);

  /* wire parent pointers */
  root_page.parent    = NULL;
  settings_page.parent = &root_page;
  display_page.parent  = &root_page;
  about_page.parent    = &root_page;

  menu_init(&menu_state, &root_page);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      anim_manager_update();

      int8_t key = Key();
      if (key == 1)      menu_key_up(&menu_state);
      else if (key == 2) menu_key_down(&menu_state);
      else if (key == 3) menu_key_enter(&menu_state);
      else if (key == 4) menu_key_back(&menu_state);

      u8g2_FirstPage(&u8g2);
      do {
          menu_render(&u8g2, &menu_state);
      } while (u8g2_NextPage(&u8g2));
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
static void test_action(void) {
    /* placeholder — wired to root item "Test Animation" */
    __NOP();
}

static void show_info_action(void) {
    /* placeholder — wired to root item "Show Info" */
    __NOP();
}
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
