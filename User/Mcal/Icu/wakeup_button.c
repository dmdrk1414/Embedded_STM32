/*
 * wakeup_button.c : EXTI 웨이크업 감지 구현 (Sleep 실습용, 현재 미사용)
 *   - 함수 접두어 Wkup_ = Wakeup (웨이크업)
 */
#include "wakeup_button.h"
#include "main.h"

/* ISR과 main이 함께 쓰는 변수이므로 volatile
 * g_wakeCount : 버튼 인터럽트 횟수 (BKP의 웨이크 횟수 DR1과는 다른 RAM 변수) */
static volatile uint8_t s_wkupEvent = 0u;
static volatile uint32_t g_wakeCount = 0u;

/* EXTI0_IRQHandler → HAL_GPIO_EXTI_IRQHandler(PR 지움) → 여기 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == WKUP_INT_GPIO_Pin)
    {
        s_wkupEvent = 1u;
        g_wakeCount++;
    }
}

uint8_t Wkup_CheckAndClear(void)
{
    uint8_t evt = s_wkupEvent;
    s_wkupEvent = 0u;
    return evt;
}
