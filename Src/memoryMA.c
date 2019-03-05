/* Includes ------------------------------------------------------------------*/
#include "memoryMA.h"
#include <string.h>
#include "usart.h"

MA_MeasurementsTypeDef measurements = { 0 };
const uint8_t sizeOfMeasurementsStruct = sizeof(measurements);
MA_SettingsTypeDef settings = { 0 };
const uint8_t sizeOfSettingsStruct = sizeof(settings);
uint8_t settingsArray[sizeof(settings)] = { 0 };
uint8_t settingsChecksum = 0;
uint8_t actionsRegister = 0;

uint8_t calculateSettingsChecksum();

bool writeDataToSettingsArray(uint8_t *data, const uint8_t size, const uint8_t address)
{
	bool dataWriteSuccessfully = false;
	if ((address + size) <= sizeOfSettingsStruct)
	{
		memcpy(settingsArray + address, data, size);
		dataWriteSuccessfully = true;
	}
	return dataWriteSuccessfully;
}

void setBalanceState(const MA_BalanceState state)
{
	switch (state)
	{
	case BalancingIn_ENABLE:
		measurements.MA_Event_Register &= ~MA_Event_Balancing_Out_Msk;
		measurements.MA_Event_Register &= ~MA_Event_Transformer_Out_Msk;
		measurements.MA_Event_Register |= MA_Event_Balancing_In_Msk;
		break;
	case BalancingOut_ENABLE:
		measurements.MA_Event_Register &= ~MA_Event_Balancing_In_Msk;
		measurements.MA_Event_Register |= MA_Event_Transformer_Out_Msk;
		measurements.MA_Event_Register |= MA_Event_Balancing_Out_Msk;
		break;
	case Balancing_DISABLE:
		measurements.MA_Event_Register &= ~MA_Event_Balancing_In_Msk;
		measurements.MA_Event_Register &= ~MA_Event_Balancing_Out_Msk;
		measurements.MA_Event_Register &= ~MA_Event_Transformer_Out_Msk;
		break;
	default:
		Error_Handler();
	}
}

void setAction(const MA_Action action)
{
	switch (action)
	{
	case SendMeasurements:
		actionsRegister |= MA_Action_SendMeasurements_Msk;
		break;
	case SendSettingsChecksum:
		actionsRegister |= MA_Action_SendSettingsChecksum_Msk;
		break;
	case UpdateSettings:
		actionsRegister |= MA_Action_UpdateSettings_Msk;
		break;
	default:
		Error_Handler();
	}
}

void executeActions()
{
	if (actionsRegister > 0)
	{
		if ((actionsRegister & MA_Action_SendMeasurements_Msk) == MA_Action_SendMeasurements)
		{
			if (UART_TransmitData((uint8_t*)&measurements, sizeOfMeasurementsStruct) == true)
			{
				actionsRegister &= ~MA_Action_SendMeasurements_Msk;
			}
		}
		if ((actionsRegister & MA_Action_SendSettingsChecksum_Msk) == MA_Action_SendSettingsChecksum)
		{
			uint8_t checksum = calculateSettingsChecksum();
			if (UART_TransmitData((uint8_t*)&checksum, sizeof(checksum)) == true)
			{
				actionsRegister &= ~MA_Action_SendSettingsChecksum_Msk;
			}
		}
		if ((actionsRegister & MA_Action_UpdateSettings_Msk) == MA_Action_UpdateSettings)
		{
			memcpy(&settings, settingsArray, sizeOfSettingsStruct);
			actionsRegister &= ~MA_Action_UpdateSettings_Msk;
		}
	}
}

uint8_t calculateSettingsChecksum()
{
	uint8_t checksum = 0;
	for (uint8_t i = 0; i < sizeOfSettingsStruct; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			checksum += ((settingsArray[i] >> j) & 0x1U);
		}
	}
	return checksum;
}