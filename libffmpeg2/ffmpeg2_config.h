#ifndef FFMPEG2_CONFIG_H
#define FFMPEG2_CONFIG_H

#define JZC_4765

#define __vc1_cache_text0__ __attribute__ ((__section__ (".vc1_cache_text0")))
#define __vc1_tcsm0_text0__ __attribute__ ((__section__ (".vc1_tcsm0_text0")))
#define __vc1_tcsm0_text1__ __attribute__ ((__section__ (".vc1_tcsm0_text1")))

#define JZC_DCORE_OPT

#define JZC_PRE_FILL_CACHES

/* used for residual values retrieving optimization  */
#define JZMPEG2_HW_IDCT
#define JZMPEG2_HW_IDCT_INTER



#endif//FFMPEG2_CONFIG

/* -------------  JZ Media HW MACRO define ------------------*/
//#define JZC_MC_OPT
//#define JZC_MXU_OPT
#define JZC_TCSM_OPT
//#define JZC_DBLK_OPT
//#define DBLK_DBG
//#define JZC_IDCT_OPT
//#define JZC_VLC_HW_OPT
//#define JZC_AUX_OPT
//#define JZC_CRC_VER
#define VMAU_OPT

/* -------------  P1 PMON define ------------------*/
//#define JZC_PMON_P0
//#define JZC_PMON_P1
//#define STA_CCLK
//#define STA_DCC
//#define STA_ICC
//#define STA_UINSN
//#define STA_INSN
//#define STA_TLB
