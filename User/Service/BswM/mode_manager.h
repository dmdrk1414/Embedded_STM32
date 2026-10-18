/*
 * mode_manager.h : 모드 관리자 (Mode Manager)
 *   - 함수 접두어 BswM_ = Basic Software Mode Manager (기본 소프트웨어 모드 관리자)
 *
 * 역할
 *   - "지금 잠들어도 되나?" 조건만 판단한다.
 *   - 조건이 맞으면 실제 잠들기 절차는 EcuM에 맡긴다. (EcuM_GoSleep)
 *   - AUTOSAR BswM의 "규칙(Rule) → 동작(Action)" 구조를 단순화한 것.
 */
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

/* main의 while (1)에서 계속 호출: 슬립 조건이 맞으면 EcuM_GoSleep() (돌아오지 않음) */
void BswM_MainFunction(void);

#endif /* MODE_MANAGER_H */
