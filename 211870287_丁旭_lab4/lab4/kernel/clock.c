
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "global.h"

/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{                // 进程切换
        ticks++; // 每发生一次时钟中断就++

        if (ticks % TIME_SLICE == 0)
        {
                schedule();
        }
        if (k_reenter != 0)
        {
                return;
        }
}

/*======================================================================*
                              milli_delay
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{

        int t = get_ticks();

        while ((get_ticks() - t) < milli_sec) // milli_sec * 10ms
        {
        }
}

PUBLIC void wait()
{
        while (get_ticks() % TIME_SLICE < TIME_SLICE - 5)
        {
        }
        milli_delay(10);
}
