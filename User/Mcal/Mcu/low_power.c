/*
 * low_power.c : 저전력 모드 모듈 (Low Power)
 *   - 함수 접두어 Lp_ = Low Power (저전력)
 *
 * 역할
 *   - Sleep, Standby 진입과 Standby 복귀 판별
 *   - 백업 레지스터(BKP) 읽기/쓰기 도구 제공 (각 칸의 의미는 EcuM이 정함)
 *   - RH850에서는 STBC (Standby Controller)를 다루는 부분과 비슷한 역할
 */
#include "low_power.h"
#include "main.h"
#include "GDS.h"    /* LP_BKP_REG_COUNT */

/* ========================================================================== */
/* 내부 전용 함수 선언 (static) : 정의는 파일 맨 아래                                         */
/* ========================================================================== */
static void Lp_EnableBackupAccess(void);

/* ========================================================================== */
/* 외부 공개 함수 : 다른 파일에서 호출                                                      */
/* ========================================================================== */

/*
 * Lp_ReadBackupReg() : 백업 레지스터 한 칸 읽기
 *
 * [설명] BKP->DR1 ~ DR10 : 리셋과 Standby에도 지워지지 않는 16비트 칸
 *        (RM0008 6.1 p.81, 7.1.1 p.90)
 *        이 모듈은 "어떻게 읽는지"만 알고, 각 칸의 의미는 모른다.
 *        DR1 ~ DR10은 4바이트 간격으로 붙어 있어서 배열처럼 접근한다.
 * [인자] index : 칸 번호 1 ~ 10 (1 → DR1)
 * [반환] 저장된 값. index가 범위를 벗어나면 0
 * [호출] ecu_state_manager.c (웨이크 횟수, 잠들기 메모)
 * [예시]
 *     uint16_t cnt = Lp_ReadBackupReg(1u);              // DR1 읽기
 *     if (Lp_ReadBackupReg(2u) == 0x5A5Au) { ... }      // DR2 확인
 */
uint16_t Lp_ReadBackupReg(uint8_t index)
{
    uint16_t value = 0u;

    if ((index >= 1u) && (index <= LP_BKP_REG_COUNT))
    {
        Lp_EnableBackupAccess();
        value = (uint16_t)((&BKP->DR1)[index - 1u]);
    }
    return value;
}

/*
 * Lp_WriteBackupReg() : 백업 레지스터 한 칸 쓰기
 *
 * [설명] 쓴 값은 리셋과 Standby에도 남는다. (전원을 완전히 끄면 지워짐)
 *        이 모듈은 "어떻게 쓰는지"만 알고, 각 칸의 의미는 모른다.
 * [인자] index : 칸 번호 1 ~ 10 (1 → DR1). 범위를 벗어나면 아무것도 안 함
 *        value : 저장할 16비트 값
 * [반환] 없음
 * [호출] ecu_state_manager.c (웨이크 횟수, 잠들기 메모)
 * [예시]
 *     Lp_WriteBackupReg(2u, 0x5A5Au);   // DR2에 잠들기 메모
 *     Lp_WriteBackupReg(1u, 0u);        // DR1 지우기
 */
void Lp_WriteBackupReg(uint8_t index, uint16_t value)
{
    if ((index >= 1u) && (index <= LP_BKP_REG_COUNT))
    {
        Lp_EnableBackupAccess();
        (&BKP->DR1)[index - 1u] = value;
    }
}

/*
 * Lp_EnterSleep() : Sleep 모드 진입 (Sleep 실습용, 현재 미사용)
 *
 * [설명] CPU 코어 클럭만 멈추고, 주변장치(NVIC, EXTI, SysTick)는 계속 동작한다.
 *        (RM0008 5.3 Low-power modes, p.72)
 *        NVIC에서 허용된 인터럽트가 오면 깨어나서 "다음 줄부터" 이어서 실행한다.
 * [인자] 없음
 * [반환] 없음 (깨어나면 돌아옴)
 * [호출] 현재 없음 (예전 Sleep 실습의 while 루프에서 사용)
 * [예시]
 *     Lp_EnterSleep();                  // 버튼 인터럽트까지 여기서 멈춤
 *     if (Wkup_CheckAndClear() != 0u)   // 깨어난 이유 확인
 *     {
 *         ...
 *     }
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
 * Lp_EnterStandby() : Standby 모드 진입 (RH850 DeepSTOP과 비슷한 구조)
 *
 * [설명] 1.8V 영역 전원을 끄고 SRAM, 레지스터 내용이 사라진다.
 *        백업 레지스터(BKP)만 유지된다. (RM0008 5.3.5, p.76)
 *        PA0 (WKUP 핀) 상승 에지로 깨어나며, 깨어나면 리셋처럼 처음부터 시작한다.
 * [인자] 없음
 * [반환] 없음 (돌아오지 않음)
 * [호출] ecu_state_manager.c의 EcuM_EarlyInit() (IWDG 켜지기 전)
 * [예시]
 *     __HAL_RCC_CLEAR_RESET_FLAGS();
 *     Lp_EnterStandby();    // 여기서 잠듦, PA0를 누르면 main()부터 다시 시작
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
 * Lp_IsWakeFromStandby() : 방금 Standby에서 깨어났는지 확인 (확인 후 지움)
 *
 * [설명] SBF (Standby Flag, PWR_CSR bit 1): Standby에 들어가면 1이 되고,
 *        전원 ON 리셋이나 CSBF로만 지워진다. (RM0008 5.4.2, p.79)
 *        확인 후 지우지 않으면, 다음에 RESET 버튼을 눌러도
 *        계속 "Standby 복귀"로 잘못 판단하게 된다.
 *        읽으면서 지우므로 시작할 때 한 번만 호출해야 한다.
 * [인자] 없음
 * [반환] 1 = Standby 복귀, 0 = 그 외
 * [호출] reset_reason.c의 Rst_GetReason()
 * [예시]
 *     uint8_t wasStandby = Lp_IsWakeFromStandby();   // 결과를 저장해 두고 사용
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

/* ========================================================================== */
/* 내부 전용 함수 (static) : 이 파일 안에서만 호출                                           */
/* ========================================================================== */

/*
 * Lp_EnableBackupAccess() : 백업 레지스터를 읽고 쓸 수 있게 준비 (내부 전용)
 *
 * [설명] RCC_APB1ENR.BKPEN = 1 : 백업 레지스터에 클럭 공급
 *        PWR_CR.DBP = 1        : 쓰기 보호 해제 (리셋 후에는 쓰기 금지 상태)
 *        (RM0008 6.1, p.81)
 * [인자] 없음
 * [반환] 없음
 * [호출] Lp_ReadBackupReg(), Lp_WriteBackupReg()
 * [예시]
 *     Lp_EnableBackupAccess();
 *     BKP->DR1 = 0u;
 */
static void Lp_EnableBackupAccess(void)
{
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}
