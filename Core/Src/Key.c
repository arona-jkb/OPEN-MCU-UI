// 按键驱动文件，在使用时请使用CubeMX生成与PA3，PA4，PA5，PA6引脚的初始化
#include "Key.h"
#include "main.h"

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

/**
  * @brief  获取当前按键的状态，无消抖及松手检测
  * @param  无
  * @retval 按下按键的键码，范围：0,1~4,0表示无按键按下
  */
unsigned char Key_GetState()
{
	unsigned char KeyNumber=0;
	
	if(HAL_GPIO_ReadPin(user_key_GPIO_Port, user_key_Pin)==GPIO_PIN_RESET){KeyNumber=4;}
	if(HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin)==GPIO_PIN_RESET){KeyNumber=1;}
	if(HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin)==GPIO_PIN_RESET){KeyNumber=2;}
	if(HAL_GPIO_ReadPin(key3_GPIO_Port, key3_Pin)==GPIO_PIN_RESET){KeyNumber=3;}
	return KeyNumber;
}

/**
  * @brief  按键驱动函数，在中断中及其类似物中调用，建议每20ms调用一次
  * @param  无
  * @retval 无
  */
void Key_Tick(void)
{
	static unsigned char NowState,LastState;
	LastState=NowState;				//按键状态更新
	NowState=Key_GetState();		//获取当前按键状态
	//如果上个时间点按键按下，这个时间点未按下，则是松手瞬间，以此避免消抖和松手检测
	if(LastState==1 && NowState==0)
	{
		Key_KeyNumber=1;
	}
	if(LastState==2 && NowState==0)
	{
		Key_KeyNumber=2;
	}
	if(LastState==3 && NowState==0)
	{
		Key_KeyNumber=3;
	}
	if(LastState==4 && NowState==0)
	{
		Key_KeyNumber=4;
	}
}
