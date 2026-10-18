/*
 * indicator.h : 상태 표시 모듈 (LED)
 *   - 함수 접두어 Indicator_ = Indicator (표시기)
 *
 * 역할
 *   - "무엇을 보여줄지"만 담당한다. (LED 패턴)
 *   - 리셋 원인과 웨이크 횟수는 EcuM이 판단하고, 이 모듈은 받아서 표시만 한다.
 */
#ifndef INDICATOR_H
#define INDICATOR_H

#include <stdint.h>
#include "GDS.h"    /* Rst_ReasonType */

/* 시작할 때 리셋 원인별 LED 패턴을 보여주고, 끝나면 LED를 끈다 */
void Indicator_ShowStartup(Rst_ReasonType reason, uint16_t wakeCount);

#endif /* INDICATOR_H */
