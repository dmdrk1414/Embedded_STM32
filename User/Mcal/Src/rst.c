/*
 * rst.c : 리셋 원인 판별 모듈 구현
 */
#include "rst.h"
#include "lp.h"     /* Lp_IsWakeFromStandby() : SBF 확인 함수 */
#include "main.h"   /* HAL 매크로 (__HAL_RCC_GET_FLAG 등)      */

/*
 * Rst_GetReason()
 *
 * [확인 순서가 중요한 이유]
 *   RM0008 7.1.2 Power reset (p.91)에 따르면
 *   "전원 ON"과 "Standby 복귀"는 Power reset이고, 이때도 내부적으로
 *   NRST 핀이 LOW로 당겨진다.
 *   → 이 두 경우에도 PINRSTF(RESET 버튼 플래그)가 같이 켜질 수 있다.
 *   → 그래서 SBF → PORRSTF → PINRSTF 순서로 확인해야
 *     "진짜 RESET 버튼"만 정확히 골라낼 수 있다.
 *
 * [플래그가 남아 있는 이유]
 *   RM0008 7.1.1 (p.90): 시스템 리셋은 모든 레지스터를 초기화하지만
 *   RCC_CSR의 리셋 플래그는 예외로 남겨 둔다. 그래서 리셋 후에 읽을 수 있다.
 */
Rst_ReasonType Rst_GetReason(void)
{
    Rst_ReasonType reason;

    /* ① Standby 복귀인가? (PWR_CSR.SBF)
     *    Lp_IsWakeFromStandby()는 SBF를 확인한 뒤 스스로 지운다. */
    if (Lp_IsWakeFromStandby() != 0u)
    {
        reason = RST_STANDBY_WAKE;
    }
    /* ② 전원 ON인가? (RCC_CSR.PORRSTF, POR: Power-On Reset)
     *    전원 ON은 PINRSTF도 같이 켜질 수 있으므로 PINRSTF보다 먼저 확인 */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != 0u)
    {
        reason = RST_POWER_ON;
    }
    /* ③ RESET 버튼인가? (RCC_CSR.PINRSTF, NRST 핀이 LOW가 됨)
     *    ①②가 아닌데 PINRSTF가 켜져 있으면 진짜 RESET 버튼 */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != 0u)
    {
        reason = RST_PIN;
    }
    /* ④ 소프트웨어 리셋인가? (RCC_CSR.SFTRSTF)
     *    코드에서 NVIC_SystemReset()을 호출했을 때 */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != 0u)
    {
        reason = RST_SOFTWARE;
    }
    /* ⑤ 워치독 리셋인가? (IWDG: 독립 워치독, WWDG: 윈도우 워치독)
     *    프로그램이 멈춰서 워치독을 갱신하지 못했을 때 */
    else if ((__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0u) ||
             (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != 0u))
    {
        reason = RST_WATCHDOG;
    }
    /* ⑥ 그 외 */
    else
    {
        reason = RST_UNKNOWN;
    }

    /* RCC_CSR.RMVF (bit 24) = 1 : 리셋 플래그를 모두 지운다.
     * 지우지 않으면 플래그가 계속 쌓여서, 다음 리셋 때
     * 이전 원인과 새 원인을 구분할 수 없게 된다. */
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return reason;
}
