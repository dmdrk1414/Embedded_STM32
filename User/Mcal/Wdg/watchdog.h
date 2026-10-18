/*
 * watchdog.h : 워치독 드라이버 (IWDG, Independent Watchdog)
 *   - 함수 접두어 Wdg_ = Watchdog (워치독)
 *   - AUTOSAR Wdg 드라이버에 해당
 *   - 타임아웃: Prescaler 4, Reload 999 → 약 67~133ms (LSI 60~30kHz)
 */
#ifndef WATCHDOG_H
#define WATCHDOG_H

/* "이상 없음" 신호: 워치독 카운터를 Reload 값으로 다시 채운다 */
void Wdg_Refresh(void);

#endif /* WATCHDOG_H */
