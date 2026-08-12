#ifndef BATTERY_ADC_H
#define BATTERY_ADC_H

#include <stdbool.h>
#include <stdint.h>

void BatteryADC_InitPin(void);
bool BatteryADC_ReadMillivolts(uint16_t *battery_mv);

#endif
