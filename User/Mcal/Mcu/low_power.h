/*
 * low_power.h : 저전력 모드 모듈 (Low Power)
 *   - 함수 접두어 Lp_ = Low Power (저전력)
 */
#ifndef LOW_POWER_H
#define LOW_POWER_H

#include <stdint.h>

void     Lp_EnterSleep(void);
void     Lp_EnterStandby(void);
uint8_t  Lp_IsWakeFromStandby(void);
uint16_t Lp_IncWakeCount(void);
void     Lp_ClearWakeCount(void);
void     Lp_SetSleepRequest(void);    /* 리셋 후 바로 잠들라는 표시 남기기 */
uint8_t  Lp_TakeSleepRequest(void);   /* 표시가 있으면 1을 돌려주고 지움   */

#endif
