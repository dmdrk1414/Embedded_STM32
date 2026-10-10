#include "lp.h"
#include "main.h"

void Lp_EnterSleep(void)
{
    HAL_SuspendTick();   /* SysTick이 1ms마다 깨우지 않도록 (RM0008 p.72) */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();    /* 깨어난 뒤 HAL_Delay가 다시 동작하도록 */
}

void Lp_EnterStandby(void)
{
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);   /* PA0 웨이크업 잠시 끄기     */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);           /* 이전 웨이크업 플래그 지우기 */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);    /* PA0 웨이크업 켜기 (EWUP)  */
    HAL_PWR_EnterSTANDBYMode();                  /* 잠듦: 이 아래는 실행 안 됨 */
}
