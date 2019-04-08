#pragma once

#include <stm32f1xx_hal_conf.h>

void updateBalancing();
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void updateBalanceTimer();