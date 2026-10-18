/*
 * mode_manager.c : 모드 관리자 구현
 *   - 함수 접두어 BswM_ = Basic Software Mode Manager (기본 소프트웨어 모드 관리자)
 *
 * 역할
 *   - 여러 모듈의 요청을 모아서 "잠들어도 되나?"를 판단한다.
 *   - 판단만 하고, 잠들기 절차(메모 + 리셋)는 EcuM이 한다.
 *
 * 지금의 슬립 규칙
 *   Task가 슬립을 요청했다 (PA0 5초 무입력)  →  EcuM_GoSleep()
 *
 * 조건이 늘어나면 (예: 통신 없음, 저장 완료) 여기에 AND로 추가한다.
 */
#include "mode_manager.h"
#include "task.h"                /* Task_IsSleepRequested() */
#include "ecu_state_manager.h"   /* EcuM_GoSleep()          */

/* ========================================================================== */
/* 외부 공개 함수 : 다른 파일에서 호출                                       */
/* ========================================================================== */

/*
 * BswM_MainFunction() : 슬립 조건 판단
 *
 * [설명] 지금 조건: Task가 슬립을 요청했는가 (PA0 5초 무입력)
 *        조건이 맞으면 EcuM_GoSleep() → 잠들기 전 리셋 (돌아오지 않음)
 *        조건이 늘어나면 여기에 AND로 추가한다. (AUTOSAR BswM의 규칙 역할)
 * [인자] 없음
 * [반환] 없음 (잠들 때는 돌아오지 않음)
 * [호출] main.c의 while (1) 안에서 Sch_Run() 다음에 계속
 * [예시]
 *     while (1)
 *     {
 *         Sch_Run();
 *         BswM_MainFunction();   // 슬립 조건 확인
 *     }
 *
 *     조건 추가 예 (나중에):
 *     if ((Task_IsSleepRequested() != 0u) && (Com_IsIdle() != 0u))
 */
void BswM_MainFunction(void)
{
    if (Task_IsSleepRequested() != 0u)
    {
        EcuM_GoSleep();
    }
}
