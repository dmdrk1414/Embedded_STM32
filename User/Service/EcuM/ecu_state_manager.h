/*
 * ecu_state_manager.h : ECU 상태 관리 모듈 (ECU State Manager)
 *   - 함수 접두어 EcuM_ = ECU State Manager (ECU 상태 관리자)
 *
 * 역할
 *   - "시작 절차"와 "잠들기 절차"만 담당한다.
 *   - 잠들지 말지 판단은 BswM(mode_manager.c), LED 표시는 Indicator(indicator.c)가 한다.
 *   - AUTOSAR EcuM을 단순화한 구조.
 *
 * 사용 순서 (main.c)
 *   EcuM_EarlyInit();            USER CODE SysInit: MX_IWDG_Init() 보다 먼저
 *   MX_..._Init();               CubeMX 초기화 (여기서 IWDG 시작)
 *   EcuM_Init();                 USER CODE 2: 리셋 원인 판단, 유지 메모리 처리
 *   Indicator_ShowStartup(EcuM_GetStartupReason(), EcuM_GetWakeCount());
 *   while (1) { ...; BswM_MainFunction(); }   → 조건이 맞으면 EcuM_GoSleep()
 */
#ifndef ECU_STATE_MANAGER_H
#define ECU_STATE_MANAGER_H

#include <stdint.h>
#include "GDS.h"    /* Rst_ReasonType */

/* 리셋 직후, 워치독 시작 전에 호출: 잠들기 요청이 있으면 바로 Standby */
void EcuM_EarlyInit(void);

/* 시작 처리: 리셋 원인 판단, 유지 메모리(웨이크 횟수) 처리 (LED 표시는 하지 않음) */
void EcuM_Init(void);

/* 잠들기 절차 시작: 메모를 남기고 리셋 (돌아오지 않음). BswM이 호출 */
void EcuM_GoSleep(void);

/* EcuM_Init()이 판단한 이번 시작의 리셋 원인 */
Rst_ReasonType EcuM_GetStartupReason(void);

/* 현재 웨이크업 횟수 (BKP->DR1) */
uint16_t EcuM_GetWakeCount(void);

#endif /* ECU_STATE_MANAGER_H */
