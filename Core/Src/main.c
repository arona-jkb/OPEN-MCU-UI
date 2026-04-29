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
#include "ux_move.h"
#include "Key.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    anim_ctrl_t *anim;
    int16_t sx, sy, ex, ey;
    uint32_t duration_ms;
    void (*easing)(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);
}
anim_start_FUNC_PARAM_sequence_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Int_To_String(int value, char* buffer, int buffer_size)
{
    snprintf(buffer, buffer_size, "%d", value);
}
void anim_START_choose(anim_start_FUNC_PARAM_sequence_t param[], uint8_t size, uint8_t index)
{
    if (index >= size) return;
    anim_start(param[index].anim, param[index].sx, param[index].sy, param[index].ex, param[index].ey,
       param[index].duration_ms, param[index].easing);
}
void anim_back_choose(anim_start_FUNC_PARAM_sequence_t param[], uint8_t size, uint8_t index)
{
    if (index >= size) return;
    anim_back(param[index].anim);
}
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
u8g2_t u8g2;
u8g2Init(&u8g2);

anim_ctrl_t anim1;
anim_ctrl_t anim2;        
anim_ctrl_t anim3;
anim_init(&anim1);
anim_init(&anim2);
anim_init(&anim3);
anim_start_FUNC_PARAM_sequence_t anim_funcs[] = {
    {&anim1, 40, 16, 20, 16, 500, quad_ease_out},
    {&anim2, 40, 32, 20, 32, 500, quad_ease_out},
    {&anim3, 40, 48, 20, 48, 500, quad_ease_out}
};

// anim_start(&anim1, 100, 16, 40, 16, 500, quad_ease_out);
// anim_start(&anim2, 100, 32, 40, 32, 500, quad_ease_out);
// anim_start(&anim3, 100, 48, 40, 48, 500, quad_ease_out);
anim_set_position(&anim1,40,16);
anim_set_position(&anim2,40,32);
anim_set_position(&anim3,40,48);
uint8_t key_flag = 0;
anim_START_choose(anim_funcs, 3, key_flag);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      anim_manager_update();
      int8_t key = Key();
      
      if(key==1)
      {
        uint8_t last_key = key_flag;
        key_flag ++;
        key_flag=key_flag%3;
        anim_START_choose(anim_funcs, 3, key_flag);
        anim_back_choose(anim_funcs, 3, last_key);
      }
      char key_str[10];
      Int_To_String(key_flag, key_str, sizeof(key_str));

      u8g2_FirstPage(&u8g2);
       do
       {
        u8g2_SetFontDirection(&u8g2,0);//确定字体方向
        u8g2_SetFont(&u8g2,u8g2_font_fub11_tf);//设置字体
        u8g2_SetDrawColor(&u8g2,2);//设置绘制颜色
        u8g2_DrawStr(&u8g2, anim1.cur_x, anim1.cur_y, "STM32");
        u8g2_DrawStr(&u8g2, anim2.cur_x, anim2.cur_y, "test");
        u8g2_DrawStr(&u8g2, anim3.cur_x, anim3.cur_y, "key");

        u8g2_DrawStr(&u8g2, 110, 16, key_str); 
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
