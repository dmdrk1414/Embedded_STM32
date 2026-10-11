/*
 * ecum.c : ECU 상태 관리 모듈 구현
 *
 * 상태 흐름
 *   [리셋/웨이크업] → EcuM_Init() → [RUN: 10ms Task 동작]
 *                                      │ 슬립 요청 (5초 무입력)
 *                                      ▼
 *                    EcuM_MainFunction() → EcuM_GoSleep() → [Standby]
 *                                                              │ PA0 상승 에지
 *                                                              ▼
 *                                                       [리셋/웨이크업] ...
 */
#include "ecum.h"
#include "rst.h"    /* Rst_GetReason()                         */
#include "lp.h"     /* Lp_EnterStandby(), 웨이크 횟수(BKP->DR1)  */
#include "task.h"   /* Task_IsSleepRequested()                 */
#include "main.h"   /* LED 핀, HAL                              */

/*
 * EcuM_Blink() : 시작 표시용 LED 깜빡임 (내부 전용)
 *   - 시작 단계에서만 쓰므로 HAL_Delay 사용 가능
 *     (아직 10ms 스케줄러가 돌기 전이라 Task 주기에 영향 없음)
 */
static void EcuM_Blink(uint16_t count, uint32_t ms)
{
    for (uint16_t i = 0u; i < (uint16_t)(count * 2u); i++)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(ms);
    }
}

/*
 * EcuM_GoSleep() : 잠들 준비 → Standby 진입 (내부 전용, 돌아오지 않음)
 *   - 잠들기 전 준비 작업은 모두 여기에 모은다.
 *   - 나중에 "데이터 저장, 주변장치 끄기" 등이 추가될 자리.
 */
static void EcuM_GoSleep(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   /* LED 끄기 (Active Low) */
    Lp_EnterStandby();                                         /* 여기서 잠듦 */
}

/*
 * EcuM_Init() : 시작 처리
 *   Standby에서 깨어나도 리셋처럼 처음부터 시작하므로,
 *   "왜 시작했는지"에 따라 유지 메모리와 시작 표시를 다르게 처리한다.
 */
void EcuM_Init(void)
{
    switch (Rst_GetReason())
    {
        case RST_STANDBY_WAKE:
            /* 로컬 웨이크업 (PA0): 횟수 +1, 그 횟수만큼 천천히 깜빡임 */
            EcuM_Blink(Lp_IncWakeCount(), 150u);
            break;

        case RST_PIN:
            /* RESET 버튼: 웨이크 횟수는 유지, 빠르게 10번 깜빡임 */
            EcuM_Blink(10u, 50u);
            break;

        default:
            /* 전원 ON 등: 유지 메모리를 명시적으로 초기화, 1초 켜기 */
            Lp_ClearWakeCount();
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            HAL_Delay(1000u);
            break;
    }

    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   /* LED 끄기 */
}

/*
 * EcuM_MainFunction() : 슬립 조건 판단
 *   - 지금 조건: Task가 슬립을 요청했는가 (PA0 5초 무입력)
 *   - 조건이 늘어나면 여기에 AND로 추가한다. (AUTOSAR의 BswM 규칙 역할)
 */
void EcuM_MainFunction(void)
{
    if (Task_IsSleepRequested() != 0u)
    {
        EcuM_GoSleep();
    }
}
