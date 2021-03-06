/**
 * @file psp_tmr.c
 */

/***********************
 *       INCLUDES
 ***********************/
#include "hw_conf.h"

#if USE_TMR != 0 && PSP_PIC32MZ != 0

#include <stddef.h>
#include <xc.h>
#include <sys/attribs.h>
#include "../../tmr.h"
#include "hw/per/tick.h"


/***********************
 *       DEFINES
 ***********************/
#define IC_PER_US   ((uint32_t)CLOCK_PERIPH / 1000000U) //Instructions in a usec

#define TIMER_DEF_PRIO   INT_PRIO_MID 

/* Ignore Timer1
#define TIMER1_IF IFS0bits.T1IF
#define TIMER1_IE IEC0bits.T1IE
#define TIMER1_IP IPC1bits.T1IP
*/
#define TIMER2_IF IFS0bits.T2IF
#define TIMER2_IE IEC0bits.T2IE
#define TIMER2_IP IPC2bits.T2IP

#define TIMER3_IF IFS0bits.T3IF
#define TIMER3_IE IEC0bits.T3IE
#define TIMER3_IP IPC3bits.T3IP

#define TIMER4_IF IFS0bits.T4IF
#define TIMER4_IE IEC0bits.T4IE
#define TIMER4_IP IPC4bits.T4IP

#define TIMER5_IF IFS0bits.T5IF
#define TIMER5_IE IEC0bits.T5IE
#define TIMER5_IP IPC5bits.T5IP

#define IPL_NAME(prio) IPL_CONC(prio)
#define IPL_CONC(prio) IPL ## prio ## AUTO
/***********************
 *       TYPEDEFS
 ***********************/

typedef struct
{
    volatile __T2CONbits_t * TxCON;
    volatile unsigned int * TMRx;
    volatile unsigned int * PRx;
    uint32_t period;
    void (*cb)(void);
}m_dsc_t;


/***********************
 *   STATIC VARIABLES
 ***********************/

static m_dsc_t m_dsc[] = 
{           /*TxCON*/                       /*TMRx*/       /*PRx*/      /*Period*/ /*Callback*/
#if 0   /*Always ignore timer 1 bcause its differnet from the otheres*/ 
        {(volatile T1CONBITS*)&T1CONbits,     &TMR1,         &PR2,          0,      NULL},
#else
        {NULL,                                NULL,          NULL,          0,      NULL},
#endif
#if TMR2_EN != 0
        {(volatile __T2CONbits_t*)&T2CONbits, &TMR2,         &PR2,          0,      NULL},
#else
        {NULL,                                NULL,          NULL,          0,      NULL},
#endif
#if TMR3_EN != 0
        {(volatile __T2CONbits_t*)&T3CONbits, &TMR3,         &PR3,          0,      NULL},
#else
        {NULL,                                NULL,          NULL,          0,      NULL},
#endif
#if TMR4_EN != 0
        {(volatile __T2CONbits_t*)&T4CONbits, &TMR4,         &PR4,          0,      NULL},
#else
        {NULL,                                 NULL,          NULL,          0,      NULL},
#endif
#if TMR5_EN != 0
        {(volatile __T2CONbits_t*)&T5CONbits,  &TMR5,         &PR5,          0,      NULL},
#else
        {NULL,                                 NULL,          NULL,          0,      NULL},
#endif 
#if TMR6_EN != 0
        {(volatile __T2CONbits_t*)&T6CONbits,  &TMR6,         &PR6,          0,      NULL},
#else
        {NULL,                                 NULL,          NULL,          0,      NULL},
#endif
};
static const uint16_t tmr_ps[] = {1, 2, 4, 8, 16, 32, 64, 256}; 

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/
        
/***********************
 *   STATIC PROTOTYPES
 ***********************/
static bool psp_tmr_id_test(tmr_t id);

/***********************
 *   GLOBAL FUNCTIONS
 ***********************/

/**
 * Initialize the timers
 */
void psp_tmr_init(void)
{
    /*No initialization is necessary*/
}

/**
 * Set the period of a timer in microseconds
 * @param tmr the id of a timer (HW_TMRx)
 * @param p_us period in microseconds
 * @return HW_RES_OK or any error from hw_res_t
 */
hw_res_t psp_tmr_set_period(tmr_t tmr, uint32_t p_us)
{
    if(psp_tmr_id_test(tmr) == false) return HW_RES_NOT_EX;
    
    hw_res_t res = HW_RES_OK;
    
    if(res == HW_RES_OK) {
        uint32_t pr = p_us * IC_PER_US;
        uint32_t new_pr = UINT32_MAX;
        uint8_t i;

        for(i = 0; 
            i < sizeof(tmr_ps) / sizeof(tmr_ps[0]) && new_pr > UINT16_MAX; 
            i++)
        {
            new_pr = pr / tmr_ps[i];
        }

        if(new_pr > UINT16_MAX)  res = HW_RES_INV_PARAM;
        else {
            /*Set the prescale*/
            *m_dsc[tmr].PRx = new_pr;
            if(i > 0)  i--;
            
            m_dsc[tmr].TxCON->TCKPS = i;    
        }
    }
    
    return res;
}

/**
 * Set the current value of a timer
 * @param tmr the id of a timer (HW_TMRx)
 * @param value the new value
 */
void psp_tmr_set_value(tmr_t tmr, uint32_t value)
{
    if(psp_tmr_id_test(tmr) == false) return;
    
    *m_dsc[tmr].TMRx = value;
}

/**
 * Set the callback function of timer (called in its interrupt)
 * @param tmr the id of a timer (HW_TMRx)
 * @param cb
 */
void psp_tmr_set_cb(tmr_t tmr, void (*cb) (void))
{
    if(psp_tmr_id_test(tmr) == false) return;
    
    m_dsc[tmr].cb = cb;
}

/**
 * Enable the interrupt of a timer
 * @param tmr the id of a timer (HW_TMRx)
 * @param en true: interrupt enable, false: disable
 */
void psp_tmr_en_int(tmr_t tmr, bool en)
{
    if(psp_tmr_id_test(tmr) == false) return;
 
    uint8_t en_value = (en == false ?  0 : 1);
            
    switch(tmr)
    {
#if 0
        case HW_TMR1:
            TIMER1_IE = en_value;
            TIMER1_IP = TMR1_PRIO;
            break;
#endif

#if TMR2_EN != 0 && TMR2_PRIO != HW_INT_PRIO_OFF
        case HW_TMR2:
            TIMER2_IE = en_value;
            TIMER2_IP = TMR2_PRIO;
            break;
#endif

#if TMR3_EN != 0 && TMR3_PRIO != HW_INT_PRIO_OFF
        case HW_TMR3:
            TIMER3_IE = en_value;
            TIMER3_IP = TMR3_PRIO;
            break;
#endif

#if TMR4_EN != 0 && TMR4_PRIO != HW_INT_PRIO_OFF
        case HW_TMR4:
            TIMER4_IE = en_value;
            TIMER4_IP = TMR4_PRIO;
            break;
#endif

#if TMR5_EN != 0 && TMR5_PRIO != HW_INT_PRIO_OFF
        case HW_TMR5:
            TIMER5_IE = en_value;
            TIMER5_IP = TMR5_PRIO;
            break;
#endif

#if TMR6_EN != 0 && TMR6_PRIO != HW_INT_PRIO_OFF
        case HW_TMR6:
            TIMER6_IE = en_value;
            TIMER6_IP = TMR6_PRIO;
            break;
#endif
        default:
            break;

    }
}

/**
 * Enable the running of a timer
 * @param tmr the id of a timer (HW_TMRx)
 * @param en true: timer run, false timer stop
 */
void psp_tmr_run(tmr_t tmr, bool en)
{
    if(psp_tmr_id_test(tmr) == false) return;
    m_dsc[tmr].TxCON->ON = (en == false ? 0 : 1);
}

/**
 * 
 */
#if 0
void __ISR(_TIMER_1_VECTOR, IPL_NAME(TMR1_PRIO)) isr_timer1 (void)
{
    TIMER1_IF = 0;
    
    if(m_dsc[HW_TMR1].cb != NULL)
    {
        m_dsc[HW_TMR1].cb();
    }
}
#endif

/**
 * 
 */
#if TMR2_EN != 0
void __ISR(_TIMER_2_VECTOR, IPL_NAME(TMR2_PRIO)) Timer2Handler(void)
{
    TIMER2_IF = 0;

    if(m_dsc[HW_TMR2].cb != NULL)
    {
        m_dsc[HW_TMR2].cb();
    }
}
#endif

/**
 * 
 */
#if TMR3_EN != 0
void __ISR(_TIMER_3_VECTOR, IPL_NAME(TMR3_PRIO)) isr_timer3 (void)
{
    TIMER3_IF = 0;
    
    if(m_dsc[HW_TMR3].cb != NULL)
    {
        m_dsc[HW_TMR3].cb();
    }
    
}
#endif

/**
 * 
 */
#if TMR4_EN != 0
void __ISR(_TIMER_4_VECTOR, IPL_NAME(TMR4_PRIO)) isr_timer4 (void)
{
    TIMER4_IF = 0;
    
    if(m_dsc[HW_TMR4].cb != NULL)
    {
        m_dsc[HW_TMR4].cb();
    }
}
#endif

/**
 * 
 */
#if TMR5_EN != 0
void __ISR(_TIMER_5_VECTOR, IPL_NAME(TMR5_PRIO)) isr_timer5 (void)
{
    TIMER5_IF = 0;
    
    if(m_dsc[HW_TMR5].cb != NULL)
    {
        m_dsc[HW_TMR5].cb();
    }
}
#endif

/**
 * 
 */
#if TMR6_EN != 0
void __ISR(_TIMER_6_VECTOR, IPL_NAME(TMR6_PRIO)) isr_timer6 (void)
{
    TIMER6_IF = 0;
    
    if(m_dsc[HW_TMR6].cb != NULL)
    {
        m_dsc[HW_TMR6].cb();
    }
}

#endif

/***********************
 *   STATIC FUNCTIONS
 ***********************/

/**
 * Test a timer id
 * @param id the id of a timer (HW_TMRx)
 * @return true: id is valid, false: id is invalid
 */
static bool psp_tmr_id_test(tmr_t id)
{
    if(id >= HW_TMR_NUM) return false;
    else if (m_dsc[id].TxCON == NULL) return false;
    
    return true;
}

#endif
