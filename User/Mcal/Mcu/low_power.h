/*
 * low_power.h : 저전력 모드 모듈 (Low Power)
 *   - 함수 접두어 Lp_ = Low Power (저전력)
 */
#ifndef LOW_POWER_H
#define LOW_POWER_H

#include <stdint.h>

void     Lp_EnterSleep(void);          /* Sleep 진입, 인터럽트로 깨면 돌아옴 (현재 미사용) */
void     Lp_EnterStandby(void);        /* Standby 진입, 돌아오지 않음 (PA0로 깨면 리셋)   */
uint8_t  Lp_IsWakeFromStandby(void);   /* 1 = Standby 복귀 (SBF 확인 후 지움)             */

/* 백업 레지스터 읽기/쓰기 (index: 1 ~ 10 → BKP->DR1 ~ DR10)
 * 각 칸의 의미는 이 모듈이 아니라 사용하는 쪽(EcuM)이 정한다. */
uint16_t Lp_ReadBackupReg(uint8_t index);
void     Lp_WriteBackupReg(uint8_t index, uint16_t value);

#endif
