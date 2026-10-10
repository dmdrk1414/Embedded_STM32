#ifndef LP_H
#define LP_H

#include <stdint.h>

void     Lp_EnterSleep(void);
void     Lp_EnterStandby(void);
uint8_t  Lp_IsWakeFromStandby(void);
uint16_t Lp_IncWakeCount(void);
void     Lp_ClearWakeCount(void);

#endif
