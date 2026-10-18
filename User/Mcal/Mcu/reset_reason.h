/*
 * reset_reason.h : 리셋 원인 판별 모듈 (Reset)
 *   - 함수 접두어 Rst_ = Reset (리셋 원인)
 *
 * 역할
 *   - MCU가 "왜 처음부터 시작했는지" 판별한다.
 *   - AUTOSAR Mcu 드라이버의 Mcu_GetResetReason()과 같은 역할.
 *   - RH850에서는 RESF (Reset Factor, 리셋 요인) 레지스터를 읽는 부분에 해당.
 *
 * 근거 문서
 *   - RM0008 7.1   Reset (p.90~91)         : 리셋 종류 설명
 *   - RM0008 7.3.10 RCC_CSR (p.119~120)    : 리셋 원인 플래그 비트
 *   - RM0008 5.4.2 PWR_CSR (p.79)          : SBF (Standby 복귀 플래그)
 */
#ifndef RESET_REASON_H
#define RESET_REASON_H

#include "GDS.h"    /* Rst_ReasonType (리셋 원인 enum) */

/*
 * 리셋 원인을 판별해서 돌려준다.
 * 주의: 내부에서 플래그를 지우므로 main 시작 시 "한 번만" 호출할 것.
 *       두 번째 호출부터는 플래그가 지워져 RST_UNKNOWN이 나온다.
 */
Rst_ReasonType Rst_GetReason(void);

#endif /* RESET_REASON_H */
