#include "temp.h"

int16_t adc_to_temp_c(uint16_t raw)
{
    /* V_REF = 5000mV(AVCC), 센서 감도 10mV/섭씨 -> temp = raw * (5000/1024) / 10 */
    return (int16_t)(((int32_t)raw * 500) / 1024);
}
