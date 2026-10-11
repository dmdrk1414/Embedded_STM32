/*
 * task.h : 주기 Task 모음
 *   - AUTOSAR OS의 주기 Task (예: OsTask_10ms)에 해당
 */
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

/* 10ms마다 Sch_Run()이 호출한다 */
void Task_10ms(void);

/* 슬립 요청 확인: 1 = 5초 동안 입력 없음 → 잠들 시간 */
uint8_t Task_IsSleepRequested(void);

#endif /* TASK_H */
