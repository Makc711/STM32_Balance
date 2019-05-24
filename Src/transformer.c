/* Includes ------------------------------------------------------------------*/
#include "transformer.h"
#include "memoryMA.h"
#include "tim.h"
#include "stm32_hal_legacy.h"

#define RELAY_SWITCHING_DELAY_MS 10
#define BALANCING_OFF_DELAY_MS 1
#define DIODE_ENABLE_DELAY_MS 1
#define PWM_PERIOD 360

bool isBalancingInEnabled = false;
bool isBalancingOutEnabled = false;
volatile uint8_t countSeconds = 0;
volatile bool noCommandFromController = true;

void setRelayToInput();
void setRelayToOutput();
void disableBalancing();
void disableBalancingOut();
void disableBalancingIn();
void enableBalancingIn();
void enableBalancingOut();

void updateBalancing()
{
	if (((measurements.MA_Event_Register & MA_Event_Safety_Status_MA_Fail_Msk) == MA_Event_Safety_Status_MA_Fail) ||
		(noCommandFromController == true))
	{
		if (((measurements.MA_Event_Register & MA_Event_Balancing_In_Msk) == MA_Event_Balancing_In) ||
			((measurements.MA_Event_Register & MA_Event_Balancing_Out_Msk) == MA_Event_Balancing_Out))
		{
			disableBalancing();
			measurements.MA_Event_Register &= ~(MA_Event_Balancing_In_Msk | MA_Event_Balancing_Out_Msk);
		}
	} else
	{
		if (((measurements.MA_Event_Register & MA_Event_Safety_Status_COT_Msk) == MA_Event_Safety_Status_COT) || 
			((measurements.MA_Event_Register & MA_Event_Safety_Status_OTT_Msk) == MA_Event_Safety_Status_OTT))
		{
			disableBalancing();
		} else
		{
			if ((measurements.MA_Event_Register & MA_Event_Balancing_In_Msk) == MA_Event_Balancing_In)
			{
				enableBalancingIn();
			} else 
			{
				if ((measurements.MA_Event_Register & MA_Event_Balancing_Out_Msk) == MA_Event_Balancing_Out)
				{
					enableBalancingOut();
				} else
				{
					disableBalancing();
				}
			}
		}
	}
}

void setRelayToInput()
{
	HAL_GPIO_WritePin(TrInOut_GPIO_Port, TrInOut_Pin, GPIO_PIN_RESET);
	HAL_Delay(RELAY_SWITCHING_DELAY_MS);
	measurements.MA_Event_Register &= ~MA_Event_Transformer_Out_Msk;
}

void setRelayToOutput()
{
	HAL_GPIO_WritePin(TrInOut_GPIO_Port, TrInOut_Pin, GPIO_PIN_SET);
	HAL_Delay(RELAY_SWITCHING_DELAY_MS);
	measurements.MA_Event_Register |= MA_Event_Transformer_Out_Msk;
}

void disableBalancing()
{
	if ((isBalancingInEnabled == true) || (isBalancingOutEnabled == true))
	{
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
		HAL_Delay(BALANCING_OFF_DELAY_MS);
		setRelayToInput();
		isBalancingInEnabled = false;
		isBalancingOutEnabled = false;
	}
}

void disableBalancingOut()
{
	if (isBalancingOutEnabled == true)
	{
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
		HAL_Delay(BALANCING_OFF_DELAY_MS);
		setRelayToInput();
		isBalancingOutEnabled = false;
	}
}

void disableBalancingIn()
{
	if (isBalancingInEnabled == true)
	{
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
		HAL_Delay(BALANCING_OFF_DELAY_MS);
		isBalancingInEnabled = false;
	}
}

void enableBalancingIn()
{
	disableBalancingOut();
	if ((measurements.MA_Event_Register & MA_Event_Safety_Status_COV_Msk) == MA_Event_Safety_Status_COV)
	{
		disableBalancingIn();
		measurements.MA_Event_Register &= ~MA_Event_Balancing_In_Msk;
	} else
	{
		if (((measurements.MA_Event_Register & MA_Event_Safety_Status_CUT_Msk) == MA_Event_Safety_Status_CUT) ||
			((measurements.MA_Event_Register & MA_Event_BufferEnable_Msk) == MA_Event_BufferEnable)) // (measurements.MA_Event_Register & MA_Event_BufferEnable_Msk) != MA_Event_BufferEnable
			{
				disableBalancingIn();
			} else
		{
			if (isBalancingInEnabled == false)
			{
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_3, PWM_PERIOD - settings.PulseWidthPWM1);
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, settings.PulseWidthPWM2);
				HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
				HAL_Delay(DIODE_ENABLE_DELAY_MS);
				HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
				isBalancingInEnabled = true;
			}
		}
	}
}

void enableBalancingOut()
{
	disableBalancingIn();
	if ((measurements.MA_Event_Register & MA_Event_Safety_Status_CUV_Msk) == MA_Event_Safety_Status_CUV)
	{
		disableBalancingOut();
		measurements.MA_Event_Register &= ~MA_Event_Balancing_Out_Msk;
	} else
	{
		if ((measurements.MA_Event_Register & MA_Event_BufferEnable_Msk) == MA_Event_BufferEnable)
		{
			disableBalancingOut();
		} else
		{
			if (isBalancingOutEnabled == false)
			{
				setRelayToOutput();
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, settings.PulseWidthPWM1);
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_3, PWM_PERIOD - settings.PulseWidthPWM2);
				HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
				HAL_Delay(DIODE_ENABLE_DELAY_MS);
				HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
				isBalancingOutEnabled = true;
			}
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim4)
	{
		countSeconds++;
		if (countSeconds >= settings.BalancingTime)
		{
			noCommandFromController = true;
			HAL_TIM_Base_Stop(&htim4);
		}
	}
}

void updateBalanceTimer()
{
	if (noCommandFromController == true)
	{
		countSeconds = 0;
		noCommandFromController = false;
		HAL_TIM_Base_Start_IT(&htim4);
	} else
	{
		countSeconds = 0;
	}
}