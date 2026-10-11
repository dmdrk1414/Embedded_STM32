/*
 * task.c : 주기 Task 구현
 *
 * [Task 작성 규칙]
 *   1. 반드시 10ms 안에 끝나야 한다.
 *   2. HAL_Delay() 금지 : Task 안에서 기다리면 다음 주기를 놓친다.
 *   3. "기다림"이 필요하면 호출 횟수를 세서 처리한다 (아래 LED 예시).
 */
#include "task.h"
#include "main.h"

/* Live Expressions 확인용: 1초에 약 100씩 증가해야 정상 */
volatile uint32_t g_task10msCount = 0u;

/* LED 토글용 카운터: 10ms × 50번 = 500ms */
static uint16_t s_ledCnt = 0u;

void Task_10ms(void)
{
    g_task10msCount++;

    /* HAL_Delay(500) 대신 "50번 호출될 때마다" 토글 → 0.5초 간격 깜빡임 */
    s_ledCnt++;
    if (s_ledCnt >= 50u)
    {
        s_ledCnt = 0u;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
}
