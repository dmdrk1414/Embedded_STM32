/*
 * lp.c : 저전력 모드 모듈 (Low Power)
 *
 * 역할
 *   - Sleep, Standby 진입과 Standby 복귀 판별, 백업 레지스터 관리
 *   - RH850에서는 STBC (Standby Controller)를 다루는 부분에 해당
 */
#include "lp.h"
#include "main.h"

/*
 * Lp_EnterSleep() : Sleep 모드 진입
 *
 * - CPU 코어 클럭만 멈추고, 주변장치(NVIC, EXTI, SysTick)는 계속 동작한다.
 *   (RM0008 5.3 Low-power modes, p.72)
 * - NVIC에서 허용된 인터럽트가 오면 깨어나서 "다음 줄부터" 이어서 실행한다.
 */
void Lp_EnterSleep(void)
{
    /* SysTick 인터럽트 끄기 (STK_CTRL.TICKINT = 0, PM0056 4.5.1 p.151)
     * 끄지 않으면 SysTick이 1ms마다 CPU를 깨워 버린다. */
    HAL_SuspendTick();

    /* SCB->SCR.SLEEPDEEP = 0 으로 설정 후 WFI 실행 → Sleep 진입
     * 버튼(EXTI0) 인터럽트가 오기 전까지 이 줄에서 멈춰 있다. */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* 깨어난 뒤 SysTick 인터럽트 다시 켜기 (TICKINT = 1)
     * 켜지 않으면 HAL_Delay()가 끝나지 않는다. */
    HAL_ResumeTick();
}

/*
 * Lp_EnterStandby() : Standby 모드 진입 (RH850 DeepSTOP에 해당)
 *
 * - 1.8V 영역 전원을 끄고 SRAM, 레지스터 내용이 사라진다.
 *   백업 레지스터(BKP)만 유지된다. (RM0008 5.3.5, p.76)
 * - PA0 (WKUP 핀) 상승 에지로 깨어나며, 깨어나면 리셋처럼 처음부터 시작한다.
 * - 이 함수는 돌아오지 않는다 (return 없음).
 */
void Lp_EnterStandby(void)
{
    /* ① WKUP 핀 기능을 잠시 끈다 (PWR_CSR.EWUP = 0)
     *    아래 ②에서 플래그를 깨끗하게 지우기 위한 준비 */
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);

    /* ② 웨이크업 플래그 지우기 (PWR_CR.CWUF = 1 → PWR_CSR.WUF = 0)
     *    RM0008 Table 15 (p.76): WUF가 지워진 상태여야 Standby에 들어간다.
     *    남아 있으면 잠들지 않고 바로 깨어날 수 있다. */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* ③ WKUP 핀 기능 켜기 (PWR_CSR.EWUP = 1)
     *    RM0008 5.4.2 (p.79): 이 순간 PA0는 GPIO 설정과 상관없이
     *    입력 Pull-down으로 "강제"되고, 상승 에지로 깨어날 수 있게 된다. */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

    /* ④ Standby 진입 (SLEEPDEEP = 1, PWR_CR.PDDS = 1, 그리고 WFI)
     *    여기서 전원이 대부분 꺼지므로 이 아래 코드는 실행되지 않는다. */
    HAL_PWR_EnterSTANDBYMode();
}

/*
 * Lp_IsWakeFromStandby() : 방금 Standby에서 깨어났는지 확인
 *
 * - SBF (Standby Flag, PWR_CSR bit 1): Standby에 들어갔다 나오면 1이 된다.
 *   (RM0008 5.3.5, p.76)
 * - 확인 후 지우지 않으면, 다음에 RESET 버튼을 눌러도
 *   계속 "Standby 복귀"로 잘못 판단하게 된다.
 * - 반환값: 1 = Standby 복귀, 0 = 그 외
 */
uint8_t Lp_IsWakeFromStandby(void)
{
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);   /* PWR_CR.CSBF = 1 → SBF 지움 */
        return 1u;
    }
    return 0u;
}

/*
 * Lp_IncWakeCount() : 웨이크업 횟수를 백업 레지스터에 1 증가
 *
 * - BKP->DR1 : 백업 데이터 레지스터 1번 (16비트 값 하나 저장)
 * - Standby에서 깨어나도 지워지지 않는다. (RM0008 6.1, p.81)
 *   → RH850 DeepSTOP의 Retention RAM과 같은 역할
 * - 반환값: 증가된 후의 횟수
 */
uint16_t Lp_IncWakeCount(void)
{
    /* 백업 레지스터 주변장치에 클럭 공급 (RCC_APB1ENR.BKPEN = 1)
     * 클럭이 없으면 레지스터를 읽고 쓸 수 없다. */
    __HAL_RCC_BKP_CLK_ENABLE();

    /* 백업 영역 쓰기 보호 해제 (PWR_CR.DBP = 1)
     * 리셋 후에는 실수로 덮어쓰지 않도록 쓰기 금지 상태다. (RM0008 6.1, p.81) */
    HAL_PWR_EnableBkUpAccess();

    /* 현재 값 + 1 을 다시 저장. DR1은 16비트이므로 (uint16_t)로 맞춘다. */
    BKP->DR1 = (uint16_t)(BKP->DR1 + 1u);

    return (uint16_t)BKP->DR1;
}

/* 웨이크업 횟수 초기화: 전원 ON일 때만 호출 */
void Lp_ClearWakeCount(void)
{
    __HAL_RCC_BKP_CLK_ENABLE();      /* 백업 레지스터 클럭 켜기 */
    HAL_PWR_EnableBkUpAccess();      /* 쓰기 보호 풀기 (PWR_CR.DBP = 1) */
    BKP->DR1 = 0u;                   /* 횟수를 0으로 */
}
