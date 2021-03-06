/* Includes ------------------------------------------------------------------*/
#include "memoryMA.h"
#include <string.h>
#include "usart.h"
#include "adc.h"

MA_MeasurementsTypeDef measurements = { 0 };
const uint8_t sizeOfMeasurementsStruct = sizeof(measurements);
MA_SettingsTypeDef settings = { 0 };
const uint8_t sizeOfSettingsStruct = sizeof(settings);
uint8_t settingsArray[sizeof(settings)] = { 0 };
uint8_t settingsChecksum = 0;
uint8_t actionsRegister = 0;

uint8_t calculateSettingsChecksum();
void updateADCMeasurements();
void updateGPIOMeasurements();

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
		measurements.MA_Event_Register |= MA_Event_Balancing_In_Msk;
		break;
	case BalancingOut_ENABLE:
		measurements.MA_Event_Register &= ~MA_Event_Balancing_In_Msk;
		measurements.MA_Event_Register |= MA_Event_Balancing_Out_Msk;
		break;
	case Balancing_DISABLE:
		measurements.MA_Event_Register &= ~MA_Event_Balancing_In_Msk;
		measurements.MA_Event_Register &= ~MA_Event_Balancing_Out_Msk;
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
	memcpy((void*) &settings, settingsArray, sizeOfSettingsStruct);
}

uint8_t calculateSettingsChecksum()
{
	uint16_t checksum = 0;
	for (uint8_t i = 0; i < sizeOfSettingsStruct; i++)
	{
		checksum += (uint16_t) (settingsArray[i] * CHECKSUM_CONSTANT);
	}
	return (uint8_t) checksum;
}

void updateMeasurements()
{
	updateADCMeasurements();
	updateGPIOMeasurements();
}

void updateADCMeasurements()
{
	uint16_t data0, data7;
	if (getADCData(&data0, &data7))
	{
		measurements.I_balance = (data0 - ADC_I_ZERO_BIT) * ADC_I_K;
		measurements.U_cell = data7 * ADC_U_ACC_K;
	}
}

void updateGPIOMeasurements()
{
	if (HAL_GPIO_ReadPin(BuffEn_GPIO_Port, BuffEn_Pin) == GPIO_PIN_SET)
	{
		measurements.MA_Event_Register &= ~MA_Event_BufferEnable_Msk;
	} else
	{
		measurements.MA_Event_Register |= MA_Event_BufferEnable_Msk;
	}
}