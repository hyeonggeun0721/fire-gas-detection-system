#include "gas.h"

uint8_t adc_to_gas_percent(uint16_t raw)
{
    if (raw > 1023) raw = 1023;
    /* 반올림: 0.5V(ADC 102) -> 10%. 버림이면 9%가 되어 다이얼 표시와 어긋난다 */
    return (uint8_t)(((uint32_t)raw * 100 + 511) / 1023);
}
