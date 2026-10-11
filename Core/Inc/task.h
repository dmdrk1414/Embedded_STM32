/*
 * task.h : 주기 Task 모음
 *   - AUTOSAR OS의 주기 Task (예: OsTask_10ms)에 해당
 */
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

/* 10ms마다 Sch_Run()이 호출한다 */
void Task_10ms(void);

#endif /* TASK_H */
