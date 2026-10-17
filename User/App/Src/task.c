/*
 * task.c : 주기 Task 구현
 *
 * [Task 작성 규칙]
 *   1. 반드시 10ms 안에 끝나야 한다.
 *   2. HAL_Delay() 금지 : 기다림은 호출 횟수를 세서 처리한다.
 *
 * [슬립 판단]
 *   - PA0(SW_WKUP)가 5초 동안 눌리지 않으면 슬립을 "요청"만 한다.
 *   - 실제 Standby 진입은 main이 한다. (판단과 실행을 분리)
 *     → AUTOSAR에서 BswM/EcuM이 "잠들 조건"을 판단하고
 *       실제 DeepSTOP 진입은 Mcu 드라이버가 하는 구조와 같다.
 */
#include "task.h"
#include "main.h"

#define LED_TOGGLE_10MS      (50u)    /* 10ms × 50  = 500ms */
#define SLEEP_TIMEOUT_10MS   (500u)   /* 10ms × 500 = 5초   */

/* Live Expressions 확인용 */
volatile uint32_t g_task10msCount = 0u;

static uint16_t s_ledCnt   = 0u;   /* LED 토글용 카운터            */
static uint16_t s_idleCnt  = 0u;   /* PA0 안 눌린 시간 (10ms 단위) */
static uint8_t  s_sleepReq = 0u;   /* 1 = 슬립 요청                */

void Task_10ms(void)
{
    g_task10msCount++;

    /* ① 깨어있음 표시: 500ms마다 LED 토글 */
    s_ledCnt++;
    if (s_ledCnt >= LED_TOGGLE_10MS)
    {
        s_ledCnt = 0u;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }

    /* ② 슬립 판단: PA0는 누르면 HIGH (버튼이 3.3V 쪽, 평소 Pull-down으로 LOW) */
    if (HAL_GPIO_ReadPin(SW_WKUP_GPIO_PA_0_GPIO_Port, SW_WKUP_GPIO_PA_0_Pin) == GPIO_PIN_SET)
    {
        s_idleCnt = 0u;                /* 눌려 있으면 시간 처음부터 다시 */
    }
    else if (s_idleCnt < SLEEP_TIMEOUT_10MS)
    {
        s_idleCnt++;                   /* 안 눌렸으면 10ms 누적 */
    }
    else
    {
        s_sleepReq = 1u;               /* 5초 동안 입력 없음 → 슬립 요청 */
    }
}

uint8_t Task_IsSleepRequested(void)
{
    return s_sleepReq;
}
