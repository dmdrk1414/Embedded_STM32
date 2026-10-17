/*
 * rst.h : 리셋 원인 판별 모듈 (Reset)
 *
 * 역할
 *   - MCU가 "왜 처음부터 시작했는지" 판별한다.
 *   - AUTOSAR Mcu 드라이버의 Mcu_GetResetReason()과 같은 역할.
 *   - RH850에서는 RESF (Reset Factor, 리셋 요인) 레지스터를 읽는 부분에 해당.
 *
 * 근거 문서
 *   - RM0008 7.1   Reset (p.90~91)         : 리셋 종류 설명
 *   - RM0008 7.3.10 RCC_CSR (p.119~120)    : 리셋 원인 플래그 비트
 *   - RM0008 5.3.5 Standby mode (p.76)     : SBF (Standby 복귀 플래그)
 */
#ifndef RST_H
#define RST_H

/* 리셋 원인 종류 */
typedef enum
{
    RST_POWER_ON = 0,   /* 전원 ON          : RCC_CSR.PORRSTF (bit 27)              */
    RST_STANDBY_WAKE,   /* Standby 복귀      : PWR_CSR.SBF (bit 1), PA0 버튼으로 깨어남 */
    RST_PIN,            /* RESET 버튼        : RCC_CSR.PINRSTF (bit 26), NRST 핀     */
    RST_SOFTWARE,       /* 소프트웨어 리셋   : RCC_CSR.SFTRSTF (bit 28)              */
    RST_WATCHDOG,       /* 워치독 리셋       : RCC_CSR.IWDGRSTF(29), WWDGRSTF(30)    */
    RST_UNKNOWN         /* 위 어디에도 해당하지 않음                                  */
} Rst_ReasonType;

/*
 * 리셋 원인을 판별해서 돌려준다.
 * 주의: 내부에서 플래그를 지우므로 main 시작 시 "한 번만" 호출할 것.
 *       두 번째 호출부터는 플래그가 지워져 RST_UNKNOWN이 나온다.
 */
Rst_ReasonType Rst_GetReason(void);

#endif /* RST_H */
