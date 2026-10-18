/*
 * indicator.c : 상태 표시 모듈 구현 (PC13 LED, Active Low)
 *   - 함수 접두어 Indicator_ = Indicator (표시기)
 *
 * 역할
 *   - 리셋 원인별 시작 LED 패턴 표시 (패턴 값은 GDS.h)
 *   - 대기는 스케줄러의 Sch_DelayMs()를 쓴다. (워치독 Refresh 포함)
 *     → App은 워치독 드라이버를 직접 부르지 않는다.
 */
#include "indicator.h"
#include "scheduler.h"   /* Sch_DelayMs() : 워치독을 살려 두면서 기다리기 */
#include "main.h"        /* LED 핀, HAL GPIO                              */
#include "GDS.h"         /* INDICATOR_xxx 패턴 값                          */

/* ========================================================================== */
/* 내부 전용 함수 선언 (static) : 정의는 파일 맨 아래                         */
/* ========================================================================== */
static void Indicator_Blink(uint16_t count, uint32_t ms);

/* ========================================================================== */
/* 외부 공개 함수 : 다른 파일에서 호출                                       */
/* ========================================================================== */

/*
 * Indicator_ShowStartup() : 시작할 때 리셋 원인별 LED 패턴 표시
 *
 * [설명] 리셋 원인에 따라 다른 패턴을 보여주고, 끝나면 LED를 끈다.
 *          RST_STANDBY_WAKE     : 웨이크 횟수만큼 천천히 깜빡임
 *          RST_PIN              : 빠르게 10번
 *          RST_WATCHDOG         : 3번 (고장 표시)
 *          RST_STANDBY_WATCHDOG : 표시 없음
 *          그 외 (전원 ON 등)   : 1초 켜기
 *        스케줄러 시작 전에만 사용한다. (기다리는 동안 Task가 돌지 않음)
 * [인자] reason    : 리셋 원인 (EcuM_GetStartupReason())
 *        wakeCount : 웨이크업 횟수 (EcuM_GetWakeCount())
 * [반환] 없음
 * [호출] main.c USER CODE 2 (EcuM_Init() 다음, Sch_Init() 전에 한 번)
 * [예시]
 *     EcuM_Init();
 *     Indicator_ShowStartup(EcuM_GetStartupReason(), EcuM_GetWakeCount());
 *     Sch_Init();
 */
void Indicator_ShowStartup(Rst_ReasonType reason, uint16_t wakeCount)
{
    switch (reason)
    {
        case RST_STANDBY_WAKE:
            /* 로컬 웨이크업 (PA0): 웨이크 횟수만큼 천천히 깜빡임 */
            Indicator_Blink(wakeCount, INDICATOR_WAKE_BLINK_MS);
            break;

        case RST_PIN:
            /* RESET 버튼: 빠르게 10번 */
            Indicator_Blink(INDICATOR_PIN_BLINK_COUNT, INDICATOR_PIN_BLINK_MS);
            break;

        case RST_STANDBY_WATCHDOG:
            /* 자는 중 워치독 리셋: 잠들기 전 리셋 적용 후에는 원래 발생하지 않음. 표시 없음 */
            break;

        case RST_WATCHDOG:
            /* 동작 중 멈춰서 워치독 리셋: 3번 깜빡여 "고장 있었음" 표시 */
            Indicator_Blink(INDICATOR_WDG_BLINK_COUNT, INDICATOR_WDG_BLINK_MS);
            break;

        default:
            /* 전원 ON 등: 1초 켜기 (PC13 = LOW → 켜짐) */
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            Sch_DelayMs(INDICATOR_POWER_ON_MS);
            break;
    }

    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   /* LED 끄기 (Active Low) */
}

/* ========================================================================== */
/* 내부 전용 함수 (static) : 이 파일 안에서만 호출                           */
/* ========================================================================== */

/*
 * Indicator_Blink() : LED를 count번 깜빡인다 (내부 전용)
 *
 * [설명] 한 번 깜빡임 = 토글 2번 → count × 2번 토글한다.
 *        대기는 Sch_DelayMs()라서 기다리는 동안에도 워치독이 Refresh된다.
 * [인자] count : 깜빡일 횟수
 *        ms    : 토글 사이 간격 (ms, 작을수록 빠르게)
 * [반환] 없음
 * [호출] Indicator_ShowStartup()
 * [예시]
 *     Indicator_Blink(10u, 50u);    // 빠르게 10번 (RESET 버튼)
 *     Indicator_Blink(3u, 100u);    // 3번 (워치독 리셋, 고장 표시)
 */
static void Indicator_Blink(uint16_t count, uint32_t ms)
{
    for (uint16_t i = 0u; i < (uint16_t)(count * 2u); i++)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        Sch_DelayMs(ms);
    }
}
