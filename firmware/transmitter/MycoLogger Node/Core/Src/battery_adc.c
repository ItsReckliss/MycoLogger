#include "battery_adc.h"
#include "main.h"

#define BATTERY_ADC_PIN                 GPIO_PIN_0
#define BATTERY_ADC_GPIO_PORT           GPIOA
#define BATTERY_ADC_DIVIDER_MULTIPLIER  2UL
#define BATTERY_CALIBRATION_NUMERATOR    9992UL
#define BATTERY_CALIBRATION_DENOMINATOR  10000UL
#define ADC_FULL_SCALE                  4095UL
#define VREFINT_CAL_ADDRESS             ((const uint16_t *)0x1FFF6EA4UL)
#define VREFINT_CAL_VDDA_MV             3000UL
#define ADC_OPERATION_TIMEOUT_MS        5UL

/*
 * STM32U031 external ADC channel numbers in DS14581 Rev 2 are shifted by one,
 * so PA0 is ADC1_IN4 rather than ADC1_IN5. On the STM32U031F6 Rev A used by
 * the transmitter, VREFINT remains on internal channel 12.
 */
#define BATTERY_ADC_CHANNEL             ADC_CHSELR_CHSEL4
#define VREFINT_ADC_CHANNEL             ADC_CHSELR_CHSEL12

/* Retained in SRAM so an attached debugger can inspect the last conversion. */
volatile uint32_t g_battery_divider_adc_raw = 0UL;
volatile uint32_t g_battery_vrefint_adc_raw = 0UL;
volatile uint32_t g_battery_vdda_mv = 0UL;

static bool WaitForSet(volatile uint32_t *reg,
                       uint32_t mask,
                       uint32_t timeout_ms)
{
    uint32_t started = HAL_GetTick();
    while ((*reg & mask) == 0UL)
    {
        if ((HAL_GetTick() - started) >= timeout_ms)
        {
            return false;
        }
    }
    return true;
}

static bool WaitForClear(volatile uint32_t *reg,
                         uint32_t mask,
                         uint32_t timeout_ms)
{
    uint32_t started = HAL_GetTick();
    while ((*reg & mask) != 0UL)
    {
        if ((HAL_GetTick() - started) >= timeout_ms)
        {
            return false;
        }
    }
    return true;
}

static void PowerDown(void)
{
    if ((ADC1->CR & ADC_CR_ADEN) != 0UL)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        (void)WaitForClear(&ADC1->CR,
                           ADC_CR_ADEN,
                           ADC_OPERATION_TIMEOUT_MS);
    }
    ADC1->CR &= ~ADC_CR_ADVREGEN;
    ADC1_COMMON->CCR &= ~ADC_CCR_VREFEN;
    __HAL_RCC_ADC_CLK_DISABLE();
}

static bool ConvertChannel(uint32_t channel, uint32_t *raw)
{
    if (raw == NULL)
    {
        return false;
    }

    /*
     * Convert one channel at a time. A multi-channel scan can leave EOC set
     * while the next result is not yet in DR on this ADC, which made the
     * second (VREFINT) read return zero.
     */
    ADC1->ISR = ADC_ISR_CCRDY | ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    ADC1->CHSELR = channel;
    if (!WaitForSet(&ADC1->ISR, ADC_ISR_CCRDY, ADC_OPERATION_TIMEOUT_MS))
    {
        return false;
    }

    ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    ADC1->CR |= ADC_CR_ADSTART;
    if (!WaitForSet(&ADC1->ISR, ADC_ISR_EOC, ADC_OPERATION_TIMEOUT_MS))
    {
        return false;
    }

    *raw = ADC1->DR;
    return true;
}

void BatteryADC_InitPin(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = BATTERY_ADC_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BATTERY_ADC_GPIO_PORT, &gpio);
}

bool BatteryADC_ReadMillivolts(uint16_t *battery_mv)
{
    uint32_t divider_raw;
    uint32_t vref_raw;
    uint32_t vdda_mv;
    uint64_t calculated_mv;
    uint16_t vref_cal;

    if (battery_mv == NULL)
    {
        return false;
    }
    *battery_mv = 0U;

    __HAL_RCC_ADC_CLK_ENABLE();

    /* PCLK/4 gives a 4 MHz ADC clock at the current 16 MHz system clock. */
    ADC1->CFGR1 = 0UL;
    ADC1->CFGR2 = ADC_CFGR2_CKMODE_1;
    /* 160.5 cycles for both PA0 and VREFINT supports the high-value divider. */
    ADC1->SMPR = ADC_SMPR_SMP1;
    ADC1_COMMON->CCR |= ADC_CCR_VREFEN;
    ADC1->CR |= ADC_CR_ADVREGEN;
    HAL_Delay(1U);

    /* Offset calibration is repeated because the ADC regulator is power-cycled. */
    ADC1->CR |= ADC_CR_ADCAL;
    if (!WaitForClear(&ADC1->CR, ADC_CR_ADCAL, ADC_OPERATION_TIMEOUT_MS))
    {
        PowerDown();
        return false;
    }

    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    if (!WaitForSet(&ADC1->ISR, ADC_ISR_ADRDY, ADC_OPERATION_TIMEOUT_MS))
    {
        PowerDown();
        return false;
    }

    if (!ConvertChannel(BATTERY_ADC_CHANNEL, &divider_raw) ||
        !ConvertChannel(VREFINT_ADC_CHANNEL, &vref_raw))
    {
        PowerDown();
        return false;
    }
    g_battery_divider_adc_raw = divider_raw;
    g_battery_vrefint_adc_raw = vref_raw;
    PowerDown();

    vref_cal = *VREFINT_CAL_ADDRESS;
    if ((divider_raw > ADC_FULL_SCALE) ||
        (vref_raw == 0UL) || (vref_raw > ADC_FULL_SCALE) ||
        (vref_cal == 0U) || (vref_cal == 0xFFFFU))
    {
        return false;
    }

    vdda_mv = ((uint32_t)vref_cal * VREFINT_CAL_VDDA_MV +
               (vref_raw / 2UL)) / vref_raw;
    g_battery_vdda_mv = vdda_mv;
    calculated_mv = ((uint64_t)divider_raw * vdda_mv *
                     BATTERY_ADC_DIVIDER_MULTIPLIER +
                     (ADC_FULL_SCALE / 2UL)) / ADC_FULL_SCALE;
    /* Calibrated at 3.752 V against a simultaneous battery-lead DMM reading. */
    calculated_mv = (calculated_mv * BATTERY_CALIBRATION_NUMERATOR +
                     (BATTERY_CALIBRATION_DENOMINATOR / 2UL)) /
                    BATTERY_CALIBRATION_DENOMINATOR;
    if (calculated_mv > UINT16_MAX)
    {
        return false;
    }

    *battery_mv = (uint16_t)calculated_mv;
    return true;
}
