// 按键驱动文件，在使用时请使用CubeMX生成与PA3，PA4，PA5，PA6引脚的初始化
#include "Key.h"
#include "main.h"
#include "stdbool.h"

unsigned char Key_KeyNumber;

/**
  * @brief  获取按键键码
  * @param  无
  * @retval 按下按键的键码，范围：0,1~4,0表示无按键按下
  */
unsigned char Key(void)
{
	unsigned char Temp=0;
	Temp=Key_KeyNumber;
	Key_KeyNumber=0;
	return Temp;
}

/* 每个按键独立追踪上一拍状态，避免某脚被外部拉死时阻塞全部按键 */
static bool k1_last, k2_last, k3_last, k4_last;

/**
  * @brief  按键驱动函数，在中断中及其类似物中调用，建议每20ms调用一次
  * @param  无
  * @retval 无
  */
void Key_Tick(void)
{
	bool k1 = (HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin) == GPIO_PIN_RESET);
	bool k2 = (HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin) == GPIO_PIN_RESET);
	bool k3 = (HAL_GPIO_ReadPin(key3_GPIO_Port, key3_Pin) == GPIO_PIN_RESET);
	bool k4 = (HAL_GPIO_ReadPin(key4_GPIO_Port, key4_Pin) == GPIO_PIN_RESET);

	/* 检测松手：上一拍按下 且 当前拍未按下 */
	if (k1_last && !k1) { Key_KeyNumber = 1; }
	if (k2_last && !k2) { Key_KeyNumber = 2; }
	if (k3_last && !k3) { Key_KeyNumber = 3; }
	if (k4_last && !k4) { Key_KeyNumber = 4; }

	k1_last = k1;
	k2_last = k2;
	k3_last = k3;
	k4_last = k4;
}
