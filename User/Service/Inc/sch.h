/*
 * sch.h : 주기 실행 스케줄러 (Scheduler)
 *
 * 역할
 *   - TIM2 1µs 카운터 값을 while문에서 확인해 10ms마다 Task_10ms()를 호출한다.
 *   - 인터럽트를 쓰지 않는 "타이머 체크" 방식.
 *   - AUTOSAR OS의 Counter(틱) + Alarm(주기) → Task 활성화에 해당.
 */
#ifndef SCH_H
#define SCH_H

/* TIM2 시작. main에서 while (1) 들어가기 전에 한 번 호출 */
void Sch_Init(void);

/* main의 while (1) 안에서 쉬지 않고 계속 호출 */
void Sch_Run(void);

#endif /* SCH_H */
