/*
 * task.c : 주기 Task 구현
 *
 * [Task 작성 규칙]
 *   1. 반드시 10ms 안에 끝나야 한다.
 *   2. HAL_Delay() 금지 : 기다림은 호출 횟수를 세서 처리한다.
 *
 * [슬립 판단]
 *   - PA0(SW_WKUP)가 5초 동안 눌리지 않으면 슬립을 "요청"만 한다.
 *   - 실제 잠들기는 EcuM이 한다. (판단과 실행을 분리)
 *     BswM_MainFunction()이 요청을 보고 EcuM_GoSleep()으로 잠들기 전 리셋을 걸고,
 *     리셋 직후 EcuM_EarlyInit()이 Standby에 들어간다.
 *     → AUTOSAR에서 BswM/EcuM이 "잠들 조건"을 판단하고
 *       실제 저전력 진입은 Mcu 드라이버가 하는 구조와 비슷하다.
 */
#include "task.h"
#include "main.h"
#include "GDS.h"    /* LED_TOGGLE_10MS, SLEEP_TIMEOUT_10MS */

/* 테스트용: Live Expressions에서 1로 바꾸면 Task가 멈춤 → 워치독 리셋 확인 */
volatile uint8_t g_testHang = 0u;

/* Live Expressions 확인용 */
volatile uint32_t g_task10msCount = 0u;

static uint16_t s_ledCnt   = 0u;   /* LED 토글용 카운터            */
static uint16_t s_idleCnt  = 0u;   /* PA0 안 눌린 시간 (10ms 단위) */
static uint8_t  s_sleepReq = 0u;   /* 1 = 슬립 요청                */

/*
 * Task_10ms() : 10ms마다 한 번 실행되는 주기 Task
 *
 * [설명] ① 500ms마다 LED 토글 (깨어 있음 표시)
 *        ② PA0가 5초 동안 안 눌리면 슬립 요청 플래그(s_sleepReq)를 세운다
 *        ③ (테스트) g_testHang = 1이면 일부러 멈춰서 워치독 리셋을 확인한다
 * [인자] 없음
 * [반환] 없음
 * [호출] scheduler.c의 Sch_Run()이 10ms마다 호출 (직접 부르지 않는다)
 * [예시]
 *     Sch_Run() 안에서:
 *         Task_10ms();       // Task 실행
 *         Wdg_Refresh();     // 끝까지 돌아오면 워치독 Refresh
 */
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

    /* [테스트] 일부러 멈춤: 스케줄러로 돌아가지 못해 Refresh가 안 되므로
     * 약 0.1초 뒤 워치독 리셋 (Refresh는 scheduler.c가 Task 뒤에 한다) */
    if (g_testHang != 0u)
    {
        for (;;) { }
    }
}

/*
 * Task_IsSleepRequested() : 슬립 요청이 있는지 알려준다
 *
 * [설명] Task는 직접 잠들지 않고, 요청 상태만 바깥(BswM)에 알려 준다.
 * [인자] 없음
 * [반환] 1 = 5초 동안 PA0 입력 없음 (잠들 시간), 0 = 아직 깨어 있어야 함
 * [호출] mode_manager.c의 BswM_MainFunction()
 * [예시]
 *     if (Task_IsSleepRequested() != 0u)
 *     {
 *         EcuM_GoSleep();    // 잠들기 전 리셋
 *     }
 */
uint8_t Task_IsSleepRequested(void)
{
    return s_sleepReq;
}
