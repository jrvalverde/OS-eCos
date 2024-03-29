/*=============================================================================
//
//      hal_diag.c
//
//      HAL diagnostic output code
//
//=============================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
// at http://sources.redhat.com/ecos/ecos-license/
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   larsi
// Contributors:larsi
// Date:        2006-07-26
// Purpose:     HAL diagnostic output
// Description: Implementations of HAL diagnostic output support.
//
//####DESCRIPTIONEND####
//
//===========================================================================*/

#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/hal_intr.h>

#include <cyg/hal/hal_io.h>

#include <cyg/hal/hal_misc.h>
/*---------------------------------------------------------------------------*/

//#define CYG_KERNEL_DIAG_LCD
#define CYG_KERNEL_DIAG_SERIAL0 // For ROM start but see immediately below:

#if defined(CYGSEM_HAL_USE_ROM_MONITOR_CygMon)
#undef CYG_KERNEL_DIAG_SERIAL0
#undef CYG_KERNEL_DIAG_LCD
#define CYG_KERNEL_DIAG_CYGMON
#define CYG_KERNEL_DIAG_GDB

#endif

/*---------------------------------------------------------------------------*/

#if defined(CYG_KERNEL_DIAG_SERIAL0) || defined(CYG_KERNEL_DIAG_CYGMON)

/*---------------------------------------------------------------------------*/
// LED diag function
/*
void hal_diag_init_led(){
// we only init the first 4 leds here
	asm volatile(
        "ldi:8	#0xf,	r4;\n"
        "ldi:20	#CYG_HAL_FR30_MB91360_DDRJ,	r5;\n"
        "stb	r4,	@r5;\n"
        "ldi:20	#CYG_HAL_FR30_MB91360_PFRJ, r5;\n"
        "stb	r4,	@r5;\n"
        : : :"r4", "r5"
    );
}
*/
// static cyg_uint8 leds = 0;

void hal_diag_init(void){

    HAL_WRITE_UINT8(CYG_HAL_FR30_MB91301_DDRJ, 0x2);
    HAL_WRITE_UINT8(CYG_HAL_FR30_MB91301_PFRJ, 0x3);

    // set up U-Timer
    HAL_WRITE_UINT8(CYG_HAL_FR30_MB91301_UTIMC0, 0x02);
    // 115200 bps
    HAL_WRITE_UINT16(CYG_HAL_FR30_MB91301_UTIMR0, 0x7);

    // setup UART
    HAL_WRITE_UINT8(CYG_HAL_FR30_MB91301_SCR0, 0x13);
    HAL_WRITE_UINT8(CYG_HAL_FR30_MB91301_SMR0, 0x30);
}


void hal_diag_led(cyg_uint8 leds)
{

    HAL_WRITE_UINT8(CYG_HAL_FR30_MB91301_PDRG, leds);

}

/*---------------------------------------------------------------------------*/

void hal_diag_write_char_serial0( char c)
{
    cyg_uint8 ssr;

    do
    {
        HAL_READ_UINT8( CYG_HAL_FR30_MB91301_SSR0 , ssr );
    } while (!(ssr & BIT3));

    HAL_WRITE_UINT8( CYG_HAL_FR30_MB91301_SODR0, c );

}

void hal_diag_write_hex_serial0(unsigned int c)
{
    unsigned char chr;
    int i;

    hal_diag_write_char_serial0('0');
    hal_diag_write_char_serial0('x');

    for(i = 28; i >= 0; i = i - 4)
    {
        chr = (c >> i) & 0xf;
        if (chr >= 10) chr = chr + 55;   /* for A-F */
        else  chr = chr + 48;  /* for 0-9 */
        hal_diag_write_char_serial0(chr);
    }
    hal_diag_write_char_serial0('\n');
    hal_diag_write_char_serial0('\r');
}


void hal_diag_drain(void)
{
    cyg_uint8 ssr;

	do
    {
        HAL_READ_UINT8( CYG_HAL_FR30_MB91301_SSR0 , ssr );
    } while (!(ssr & BIT3));
}

void hal_diag_read_char_serial0(char *c)
{
    cyg_uint8 ssr;

    do
    {
        HAL_READ_UINT8( CYG_HAL_FR30_MB91301_SSR0 , ssr );
    } while (!(ssr & BIT4));

    HAL_READ_UINT8( CYG_HAL_FR30_MB91301_SIDR0, *c );

}


#if defined(CYG_KERNEL_DIAG_CYGMON)
void hal_diag_dumb_write_char(char c)
#else
void hal_diag_write_char(char c)
#endif
{
#ifdef CYG_KERNEL_DIAG_GDB
    static char line[100];
    static int pos = 0;
//    register volatile cyg_uint16 *volatile tty_status = SERIAL1_SR;    

    // No need to send CRs
    if( c == '\r' ) return;

    line[pos++] = c;

    if( c == '\n' || pos == sizeof(line) )
    {

        // Disable interrupts. This prevents GDB trying to interrupt us
        // while we are in the middle of sending a packet. The serial
        // receive interrupt will be seen when we re-enable interrupts
        // later.
        CYG_INTERRUPT_STATE oldstate;
        HAL_DISABLE_INTERRUPTS(oldstate);
        
        while(1)
        {
            static char hex[] = "0123456789ABCDEF";
            cyg_uint8 csum = 0;
            int i;
            char c1;
        
            hal_diag_write_char_serial0('$');
            hal_diag_write_char_serial0('O');
            csum += 'O';
            for( i = 0; i < pos; i++ )
            {
                char ch = line[i];
                char h = hex[(ch>>4)&0xF];
                char l = hex[ch&0xF];
                hal_diag_write_char_serial0(h);
                hal_diag_write_char_serial0(l);
                csum += h;
                csum += l;
            }
            hal_diag_write_char_serial0('#');
            hal_diag_write_char_serial0(hex[(csum>>4)&0xF]);
            hal_diag_write_char_serial0(hex[csum&0xF]);

            hal_diag_read_char_serial0( &c1 );

            if( c1 == '+' ) break;

            {
                extern void cyg_hal_user_break(CYG_ADDRWORD *regs);
                extern cyg_bool cyg_hal_is_break(char *buf, int size);
                if( cyg_hal_is_break( &c1 , 1 ) )
                    cyg_hal_user_break( NULL );    
            }
            
            break;
        }
        
        pos = 0;

        // Wait for all data from serial line to drain
        // and clear ready-to-send indication.
        hal_diag_drain_serial0();
        
        // And re-enable interrupts
        HAL_RESTORE_INTERRUPTS( oldstate );
        
    }
#else
    hal_diag_write_char_serial0(c);
#endif    
}


void hal_diag_read_char(char *c)
{
    for(;;)
    {
#if defined(CYG_KERNEL_DIAG_GDB) && defined(CYGSEM_HAL_USE_ROM_MONITOR)

        typedef void rom_read_fn(char *c);
        rom_read_fn *fn = ((rom_read_fn **)0x80000100)[62];

        fn(c);
    
#else    
        hal_diag_read_char_serial0(c);

#endif    

#if defined(CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS)
        if( *c == 3 )
        {
            // Ctrl-C: breakpoint.
            extern void breakpoint(void);
            breakpoint();
            continue;
        }
#elif defined(CYGSEM_HAL_USE_ROM_MONITOR)
        if( *c == 3 )
        {
            // Ctrl-C: breakpoint.

//                HAL_BREAKPOINT(_breakinst);
            typedef void bpt_fn(void);
            bpt_fn *bfn = ((bpt_fn **)0x80000100)[61];

            bfn();
            continue;            
        }
#endif

        break;      
    }
}

#endif // defined(CYG_KERNEL_DIAG_SERIAL0) || defined(CYG_KERNEL_DIAG_CYGMON)


#if defined(CYG_KERNEL_DIAG_CYGMON) // only

/* This code has been imported from the BSP module. The definitions have
 * been left as-is, even though there was scope for doing more, to avoid
 * too much drift from the original sources
 */

struct bsp_comm_procs {
    void *ch_data;
    void (*__write)(void *ch_data, const char *buf, int len);
    int  (*__read)(void *ch_data, char *buf, int len);
    void (*__putc)(void *ch_data, char ch);
    int  (*__getc)(void *ch_data);
    int  (*__control)(void *ch_data, int func, ...);
};

// This is pointed to by entry BSP_NOTVEC_BSP_COMM_PROCS:
typedef struct {
    int  version;       /* version number for future expansion */
    void *__ictrl_table;
    void *__exc_table;
    void *__dbg_vector;
    void *__kill_vector;
    struct bsp_comm_procs *__console_procs;
    struct bsp_comm_procs *__debug_procs;
    void *__flush_dcache;
    void *__flush_icache;
    void *__cpu_data;
    void *__board_data;
    void *__sysinfo;
    int  (*__set_debug_comm)(int __comm_id);
    int  (*__set_console_comm)(int __comm_id);
    int  (*__set_serial_baud)(int __comm_id, int baud);
    void *__dbg_data;
    void (*__reset)(void);
    int  __console_interrupt_flag;
} bsp_shared_t;

/*
 * Core Exception vectors.
 */
#define BSP_EXC_INT	    0
#define BSP_EXC_TLBMOD	    1
#define BSP_EXC_TLBL	    2
#define BSP_EXC_TLBS	    3
#define BSP_EXC_ADEL	    4
#define BSP_EXC_ADES	    5
#define BSP_EXC_IBE         6
#define BSP_EXC_DBE         7
#define BSP_EXC_SYSCALL     8
#define BSP_EXC_BREAK       9
#define BSP_EXC_ILL        10
#define BSP_EXC_CPU        11
#define BSP_EXC_OV         12
#define BSP_EXC_TRAP       13
#define BSP_EXC_VCEI       14
#define BSP_EXC_FPE        15
#define BSP_EXC_RSV16      16
#define BSP_EXC_RSV17      17
#define BSP_EXC_RSV18      18
#define BSP_EXC_RSV19      19
#define BSP_EXC_RSV20      20
#define BSP_EXC_RSV21      21
#define BSP_EXC_RSV22      22
#define BSP_EXC_WATCH      23
#define BSP_EXC_RSV24      24
#define BSP_EXC_RSV25      25
#define BSP_EXC_RSV26      26
#define BSP_EXC_RSV27      27
#define BSP_EXC_RSV28      28
#define BSP_EXC_RSV29      29
#define BSP_EXC_RSV30      30
#define BSP_EXC_VCED       31
/* tx39 debug exception */
#define BSP_EXC_DEBUG      32
#define BSP_EXC_TLB        33
#define BSP_EXC_NMI        34
/*
 * Hack for eCos on tx39 to set an async breakpoint.
 */
#define BSP_VEC_BP_HOOK    35

#define BSP_EXC_XTLB	   36
#define BSP_EXC_CACHE	   37

#define BSP_MAX_EXCEPTIONS 38

/*
 * Another hack for tx39 eCos compatibility.
 */
#if defined(__CPU_R3900__)
#define BSP_VEC_MT_DEBUG   15
#else
#define BSP_VEC_MT_DEBUG   38
#endif

#define BSP_VEC_STUB_ENTRY 39
#define BSP_VEC_BSPDATA    40
#define BSP_VEC_MAGIC      41
#define BSP_VEC_IRQ_CHECK  42

#define BSP_VEC_PAD        43
#define NUM_VTAB_ENTRIES   44


#define BSP_MAGIC_VAL      0x55aa4321

#define SYS_interrupt 1000

// These vectors should be called with:
//
//  k0 - Exception Number

#define CYGMON_VECTOR_TABLE_BASE 0x80000100
#define CYGMON_VECTOR_TABLE ((CYG_ADDRESS *)CYGMON_VECTOR_TABLE_BASE)

#if 0 // UNUSED
static int
hal_bsp_set_debug_comm(int arg)
{
    bsp_shared_t *shared;

    shared = (bsp_shared_t *)
        (CYGMON_VECTOR_TABLE[ BSP_VEC_BSPDATA ]);

    if (0 != shared->__set_debug_comm) {
        return (*(shared->__set_debug_comm))(arg);
    }
    return 0;
}

static int
hal_bsp_set_console_comm(int arg)
{
    bsp_shared_t *shared;

    shared = (bsp_shared_t *)
        (CYGMON_VECTOR_TABLE[ BSP_VEC_BSPDATA ]);

    if (0 != shared->__set_console_comm) {
        return (*(shared->__set_console_comm))(arg);
    }
    return 0;
}
#endif // 0 UNUSED

static void bsp_trap(int trap_num);

static int
hal_bsp_console_write(const void *p, int len)
{
    bsp_shared_t *shared;
    struct bsp_comm_procs *com;
    int  magic;

    /*hal_bsp_set_console_comm(0);*/

    /* If this is not a BSP-based CygMon, return 0 */
    magic = (int)(CYGMON_VECTOR_TABLE[ BSP_VEC_MAGIC ]);
    if (magic != BSP_MAGIC_VAL)
	return 0;

    shared = (bsp_shared_t *)
        (CYGMON_VECTOR_TABLE[ BSP_VEC_BSPDATA ]);

    com = shared->__console_procs;

    if (0 != com) {
	shared->__console_interrupt_flag = 0;
        com->__write(com->ch_data, p, len);
	if (shared->__console_interrupt_flag) {
	    /* debug interrupt; stop here */
	    bsp_trap(SYS_interrupt);
	}

        return 1;
    }
    return 0;
}

static void
bsp_trap(int trap_num)
{
    asm("syscall\n");
}


static void
hal_dumb_serial_write(const char *p, int len)
{
    int i;
    for ( i = 0 ; i < len; i++ ) {
	hal_diag_dumb_write_char(p[i]);
    }
} 
/*
void hal_diag_write_char(char c)
{
    static char line[100];
    static int pos = 0;

    // No need to send CRs
    if( c == '\r' ) return;

    line[pos++] = c;

    if( c == '\n' || pos == sizeof(line) ) {
        CYG_INTERRUPT_STATE old;

        // Disable interrupts. This prevents GDB trying to interrupt us
        // while we are in the middle of sending a packet. The serial
        // receive interrupt will be seen when we re-enable interrupts
        // later.
        
        HAL_DISABLE_INTERRUPTS(old);
        
        if ( ! hal_bsp_console_write( line, pos ) )
            // then there is no function registered, just spew it out serial
            hal_dumb_serial_write( line, pos );
        
        pos = 0;

        // And re-enable interrupts
        HAL_RESTORE_INTERRUPTS(old);

    }
}*/

int
hal_diag_irq_check(int vector)
{
    typedef int irq_check_fn(int irq_nr);
    irq_check_fn *fn = (irq_check_fn *)(CYGMON_VECTOR_TABLE[ BSP_VEC_IRQ_CHECK ]);
    int  magic;
    

    /* If this is not a BSP-based CygMon, return 0 */
    magic = (int)(CYGMON_VECTOR_TABLE[ BSP_VEC_MAGIC ]);
    if (magic != BSP_MAGIC_VAL)
	return 0;

#if defined(CYGPKG_HAL_MIPS_TX3904)
    /* convert vector to BSP irq number */
    if (vector == 16)
	vector = 2;
    else
	vector += 3;
#endif

    return fn(vector);
}

#endif // defined(CYG_KERNEL_DIAG_CYGMON) *only*


/*---------------------------------------------------------------------------*/
/* End of hal_diag.c */
