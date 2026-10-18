/*
 * scheduler.c : 주기 실행 스케줄러 구현 (일반 타이머 카운트 방식)
 *   - 함수 접두어 Sch_ = Scheduler (스케줄러)
 *
 * TIM2 설정 (CubeMX)
 *   - 타이머 클럭 8MHz, PSC = 7  → 1MHz (1칸 = 1µs)
 *   - ARR = 65535                → 0~65535를 계속 셈 (한 바퀴 = 65.536ms)
 *   - 인터럽트 사용 안 함 (ISR에는 로직을 넣지 않는다)
 *
 * 동작
 *   - while문에서 카운터 값(TIM2->CNT)을 읽어
 *     지난번 실행 시각과의 차이가 10000µs(=10ms) 이상이면 Task_10ms() 실행.
 */
#include "scheduler.h"
#include "task.h"
#include "main.h"

#define SCH_PERIOD_10MS_US   (10000u)   /* 10ms = 1µs × 10000 */

/* CubeMX가 main.c에 만든 TIM2 핸들 */
extern TIM_HandleTypeDef htim2;

/* 마지막으로 Task_10ms()를 실행한 기준 카운터 값 (단위: µs, 16비트) */
static uint16_t s_last10ms = 0u;

/* Live Expressions 확인용: 0이 아니면 10ms 주기를 놓친 적이 있다는 뜻 */
volatile uint32_t g_schOverrun = 0u;

/*
 * Sch_Init() : TIM2 카운트 시작 (인터럽트 없이)
 *   HAL_TIM_Base_Start() : TIMx_CR1.CEN = 1 → 카운터만 돌린다.
 */
void Sch_Init(void)
{
    (void)HAL_TIM_Base_Start(&htim2);
    s_last10ms = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);   /* 시작 기준 시각 */
}

/*
 * Sch_Run() : while (1) 안에서 계속 호출
 *
 * [16비트 부호 없는 뺄셈을 쓰는 이유]
 *   카운터는 65535 다음에 0으로 돌아간다.
 *   예) 지난번 60000, 지금 4464 → (uint16_t)(4464 - 60000) = 10000
 *   → 한 바퀴를 넘어가도 경과 시간이 올바르게 계산된다.
 *   단, while문이 65ms 넘게 막히면 한 바퀴를 통째로 놓쳐서 계산이 틀어진다.
 *
 * [s_last10ms += 10000 을 하는 이유]
 *   "지금 값"을 저장하면 늦게 확인한 만큼 주기가 계속 밀린다.
 *   +10000을 하면 10ms 간격 기준이 유지되어 오차가 쌓이지 않는다.
 */
void Sch_Run(void)
{
    uint16_t now = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);   /* TIM2->CNT 읽기 */
    uint16_t elapsed = (uint16_t)(now - s_last10ms);

    if (elapsed >= SCH_PERIOD_10MS_US)
    {
        /* 20ms 이상 지남 = 한 주기 이상 늦었다 (while문이 막혔다는 증거) */
        if (elapsed >= (uint16_t)(2u * SCH_PERIOD_10MS_US))
        {
            g_schOverrun++;
        }

        s_last10ms = (uint16_t)(s_last10ms + SCH_PERIOD_10MS_US);

        /* Start 10ms Task */
        Task_10ms();
    }
}
