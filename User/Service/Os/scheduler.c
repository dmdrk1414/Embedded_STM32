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
 *   - Task가 끝까지 돌아오면 워치독을 Refresh한다. (App은 워치독을 직접 부르지 않음)
 */
#include "scheduler.h"
#include "task.h"
#include "main.h"
#include "watchdog.h"    /* Task가 끝난 뒤 워치독 Refresh */
#include "GDS.h"         /* SCH_PERIOD_10MS_US */

/* CubeMX가 main.c에 만든 TIM2 핸들 */
extern TIM_HandleTypeDef htim2;

/* 마지막으로 Task_10ms()를 실행한 기준 카운터 값 (단위: µs, 16비트) */
static uint16_t s_last10ms = 0u;

/* Live Expressions 확인용: 0이 아니면 10ms 주기를 놓친 적이 있다는 뜻 */
volatile uint32_t g_schOverrun = 0u;

/*
 * Sch_Init() : 스케줄러 시작 (TIM2 카운터 시작, 인터럽트 없음)
 *
 * [설명] HAL_TIM_Base_Start()로 TIMx_CR1.CEN = 1 → 카운터만 돌린다.
 *        현재 카운터 값을 첫 기준 시각(s_last10ms)으로 저장한다.
 * [인자] 없음
 * [반환] 없음
 * [호출] main.c USER CODE 2에서 while (1) 들어가기 전에 한 번
 * [예시]
 *     EcuM_Init();
 *     Sch_Init();        // 이후 while (1)에서 Sch_Run() 호출
 */
void Sch_Init(void)
{
    (void)HAL_TIM_Base_Start(&htim2);
    s_last10ms = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);   /* 시작 기준 시각 */
}

/*
 * Sch_Run() : 10ms가 지났으면 Task_10ms()를 실행하고 워치독을 Refresh
 *
 * [설명] 반복문이 아니라 "한 번 확인하고 바로 돌아오는" 함수.
 *        대부분은 아무것도 안 하고 돌아오고, 10ms에 한 번만 Task를 실행한다.
 *        20ms 이상 늦게 확인했으면 g_schOverrun을 1 올린다.
 * [인자] 없음
 * [반환] 없음
 * [호출] main.c의 while (1) 안에서 쉬지 않고 계속
 * [예시]
 *     while (1)
 *     {
 *         Sch_Run();             // 10ms마다 Task_10ms()
 *         BswM_MainFunction();
 *     }
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

        /* Task가 끝까지 돌아왔을 때만 "이상 없음" 신호
         * (Task 안에서 멈추면 여기까지 못 와서 워치독 리셋) */
        Wdg_Refresh();
    }
}

/*
 * Sch_DelayMs() : 워치독을 살려 두면서 기다리기 (스케줄러 시작 전 전용)
 *
 * [설명] 워치독 타임아웃(최소 약 67ms)보다 긴 HAL_Delay를 쓰면 리셋된다.
 *        그래서 10ms(SCH_DELAY_STEP_MS)씩 나눠 기다리며 매번 Refresh한다.
 *        기다리는 동안 Task_10ms()는 돌지 않으므로, Sch_Init() 전에만 쓴다.
 *        시간 관리는 스케줄러의 일이라 여기 두었다. (예전 EcuM_Wait)
 * [인자] ms : 기다릴 시간 (ms, 10ms 단위로 내림. 예: 25 → 20ms)
 * [반환] 없음
 * [호출] indicator.c (시작 LED 패턴)
 * [예시]
 *     HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
 *     Sch_DelayMs(1000u);   // 1초 켜 두기 (그동안 워치독 100번 Refresh)
 */
void Sch_DelayMs(uint32_t ms)
{
    while (ms >= SCH_DELAY_STEP_MS)
    {
        HAL_Delay(SCH_DELAY_STEP_MS);
        Wdg_Refresh();
        ms -= SCH_DELAY_STEP_MS;
    }
}
