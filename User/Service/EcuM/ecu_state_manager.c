/*
 * ecu_state_manager.c : ECU 상태 관리 모듈 구현
 *   - 함수 접두어 EcuM_ = ECU State Manager (ECU 상태 관리자)
 *
 * 상태 흐름
 *   [리셋/웨이크업] → EcuM_EarlyInit() (메모 없음) → MX_IWDG_Init()
 *                  → EcuM_Init() → [RUN: 10ms Task 동작, IWDG ON]
 *                                      │ 슬립 요청 (5초 무입력)
 *                                      ▼
 *                    EcuM_MainFunction() → EcuM_GoSleep()
 *                                          (BKP->DR2 메모 + 소프트웨어 리셋)
 *                                      │ 리셋 → IWDG OFF
 *                                      ▼
 *                    EcuM_EarlyInit() (메모 있음) → [Standby, IWDG OFF]
 *                                      │ PA0 상승 에지
 *                                      ▼
 *                                [리셋/웨이크업] ...
 */
#include "ecu_state_manager.h"
#include "reset_reason.h"    /* Rst_GetReason()                         */
#include "low_power.h"     /* Lp_EnterStandby(), 웨이크 횟수(BKP->DR1)  */
#include "task.h"   /* Task_IsSleepRequested()                 */
#include "main.h"   /* LED 핀, HAL                              */
#include "watchdog.h"    /* 대기 중 워치독 Refresh */


/*
 * EcuM_Wait() : 워치독을 살려 두면서 기다리기 (내부 전용)
 *   - 워치독 타임아웃(최소 약 67ms)보다 긴 HAL_Delay를 쓰면 리셋된다.
 *   - 그래서 10ms씩 나눠 기다리며 매번 Refresh한다.
 *   - 시작 단계 전용. 스케줄러가 돈 뒤에는 사용하지 않는다.
 */
static void EcuM_Wait(uint32_t ms)
{
    while (ms >= 10u)
    {
        HAL_Delay(10u);
        Wdg_Refresh();
        ms -= 10u;
    }
}

/*
 * EcuM_Blink() : 시작 표시용 LED 깜빡임 (내부 전용)
 *   - 시작 단계 전용 (아직 10ms 스케줄러가 돌기 전이라 Task 주기에 영향 없음)
 *   - 대기는 HAL_Delay 대신 EcuM_Wait()를 써서 워치독을 Refresh한다.
 */
static void EcuM_Blink(uint16_t count, uint32_t ms)
{
    for (uint16_t i = 0u; i < (uint16_t)(count * 2u); i++)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        EcuM_Wait(ms);
    }
}

/*
 * EcuM_GoSleep() : 잠들 준비 → 소프트웨어 리셋 (내부 전용, 돌아오지 않음)
 *
 *   [왜 바로 Standby에 들어가지 않고 리셋하는가]
 *   - IWDG는 한번 켜지면 끌 수 없고 (RM0008 5.3.4, p.75),
 *     LSI 클럭도 끊을 수 없다. (RM0008 7.2.9, p.98)
 *   - 그대로 Standby에 들어가면 IWDG가 약 0.1초 만에 깨운다.
 *   - 리셋하면 IWDG도 꺼지므로, 리셋 직후 IWDG가 켜지기 전에 잠든다.
 *   - 회사 제품의 예전 방식 "슬립 전 리셋"과 같은 구조.
 */
static void EcuM_GoSleep(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   /* LED 끄기 */
    Lp_SetSleepRequest();                                      /* "다음 시작 때 바로 잠들어라" */
    NVIC_SystemReset();                                        /* 소프트웨어 리셋 (IWDG 꺼짐) */
}

/*
 * EcuM_EarlyInit() : 리셋 직후 가장 먼저 실행 (MX_IWDG_Init 보다 앞)
 *   - 잠들기 요청 표시가 있으면, IWDG가 꺼진 상태 그대로 Standby 진입.
 *   - 표시가 없으면 아무것도 하지 않고 돌아가서 평소처럼 초기화.
 */
void EcuM_EarlyInit(void)
{
    if (Lp_TakeSleepRequest() != 0u)
    {
        /* 이번 리셋은 "잠들기 위한 리셋"이므로 리셋 플래그(SFTRSTF)를 정리해 둔다.
         * 그래야 PA0로 깨어났을 때 원인 판단이 깔끔하다. */
        __HAL_RCC_CLEAR_RESET_FLAGS();
        Lp_EnterStandby();                /* 여기서 잠듦 (IWDG 없이) */
    }
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

        case RST_STANDBY_WATCHDOG:
            /* 자는 중 워치독이 깨움: 잠들기 전 리셋 적용 후에는 IWDG가 꺼진 채 잠들어서
             * 원래 발생하지 않는다. 혹시 발생하면 웨이크 횟수는 올리지 않고 표시 없이 진행 */
            break;

        case RST_WATCHDOG:
            /* 동작 중 멈춰서 워치독 리셋: 빠르게 3번 깜빡여 "고장 있었음" 표시 */
            EcuM_Blink(3u, 100u);
            break;

        default:
            /* 전원 ON 등: 유지 메모리를 명시적으로 초기화, 1초 켜기 */
            Lp_ClearWakeCount();
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            EcuM_Wait(1000u);
            break;
    }

    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   /* LED 끄기 */
}

/*
 * EcuM_MainFunction() : 슬립 조건 판단
 *   - 지금 조건: Task가 슬립을 요청했는가 (PA0 5초 무입력)
 *   - 조건이 맞으면 EcuM_GoSleep() → 잠들기 전 리셋 (돌아오지 않음)
 *   - 조건이 늘어나면 여기에 AND로 추가한다. (AUTOSAR의 BswM 규칙 역할)
 */
void EcuM_MainFunction(void)
{
    if (Task_IsSleepRequested() != 0u)
    {
        EcuM_GoSleep();
    }
}
