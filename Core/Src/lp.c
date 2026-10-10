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

/*
 * SBF (Standby Flag, 대기 모드 플래그)
 * "방금 Standby에서 깨어났음" 표시
 * RM0008 5.3.5, p.76
 * */
uint8_t Lp_IsWakeFromStandby(void)
{
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);   /* 확인했으면 지우기 */
        return 1u;
    }
    return 0u;
}

/* BKP	Backup registers (백업 레지스터) 주변장치
 * DR1	Data Register 1 (데이터 레지스터 1번). 16비트 값 하나를 저장하는 칸
 * BKP->DR1 + 1u	지금 저장된 값에 1을 더함
 * (uint16_t)	결과를 16비트로 맞춤
 *
 * BKP->DR1 (백업 데이터 레지스터
 * 1)	Standby 중에도 값이 지워지지 않는 저장소
 * RM0008 6.1, p.81
*/
uint16_t Lp_IncWakeCount(void)
{
    __HAL_RCC_BKP_CLK_ENABLE();          /* 백업 레지스터 클럭 켜기 */
    HAL_PWR_EnableBkUpAccess();          /* 쓰기 보호 풀기 */
    BKP->DR1 = (uint16_t)(BKP->DR1 + 1u);
    return (uint16_t)BKP->DR1;
}
