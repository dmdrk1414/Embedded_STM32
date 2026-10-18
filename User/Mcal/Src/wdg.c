/*
 * wdg.c : 워치독 드라이버 구현
 *   - IWDG는 MX_IWDG_Init()에서 시작된다. (IWDG_KR = 0xCCCC)
 *   - 한번 시작하면 리셋 전까지 끌 수 없다. (RM0008 19.3, p.494)
 */
#include "wdg.h"
#include "main.h"

extern IWDG_HandleTypeDef hiwdg;   /* CubeMX가 main.c에 만든 핸들 */

void Wdg_Refresh(void)
{
    (void)HAL_IWDG_Refresh(&hiwdg);   /* IWDG_KR = 0xAAAA → 카운터를 999로 다시 채움 */
}
