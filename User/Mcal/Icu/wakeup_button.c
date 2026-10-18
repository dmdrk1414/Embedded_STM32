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

/*
 * HAL_GPIO_EXTI_Callback() : EXTI 인터럽트 콜백 (HAL의 __weak 함수 재정의)
 *
 * [설명] ISR 안에서 실행되므로 플래그만 세우고 바로 끝낸다. (HAL_Delay 금지)
 *        호출 경로: EXTI0_IRQHandler → HAL_GPIO_EXTI_IRQHandler(PR 지움) → 여기
 * [인자] GPIO_Pin : 인터럽트가 발생한 핀 (예: WKUP_INT_GPIO_Pin = PB0)
 * [반환] 없음
 * [호출] HAL이 자동 호출 (직접 부르지 않는다)
 * [예시]
 *     PB0 버튼 누름 → 하강 에지 → 이 함수 → s_wkupEvent = 1
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == WKUP_INT_GPIO_Pin)
    {
        s_wkupEvent = 1u;
        g_wakeCount++;
    }
}

/*
 * Wkup_CheckAndClear() : 버튼 웨이크업이 있었는지 확인하고 지우기
 *
 * [설명] ISR이 세운 s_wkupEvent를 읽고 0으로 지운다.
 * [인자] 없음
 * [반환] 1 = 버튼(PB0)으로 깨어남, 0 = 다른 이유
 * [호출] 현재 없음 (예전 Sleep 실습에서 Lp_EnterSleep() 다음에 사용)
 * [예시]
 *     Lp_EnterSleep();
 *     if (Wkup_CheckAndClear() != 0u)
 *     {
 *         // 버튼으로 깨어났을 때만 할 일
 *     }
 */
uint8_t Wkup_CheckAndClear(void)
{
    uint8_t evt = s_wkupEvent;
    s_wkupEvent = 0u;
    return evt;
}
