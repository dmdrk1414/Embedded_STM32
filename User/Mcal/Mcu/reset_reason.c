/*
 * reset_reason.c : 리셋 원인 판별 모듈 구현
 *   - 함수 접두어 Rst_ = Reset (리셋 원인)
 */
#include "reset_reason.h"
#include "low_power.h"     /* Lp_IsWakeFromStandby() : SBF 확인 함수 */
#include "main.h"   /* HAL 매크로 (__HAL_RCC_GET_FLAG 등)      */

/* Live Expressions 확인용: 이번 시작의 리셋 원인 (enum 이름으로 보임) */
volatile Rst_ReasonType g_rstReason = RST_UNKNOWN;

/*
 * Rst_GetReason()
 *
 * [확인 순서가 중요한 이유]
 *   RM0008 7.1.2 Power reset (p.91)에 따르면
 *   "전원 ON"과 "Standby 복귀"는 Power reset이고, 이때도 내부적으로
 *   NRST 핀이 LOW로 당겨진다.
 *   → 이 두 경우에도 PINRSTF(RESET 버튼 플래그)가 같이 켜질 수 있다.
 *   → 또 Standby 중 워치독 리셋은 SBF와 IWDGRSTF가 둘 다 1이다.
 *   → 그래서 워치독 → SBF → PORRSTF → PINRSTF → SFTRSTF 순서로 확인해야
 *     각 원인을 정확히 골라낼 수 있다.
 *
 * [플래그가 남아 있는 이유]
 *   RM0008 7.1.1 (p.90): 시스템 리셋은 모든 레지스터를 초기화하지만
 *   RCC_CSR의 리셋 플래그는 예외로 남겨 둔다. 그래서 리셋 후에 읽을 수 있다.
 */
Rst_ReasonType Rst_GetReason(void)
{
    Rst_ReasonType reason;

    /* Standby에 있었는가? (PWR_CSR.SBF)
     *   Lp_IsWakeFromStandby()는 SBF를 확인한 뒤 스스로 지우므로
     *   딱 한 번만 호출해서 결과를 저장해 둔다. */
    uint8_t wasStandby = Lp_IsWakeFromStandby();

    /* 워치독 리셋인가? (IWDG: 독립 워치독, WWDG: 윈도우 워치독) */
    uint8_t isWdg = ((__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0u) ||
                     (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != 0u)) ? 1u : 0u;

    /* ① 워치독 리셋
     *    Standby 중 워치독 리셋은 SBF와 IWDGRSTF가 둘 다 1이므로
     *    SBF보다 먼저 확인해야 구분할 수 있다.
     *    - 자는 중이었음 → 자는 중 워치독이 깨움 (잠들기 전 리셋 적용 후엔 원래 없음)
     *    - 동작 중이었음 → 프로그램이 멈춤 (진짜 고장) */
    if (isWdg != 0u)
    {
        reason = (wasStandby != 0u) ? RST_STANDBY_WATCHDOG : RST_WATCHDOG;
    }
    /* ② Standby 복귀
     *    워치독이 아닌데 Standby였다 → PA0(WKUP)로 깨어남 */
    else if (wasStandby != 0u)
    {
        reason = RST_STANDBY_WAKE;
    }
    /* ③ 전원 ON인가? (RCC_CSR.PORRSTF, POR: Power-On Reset)
     *    전원 ON은 PINRSTF도 같이 켜질 수 있으므로 PINRSTF보다 먼저 확인 */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != 0u)
    {
        reason = RST_POWER_ON;
    }
    /* ④ RESET 버튼인가? (RCC_CSR.PINRSTF, NRST 핀이 LOW가 됨)
     *    위가 모두 아닌데 PINRSTF가 켜져 있으면 진짜 RESET 버튼 */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != 0u)
    {
        reason = RST_PIN;
    }
    /* ⑤ 소프트웨어 리셋인가? (RCC_CSR.SFTRSTF)
     *    코드에서 NVIC_SystemReset()을 호출했을 때 */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != 0u)
    {
        reason = RST_SOFTWARE;
    }
    /* ⑥ 그 외 */
    else
    {
        reason = RST_UNKNOWN;
    }

    g_rstReason = reason;   /* Live Expressions 확인용 */

    /* RCC_CSR.RMVF (bit 24) = 1 : 리셋 플래그를 모두 지운다.
     * 지우지 않으면 플래그가 계속 쌓여서, 다음 리셋 때
     * 이전 원인과 새 원인을 구분할 수 없게 된다. */
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return reason;
}
