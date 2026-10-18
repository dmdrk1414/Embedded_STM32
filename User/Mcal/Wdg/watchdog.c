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

/*
 * Wdg_Refresh() : 워치독에 "이상 없음" 신호 보내기
 *
 * [설명] IWDG_KR = 0xAAAA → 카운터를 Reload 값(999)으로 다시 채운다.
 *        약 67 ~ 133ms 안에 다시 부르지 않으면 MCU가 리셋된다.
 * [인자] 없음
 * [반환] 없음
 * [호출] scheduler.c의 Sch_Run() (Task_10ms() 끝난 뒤, 10ms마다)
 *        scheduler.c의 Sch_DelayMs() (시작 단계 대기 중)
 * [예시]
 *     Task_10ms();
 *     Wdg_Refresh();     // Task가 끝까지 돌아왔을 때만
 */
void Wdg_Refresh(void)
{
    (void)HAL_IWDG_Refresh(&hiwdg);   /* IWDG_KR = 0xAAAA → 카운터를 999로 다시 채움 */
}
