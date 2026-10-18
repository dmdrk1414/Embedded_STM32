/*
 * wakeup_button.h : EXTI 웨이크업 감지 (PB0 버튼, EXTI0)
 *   - 함수 접두어 Wkup_ = Wakeup (웨이크업)
 *   - Sleep 실습용 모듈. 현재 main 루프에서는 사용하지 않는다.
 *   - AUTOSAR Icu 드라이버의 웨이크업 알림과 비슷한 역할
 */
#ifndef WAKEUP_BUTTON_H
#define WAKEUP_BUTTON_H

#include <stdint.h>

uint8_t Wkup_CheckAndClear(void);

#endif
