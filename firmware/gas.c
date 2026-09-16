#include "gas.h"

uint8_t adc_to_gas_percent(uint16_t raw)
{
    if (raw > 1023) raw = 1023;
    return (uint8_t)(((uint32_t)raw * 100) / 1023);
}
