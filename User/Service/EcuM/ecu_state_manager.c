/*
 * ecu_state_manager.c : ECU 상태 관리 모듈 구현
 *   - 함수 접두어 EcuM_ = ECU State Manager (ECU 상태 관리자)
 *
 * 역할 : "시작 절차"와 "잠들기 절차"만 담당
 *   - 잠들지 말지 판단 → mode_manager.c (BswM)
 *   - LED 표시         → indicator.c (Indicator)
 *
 * 상태 흐름
 *   [리셋/웨이크업] → EcuM_EarlyInit() (메모 없음) → MX_IWDG_Init()
 *                  → EcuM_Init() → [RUN: 10ms Task 동작, IWDG ON]
 *                                      │ 슬립 요청 (5초 무입력)
 *                                      ▼
 *                    BswM_MainFunction() → EcuM_GoSleep()
 *                                          (BKP->DR2 메모 + 소프트웨어 리셋)
 *                                      │ 리셋 → IWDG OFF
 *                                      ▼
 *                    EcuM_EarlyInit() (메모 있음) → [Standby, IWDG OFF]
 *                                      │ PA0 상승 에지
 *                                      ▼
 *                                [리셋/웨이크업] ...
 */
#include "ecu_state_manager.h"
#include "reset_reason.h"    /* Rst_GetReason()                            */
#include "low_power.h"       /* Lp_EnterStandby(), 백업 레지스터 읽기/쓰기 */
#include "main.h"            /* NVIC_SystemReset, __HAL_RCC_CLEAR_RESET_FLAGS */
#include "GDS.h"             /* ECUM_BKP_WAKE_COUNT, ECUM_BKP_SLEEP_REQ, ECUM_SLEEP_REQ_MAGIC */

/*
 * 백업 레지스터 사용 계획 (값은 GDS.h에 정의)
 *   DR1 : 웨이크업 횟수 (PA0로 깨어난 횟수)
 *   DR2 : 잠들기 요청 메모 (0x5A5A = 다음 시작 때 바로 Standby)
 *   - RH850 DeepSTOP의 Retention RAM과 비슷한 역할
 */

/* EcuM_Init()이 판단한 결과를 기억해 두고, 다른 모듈이 읽어 간다 */
static Rst_ReasonType s_startupReason = RST_UNKNOWN;   /* 이번 시작의 리셋 원인 */
static uint16_t       s_wakeCount     = 0u;            /* 현재 웨이크업 횟수    */

/* ========================================================================== */
/* 내부 전용 함수 선언 (static) : 정의는 파일 맨 아래                         */
/* ========================================================================== */
static uint16_t EcuM_IncWakeCount(void);
static void     EcuM_ClearWakeCount(void);
static uint8_t  EcuM_TakeSleepRequest(void);

/* ========================================================================== */
/* 외부 공개 함수 : 다른 파일에서 호출                                       */
/* ========================================================================== */

/*
 * EcuM_EarlyInit() : 리셋 직후 가장 먼저 실행 (MX_IWDG_Init 보다 앞)
 *
 * [설명] 잠들기 요청 메모가 있으면, IWDG가 꺼진 상태 그대로 Standby 진입.
 *        메모가 없으면 아무것도 하지 않고 돌아가서 평소처럼 초기화.
 * [인자] 없음
 * [반환] 없음 (메모가 있으면 Standby로 가서 돌아오지 않음)
 * [호출] main.c USER CODE SysInit (반드시 MX_IWDG_Init()보다 위)
 * [예시]
 *     SystemClock_Config();
 *     EcuM_EarlyInit();   // 메모 있으면 여기서 잠듦
 *     MX_GPIO_Init();
 *     MX_IWDG_Init();     // 메모 없을 때만 여기까지 옴
 */
void EcuM_EarlyInit(void)
{
    if (EcuM_TakeSleepRequest() != 0u)
    {
        /* 이번 리셋은 "잠들기 위한 리셋"이므로 리셋 플래그(SFTRSTF)를 정리해 둔다.
         * 그래야 PA0로 깨어났을 때 원인 판단이 깔끔하다. */
        __HAL_RCC_CLEAR_RESET_FLAGS();
        Lp_EnterStandby();                /* 여기서 잠듦 (IWDG 없이) */
    }
}

/*
 * EcuM_Init() : 시작 처리 (리셋 원인 판단 + 유지 메모리 처리)
 *
 * [설명] Standby에서 깨어나도 리셋처럼 처음부터 시작하므로,
 *        "왜 시작했는지"에 따라 유지 메모리(웨이크 횟수)를 다르게 처리한다.
 *          RST_STANDBY_WAKE : 웨이크 횟수 +1
 *          전원 ON 등       : 웨이크 횟수 0으로 초기화
 *          그 외            : 웨이크 횟수 유지
 *        LED 표시는 하지 않는다. 결과는 EcuM_GetStartupReason(),
 *        EcuM_GetWakeCount()로 꺼내서 Indicator가 표시한다.
 * [인자] 없음
 * [반환] 없음
 * [호출] main.c USER CODE 2 (MX_..._Init() 다음, Sch_Init() 전에 한 번)
 * [예시]
 *     EcuM_Init();
 *     Indicator_ShowStartup(EcuM_GetStartupReason(), EcuM_GetWakeCount());
 */
void EcuM_Init(void)
{
    s_startupReason = Rst_GetReason();   /* 플래그를 지우므로 여기서 한 번만 호출 */

    switch (s_startupReason)
    {
        case RST_STANDBY_WAKE:
            /* 로컬 웨이크업 (PA0): 횟수 +1 */
            s_wakeCount = EcuM_IncWakeCount();
            break;

        case RST_PIN:
        case RST_STANDBY_WATCHDOG:
        case RST_WATCHDOG:
            /* RESET 버튼, 워치독 리셋: 웨이크 횟수는 유지 */
            s_wakeCount = Lp_ReadBackupReg(ECUM_BKP_WAKE_COUNT);
            break;

        default:
            /* 전원 ON 등: 유지 메모리를 명시적으로 초기화 */
            EcuM_ClearWakeCount();
            s_wakeCount = 0u;
            break;
    }
}

/*
 * EcuM_GoSleep() : 잠들기 절차 시작 → 소프트웨어 리셋 (돌아오지 않음)
 *
 * [설명] 여기서는 잠들지 않는다. BKP->DR2에 "다음 시작 때 바로 잠들어라"
 *        메모(0x5A5A)를 남긴 뒤 리셋한다. (LED는 리셋되면서 꺼진다)
 *        실제 Standby 진입은 리셋 직후 EcuM_EarlyInit()이 한다.
 * [인자] 없음
 * [반환] 없음 (리셋되므로 돌아오지 않음)
 * [호출] mode_manager.c의 BswM_MainFunction() (슬립 조건이 맞았을 때)
 * [예시]
 *     if (Task_IsSleepRequested() != 0u)
 *     {
 *         EcuM_GoSleep();    // 이 아래 코드는 실행되지 않음
 *     }
 *
 *   [왜 바로 Standby에 들어가지 않고 리셋하는가]
 *   - IWDG는 한번 켜지면 끌 수 없고 (RM0008 5.3.4, p.75),
 *     LSI 클럭도 끊을 수 없다. (RM0008 7.2.9, p.98)
 *   - 그대로 Standby에 들어가면 IWDG가 약 0.1초 만에 깨운다.
 *   - 리셋하면 IWDG도 꺼지므로, 리셋 직후 IWDG가 켜지기 전에 잠든다.
 *   - 회사 제품의 예전 방식 "슬립 전 리셋"과 같은 구조.
 */
void EcuM_GoSleep(void)
{
    Lp_WriteBackupReg(ECUM_BKP_SLEEP_REQ, ECUM_SLEEP_REQ_MAGIC); /* "다음 시작 때 바로 잠들어라" */
    NVIC_SystemReset();                                          /* 소프트웨어 리셋 (IWDG 꺼짐) */
}

/*
 * EcuM_GetStartupReason() : 이번 시작의 리셋 원인 읽기
 *
 * [설명] EcuM_Init()에서 Rst_GetReason()으로 판단해 저장해 둔 값을 돌려준다.
 *        Rst_GetReason()은 한 번만 호출해야 하므로, 다른 모듈은 이 함수를 쓴다.
 * [인자] 없음
 * [반환] Rst_ReasonType (GDS.h). EcuM_Init() 전에는 RST_UNKNOWN
 * [호출] main.c (Indicator_ShowStartup()에 넘길 때)
 * [예시]
 *     if (EcuM_GetStartupReason() == RST_WATCHDOG) { ... }   // 고장 기록 등
 */
Rst_ReasonType EcuM_GetStartupReason(void)
{
    return s_startupReason;
}

/*
 * EcuM_GetWakeCount() : 현재 웨이크업 횟수 읽기
 *
 * [설명] EcuM_Init()에서 처리한 뒤의 웨이크 횟수 (BKP->DR1 값)
 * [인자] 없음
 * [반환] 웨이크업 횟수 (전원 ON 후 PA0로 깨어난 횟수)
 * [호출] main.c (Indicator_ShowStartup()에 넘길 때)
 * [예시]
 *     uint16_t n = EcuM_GetWakeCount();   // 3번째 웨이크업이면 3
 */
uint16_t EcuM_GetWakeCount(void)
{
    return s_wakeCount;
}

/* ========================================================================== */
/* 내부 전용 함수 (static) : 이 파일 안에서만 호출                           */
/* ========================================================================== */

/*
 * EcuM_IncWakeCount() : 웨이크업 횟수를 1 올린다 (내부 전용)
 *
 * [설명] BKP->DR1(웨이크 횟수)을 읽어서 +1 한 뒤 다시 저장한다.
 *        리셋과 Standby에도 값이 남는다.
 * [인자] 없음
 * [반환] 증가된 후의 횟수 (1, 2, 3 ...)
 * [호출] EcuM_Init()의 RST_STANDBY_WAKE (PA0로 깨어났을 때)
 * [예시]
 *     s_wakeCount = EcuM_IncWakeCount();   // 처음 깨면 1, 두 번째면 2
 */
static uint16_t EcuM_IncWakeCount(void)
{
    uint16_t cnt = (uint16_t)(Lp_ReadBackupReg(ECUM_BKP_WAKE_COUNT) + 1u);
    Lp_WriteBackupReg(ECUM_BKP_WAKE_COUNT, cnt);
    return cnt;
}

/*
 * EcuM_ClearWakeCount() : 웨이크업 횟수를 0으로 지운다 (내부 전용)
 *
 * [설명] BKP->DR1 = 0. 유지 메모리는 전원 ON일 때 명시적으로 초기화하는 게 규칙.
 *        (RESET 버튼, 코드 다운로드로는 BKP가 지워지지 않기 때문)
 * [인자] 없음
 * [반환] 없음
 * [호출] EcuM_Init()의 default (전원 ON 등)
 * [예시]
 *     EcuM_ClearWakeCount();   // 이후 첫 PA0 웨이크업은 1부터 셈
 */
static void EcuM_ClearWakeCount(void)
{
    Lp_WriteBackupReg(ECUM_BKP_WAKE_COUNT, 0u);
}

/*
 * EcuM_TakeSleepRequest() : 잠들기 요청 메모를 확인하고 지운다 (내부 전용)
 *
 * [설명] BKP->DR2가 0x5A5A이면 메모를 지우고 1을 돌려준다.
 *        확인하면서 지우므로 한 번만 동작한다. (계속 잠드는 일 방지)
 * [인자] 없음
 * [반환] 1 = 메모 있음 (바로 잠들어야 함), 0 = 메모 없음 (평소처럼 시작)
 * [호출] EcuM_EarlyInit()
 * [예시]
 *     if (EcuM_TakeSleepRequest() != 0u)
 *     {
 *         Lp_EnterStandby();   // IWDG 켜지기 전에 잠듦
 *     }
 */
static uint8_t EcuM_TakeSleepRequest(void)
{
    uint8_t req = 0u;

    if (Lp_ReadBackupReg(ECUM_BKP_SLEEP_REQ) == ECUM_SLEEP_REQ_MAGIC)
    {
        Lp_WriteBackupReg(ECUM_BKP_SLEEP_REQ, 0u);
        req = 1u;
    }
    return req;
}
