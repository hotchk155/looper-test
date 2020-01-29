typedef uint8_t byte;
void mydelay(volatile uint32_t nof);
void wait_ms();
byte is_ms_tick();

enum {
	SZ_DBLK = 512
};

typedef struct { byte data[SZ_DBLK]; } DBLK;


#if defined(__GNUC__) /* GNU Compiler */
#ifndef __ALIGN_END
#define __ALIGN_END __attribute__((aligned(4)))
#endif
#ifndef __ALIGN_BEGIN
#define __ALIGN_BEGIN
#endif
#else
#ifndef __ALIGN_END
#define __ALIGN_END
#endif
#ifndef __ALIGN_BEGIN
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) /* ARM Compiler */
#define __ALIGN_BEGIN __attribute__((aligned(4)))
#elif defined(__ICCARM__) /* IAR Compiler */
#define __ALIGN_BEGIN
#endif
#endif
#endif
