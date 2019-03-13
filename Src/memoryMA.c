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

void sendMeasurements()
{
	UART_TransmitData((uint8_t*)&measurements, sizeOfMeasurementsStruct);
}

void sendSettingsChecksum()
{
	uint8_t checksum = calculateSettingsChecksum();
	UART_TransmitData((uint8_t*)&checksum, sizeof(checksum));
}

void updateSettings()
{
	memcpy(&settings, settingsArray, sizeOfSettingsStruct);
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