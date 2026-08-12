#include "battery_adc.h"
#include "main.h"

#define BATTERY_ADC_PIN                 GPIO_PIN_0
#define BATTERY_ADC_GPIO_PORT           GPIOA
#define BATTERY_ADC_DIVIDER_MULTIPLIER  2UL
#define ADC_FULL_SCALE                  4095UL
#define VREFINT_CAL_ADDRESS             ((const uint16_t *)0x1FFF6EA4UL)
#define VREFINT_CAL_VDDA_MV             3000UL
#define ADC_OPERATION_TIMEOUT_MS        5UL

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

    ADC1->ISR = ADC_ISR_CCRDY;
    /* On STM32U031 TSSOP20, physical pin 8 / PA0 is ADC1_IN5. */
    ADC1->CHSELR = ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL12;
    if (!WaitForSet(&ADC1->ISR, ADC_ISR_CCRDY, ADC_OPERATION_TIMEOUT_MS))
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

    ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    ADC1->CR |= ADC_CR_ADSTART;
    if (!WaitForSet(&ADC1->ISR, ADC_ISR_EOC, ADC_OPERATION_TIMEOUT_MS))
    {
        PowerDown();
        return false;
    }
    divider_raw = ADC1->DR;

    if (!WaitForSet(&ADC1->ISR, ADC_ISR_EOC, ADC_OPERATION_TIMEOUT_MS))
    {
        PowerDown();
        return false;
    }
    vref_raw = ADC1->DR;
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
    if (calculated_mv > UINT16_MAX)
    {
        return false;
    }

    *battery_mv = (uint16_t)calculated_mv;
    return true;
}
