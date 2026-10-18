/*
 * ecu_state_manager.h : ECU 상태 관리 모듈 (ECU State Manager)
 *   - 함수 접두어 EcuM_ = ECU State Manager (ECU 상태 관리자)
 *
 * 역할
 *   - ECU의 "시작 → 동작 → 잠들기" 흐름을 한곳에서 관리한다.
 *   - AUTOSAR EcuM을 단순화한 구조.
 *
 * 사용 순서 (main.c)
 *   EcuM_EarlyInit();            USER CODE SysInit: MX_IWDG_Init() 보다 먼저
 *   MX_..._Init();               CubeMX 초기화 (여기서 IWDG 시작)
 *   EcuM_Init();                 USER CODE 2: 하드웨어 초기화 직후 한 번
 *   while (1) { ...; EcuM_MainFunction(); }
 */
#ifndef ECU_STATE_MANAGER_H
#define ECU_STATE_MANAGER_H

/* 시작 처리: 리셋 원인 판단, 유지 메모리(웨이크 횟수) 처리, 시작 표시 */
void EcuM_Init(void);

/* while (1)에서 계속 호출: 슬립 조건이 맞으면 잠들기 전 리셋을 건다 (돌아오지 않음)
 * 실제 Standby 진입은 리셋 직후 EcuM_EarlyInit()에서 한다. */
void EcuM_MainFunction(void);

/* 리셋 직후, 워치독 시작 전에 호출: 잠들기 요청이 있으면 바로 Standby */
void EcuM_EarlyInit(void);

#endif /* ECU_STATE_MANAGER_H */
