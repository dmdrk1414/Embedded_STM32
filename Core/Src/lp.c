#include "lp.h"
#include "main.h"

void Lp_EnterSleep(void)
{
    HAL_SuspendTick();   /* SysTick이 1ms마다 깨우지 않도록 (RM0008 p.72) */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();    /* 깨어난 뒤 HAL_Delay가 다시 동작하도록 */
}
