// Copyright 2010-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _ROM_ETS_SYS_H_
#define _ROM_ETS_SYS_H_

#include <stdint.h>
#include <stdbool.h>
#include "soc/dport_reg.h"

#include "soc/soc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ETS_DEBUG
#define ETS_SERIAL_ENABLED()  (1)

typedef enum {
    ETS_OK     = 0,
    ETS_FAILED = 1
} ETS_STATUS;

typedef uint32_t ETSSignal;
typedef uint32_t ETSParam;

typedef struct ETSEventTag ETSEvent;

struct ETSEventTag {
    ETSSignal sig;
    ETSParam  par;
};

typedef void (*ETSTask)(ETSEvent *e);

enum ETS_User_Priorities {
    /* task priorities... */
    TICK_TASK_A_PRIO      = 2,
    KBD_TASK_PRIO         = 5,
    MAX_ETS_USER_PRIO     = 16,

    /* ISR priorities... */
    MAX_ETS_USER_ISR_PRIO = 0xFF - 16
};

/* ETS interrupt entry and exit */
/* actually we don't need the following 2 macros any more since we won't exit
 * isr until it is finised, more over, we don't do nest interrupt
 */

void ets_run(void);

void ets_task(ETSTask task, uint8_t prio, ETSEvent *queue, uint8_t qlen);

ETS_STATUS ets_post(uint8_t prio, ETSSignal sig, ETSParam  par);

/*
 * now things become complicated, print could be directed to uart and/or SDIO
 */
int ets_printf(const char *fmt, ...);

/* by default it's UART, just install_uart_printf, set putc1 to NULL to disable */
void ets_install_putc1(void (*p)(char c));
void ets_install_uart_printf(void);

/* no need to install, call directly */
int ets_uart_printf(const char *fmt, ...);

/* usually we don't need to call this, unless we want to disable SDIO print */
void ets_install_putc2(void (*p)(char c));
/* @prepare_buf: allocate buffer for printf internal writting 
 * @putc: just set to NULL, let printf write to to buffer, unless if you want more fancy stuff
 * @post_printf: will be called every time printf finish write buffer
 *
 * main idea of external printf is re-directing content to an external buffer.
 *  e.g. sip module allocates an event buffer in prepare_buf() and send the event to host in post_printf()
 *  moreover, you can check printf_buf_remain_len in post_printf(), send the event to host till buf is
 *  about to full.
 * 
 * TBD: Another approach is sending printf parameter to host and let host to decode, which could save some bytes.
 */
void ets_install_external_printf(void (*prepare_buf)(char ** bufptr, uint16_t *buflen, uint32_t *cookie),
                                  void (*putc)(char c),
                                  void (*post_printf)(uint32_t cookie));

uint16_t est_get_printf_buf_remain_len(void);
void est_reset_printf_buf_len(void);

/* external (SDIO) printf only, still need to install*/
int ets_external_printf(const char *fmt, ...);


#define ETS_PRINTF(...) ets_printf(...)

#define ETS_ASSERT(v) do { \
    if (!(v)) {             \
        ets_printf("%s %u \n", __FILE__, __LINE__); \
        while (1) {};   \
    }                   \
} while (0);

/* memory and string support */
int8_t ets_char2xdigit(char ch);
uint8_t * ets_str2macaddr(uint8_t *macaddr, char *str);
void ets_getc(char *c);
void ets_putc(char c);

/* timer related */
typedef uint32_t ETSHandle;
typedef void ETSTimerFunc(void *timer_arg);

typedef struct _ETSTIMER_ {
    struct _ETSTIMER_    *timer_next;
    uint32_t              timer_expire;
    uint32_t              timer_period;
    ETSTimerFunc         *timer_func;
    void                 *timer_arg;
} ETSTimer;

void ets_timer_init(void);
void ets_timer_arm(ETSTimer *timer, uint32_t tmout, bool repeat);
void ets_timer_disarm(ETSTimer *timer);
void ets_timer_done(ETSTimer *ptimer);
void ets_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg);

/* watchdog related */
typedef enum {
    WDT_NONE                  = -1,
    WDT_DISABLED              = 0,
    WDT_CONTROL_RESET         = 1,
    WDT_CONTROL_INTR          = 2, /* usually we use this mode? */
    WDT_CONTROL_EXTERNAL_FEED = 3,
    WDT_TESET_OVERFLOW        = 4, // intend to make watchdog overflow to test
} WDT_MODE;

typedef enum{
    WDT_INTERVAL_THREE_SEC      = 3,
    WDT_INTERVAL_SIX_SEC        = 6,
    WDT_INTERVAL_TWELVE_SEC     = 12,
} WDT_INTERVAL_TIME;

void ets_wdt_init(void);
WDT_MODE ets_wdt_get_mode(void);
void ets_wdt_enable(WDT_MODE mode, WDT_INTERVAL_TIME feed_interval, 
		WDT_INTERVAL_TIME expire_interval);
WDT_MODE ets_wdt_disable(void);
void ets_wdt_restore(WDT_MODE old_mode);

/* interrupt related */
typedef void (* ets_isr_t)(void *);

#define ETS_WMAC_SOURCE       0
#define ETS_SLC_SOURCE          (( PRO_SLC0_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) // 1
#define ETS_UART_SOURCE         (( PRO_UART_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //28
#define ETS_UART1_SOURCE        (( PRO_UART1_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //29
#define ETS_GPIO_SOURCE         (( PRO_GPIO_INTERRUPT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //19
#define ETS_FRC_TIMER2_SOURCE   (( PRO_TIMER_INT2_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //43
#define ETS_PCNT_SOURCE         (( PRO_PCNT_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //42  //Tony add
#define ETS_RMT_CTRL_SOURCE     (( PRO_RMT_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //41
#define ETS_GPIO_INUM           4            
#define ETS_PCNT_INUM           17
#define ETS_I2S0_SOURCE   (( PRO_I2S0_INT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //28
#define ETS_I2S0_INUM     17
#define ETS_RMT_CTRL_INUM       4//17

#define ETS_I2C0_INUM    17
#define ETS_I2C0_SOURCE  (( PRO_I2C_EXT0_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //43
#define ETS_I2C1_INUM    18
#define ETS_I2C1_SOURCE   (( PRO_I2C_EXT1_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //44

#define ETS_I2S1_SOURCE   (( PRO_I2S1_INT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //28  
#define ETS_I2S1_INUM     17

#define ETS_SPI1_SOURCE   (( PRO_SPI1_DMA_INT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //51  
#define ETS_SPI1_INUM     17

#define ETS_SPI2_SOURCE   (( PRO_SPI2_DMA_INT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //52  
#define ETS_SPI2_INUM     17

#define ETS_SPI3_SOURCE  (( PRO_SPI3_DMA_INT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4) //53  
#define ETS_SPI3_INUM     17


#define ETS_SPI_DMA_CHAN_SEL 	SPI_DMA_CHAN_SEL


//#define HW_GPIO_INUM            ETS_PCNT_SOURCE
#define ETS_MPWM_SOURCE      (( PRO_PWM2_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //    17//?
#define ETS_MPWM_INUM        17

#define ETS_EPWM_SOURCE      (( PRO_PWM0_INTR_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)  //    17//?
#define ETS_EPWM_INUM        17


#define ETS_LEDC_INUM       18
#define ETS_LEDC_SOURCE     (( PRO_LEDC_INT_MAP_REG - (PRO_MAC_INTR_MAP_REG) )/4)
//#define ETS_SLC_SOURCE        1
//#define ETS_UART_SOURCE       13
//#define ETS_UART1_SOURCE      14
//#define ETS_FRC_TIMER2_SOURCE 43
#define ETS_WMAC_INUM       0
#define ETS_SLC_INUM        1
#define ETS_SPI_INUM        2
#define ETS_HSIP_INUM       2
#define ETS_I2S_INUM        2
#define ETS_RTC_INUM        3
#define ETS_FRC_TIMER1_INUM 9  /* use edge*/
#define ETS_FRC_TIMER2_INUM 10 /* use edge*/
#define ETS_WDT_INUM        8  /* use edge*/
#define ETS_GPIO_INUM       4
#define ETS_UART_INUM       5
#define ETS_UART1_INUM      5//In the soc program the num is 18
#define ETS_MAX_INUM        6


void ets_isr_attach(int i, ets_isr_t func, void *arg);
void ets_isr_mask(uint32_t mask);
void ets_isr_unmask(uint32_t unmask);
void ets_intr_lock(void);
void ets_intr_unlock(void);
void intr_matrix_set(int cpu_no, uint32_t model_num, uint32_t intr_num);

#define _ETSTR(v) # v
#define _ETS_SET_INTLEVEL(intlevel)        ({ unsigned __tmp; \
            __asm__ __volatile__(   "rsil   %0, " _ETSTR(intlevel) "\n" \
                        : "=a" (__tmp) : : "memory" ); \
            })

void ets_delay_us(uint32_t us);
void ets_update_cpu_frequency(uint32_t ticks_per_us);
uint32_t ets_get_xtal_scale();
uint32_t ets_get_detected_xtal_freq();

#if 0
#define isdigit(c) ((c >= '0') && (c <= '9'))

#define isxdigit(c) (((c >= '0') && (c <= '9')) ||   \
                        ((c >= 'a') && (c <= 'f')) || \
                        ((c >= 'A') && (c <= 'F')) )

#define isblank(c) ((c == ' ') || (c == '\t'))

#define isupper(c) ((c >= 'A') && (c <= 'Z'))
#endif


#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#define ETS_MEM_BAR() asm volatile ( "" : : : "memory" )

#ifdef __cplusplus
}
#endif

#endif /* _ROM_ETS_SYS_H_ */
