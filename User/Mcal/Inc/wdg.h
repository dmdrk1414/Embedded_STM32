/*
 * wdg.h : 워치독 드라이버 (IWDG, Independent Watchdog)
 *   - AUTOSAR Wdg 드라이버에 해당
 *   - 타임아웃: Prescaler 4, Reload 999 → 약 67~133ms (LSI 60~30kHz)
 */
#ifndef WDG_H
#define WDG_H

/* "이상 없음" 신호: 워치독 카운터를 Reload 값으로 다시 채운다 */
void Wdg_Refresh(void);

#endif /* WDG_H */
