/*
 * watchdog.c : 워치독 드라이버 구현
 *   - 함수 접두어 Wdg_ = Watchdog (워치독)
 *   - IWDG는 MX_IWDG_Init()에서 시작된다. (IWDG_KR = 0xCCCC)
 *   - 한번 시작하면 리셋 전까지 끌 수 없다. (RM0008 5.3.4, p.75)
 *   - IWDG의 클럭(LSI)도 강제로 켜져 끌 수 없다. (RM0008 7.2.9, p.98)
 */
#include "watchdog.h"
#include "main.h"

extern IWDG_HandleTypeDef hiwdg;   /* CubeMX가 main.c에 만든 핸들 */

void Wdg_Refresh(void)
{
    (void)HAL_IWDG_Refresh(&hiwdg);   /* IWDG_KR = 0xAAAA → 카운터를 999로 다시 채움 */
}
