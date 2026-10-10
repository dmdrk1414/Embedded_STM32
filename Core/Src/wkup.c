#include "wkup.h"
#include "main.h"

/* ISR과 main이 함께 쓰는 변수이므로 volatile */
static volatile uint8_t s_wkupEvent = 0u;

/* EXTI0_IRQHandler → HAL_GPIO_EXTI_IRQHandler(PR 지움) → 여기 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == WKUP_INT_GPIO_Pin)
    {
        s_wkupEvent = 1u;
    }
}

uint8_t Wkup_CheckAndClear(void)
{
    uint8_t evt = s_wkupEvent;
    s_wkupEvent = 0u;
    return evt;
}
