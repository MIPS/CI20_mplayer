#ifndef T_SDE_H_
#define T_SDE_H_

#define SDE_VBASE sde_base
#define SDE_PBASE 0x13290000

#define SDE_SLV_STAT_OFST	0x0	// mb level control: irq, dec_error, mb_done, mb_en
#define SDE_SLV_SL_CTRL_OFST	0x4	// slice level control: slice_end, slice_init
#define SDE_SLV_SL_GEOM_OFST	0x8
#define SDE_SLV_GL_CTRL_OFST	0xC	// global control: irq_en, module_reset, module_en
#define SDE_SLV_CODEC_ID_OFST	0x10	/* codec id(one hot): [0]:h264_decode, [1]:h264_encode,	*/
					/*                    [2]:vp8_decode,  [3]:vc1_decode, 	*/
#define SDE_DBG_0_OFST		0x100
#define SDE_DBG_1_OFST		0x104
#define SDE_DBG_2_OFST		0x108
#define SDE_DBG_3_OFST		0x10C
#define SDE_DBG_4_OFST		0x110
#define SDE_DBG_5_OFST		0x114
#define SDE_DBG_6_OFST		0x118
					/*                    [7:4]:reserved.	*/

#define SDE_SLV_CFG_0_OFST	0x14	// slice ctrl
#define SDE_SLV_CFG_1_OFST	0x18	// slice parameter
#define SDE_SLV_CFG_2_OFST	0x1C	// desp addr
#define SDE_SLV_CFG_3_OFST	0x20	// sync end addr
#define SDE_SLV_CFG_4_OFST	0x24	// bs addr
#define SDE_SLV_CFG_5_OFST	0x28	// 
#define SDE_SLV_CFG_6_OFST	0x2C	// 
#define SDE_SLV_CFG_7_OFST	0x30	// 
#define SDE_SLV_CFG_8_OFST	0x34	// 
#define SDE_SLV_CFG_9_OFST	0x38	// 
#define SDE_SLV_CFG_10_OFST	0x3C	// 
#define SDE_SLV_CFG_11_OFST	0x40	// 
#define SDE_SLV_CFG_12_OFST	0x44	// 
#define SDE_SLV_CFG_13_OFST	0x48	// 
#define SDE_SLV_CFG_14_OFST	0x4C	// 
#define SDE_SLV_CFG_15_OFST	0x50	// 

#define SDE_DBG_OFST		0x100
#define SDE_CTX_TBL_OFST	0x2000
#define SDE_CQP_TBL_OFST	0x3800

#define SDE_MB_DONE_SHIFT	0
#define SDE_SLICE_DONE_SHIFT	1
#define SDE_MB_IRQ_SHIFT	2
#define SDE_MB_ERROR_SHIFT	8

#define SDE_MB_EN_SHIFT 	0
#define SDE_SLIEC_INIT_SHIFT	1
#define SDE_SLIEC_END_SHIFT	2

#define SDE_MODULE_ENABLE_SHIFT	0
#define SDE_MODULE_RESET_SHIFT	1
#define SDE_IRQ_ENABLE_SHIFT	2

#define SET_SDE_STAT(val)	({write_reg((SDE_VBASE + SDE_SLV_STAT_OFST), (val));})
#define SET_SDE_CTRL(val)	({write_reg((SDE_VBASE + SDE_SLV_SL_CTRL_OFST), (val));})
#define SET_SDE_SL_GEOM(val)	({write_reg((SDE_VBASE + SDE_SLV_SL_GEOM_OFST), (val));})
#define SET_SDE_GL_CTRL(val)	({write_reg((SDE_VBASE + SDE_SLV_GL_CTRL_OFST), (val));})
#define SET_SDE_CODEC_ID(val)	({write_reg((SDE_VBASE + SDE_SLV_CODEC_ID_OFST), (val));})

#define GET_SDE_STAT()		({read_reg(SDE_VBASE, SDE_SLV_STAT_OFST);})
#define GET_SDE_CTRL()		({read_reg(SDE_VBASE, SDE_SLV_SL_CTRL_OFST);})
#define GET_SDE_SL_GEOM()	({read_reg(SDE_VBASE, SDE_SLV_SL_GEOM_OFST);})
#define GET_SDE_GL_CTRL()	({read_reg(SDE_VBASE, SDE_SLV_GL_CTRL_OFST);})

#define SET_SDE_CFG_0(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_0_OFST), (val));})
#define SET_SDE_CFG_1(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_1_OFST), (val));})
#define SET_SDE_CFG_2(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_2_OFST), (val));})
#define SET_SDE_CFG_3(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_3_OFST), (val));})
#define SET_SDE_CFG_4(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_4_OFST), (val));})
#define SET_SDE_CFG_5(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_5_OFST), (val));})
#define SET_SDE_CFG_6(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_6_OFST), (val));})
#define SET_SDE_CFG_7(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_7_OFST), (val));})
#define SET_SDE_CFG_8(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_8_OFST), (val));})
#define SET_SDE_CFG_9(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_9_OFST), (val));})
#define SET_SDE_CFG_10(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_10_OFST), (val));})
#define SET_SDE_CFG_11(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_11_OFST), (val));})
#define SET_SDE_CFG_12(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_12_OFST), (val));})
#define SET_SDE_CFG_13(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_13_OFST), (val));})
#define SET_SDE_CFG_14(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_14_OFST), (val));})
#define SET_SDE_CFG_15(val)	({write_reg((SDE_VBASE + SDE_SLV_CFG_15_OFST), (val));})

#define GET_SDE_CFG_0()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_0_OFST);})
#define GET_SDE_CFG_1()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_1_OFST);})
#define GET_SDE_CFG_2()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_2_OFST);})
#define GET_SDE_CFG_3()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_3_OFST);})
#define GET_SDE_CFG_4()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_4_OFST);})
#define GET_SDE_CFG_5()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_5_OFST);})
#define GET_SDE_CFG_6()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_6_OFST);})
#define GET_SDE_CFG_7()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_7_OFST);})
#define GET_SDE_CFG_8()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_8_OFST);})
#define GET_SDE_CFG_9()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_9_OFST);})
#define GET_SDE_CFG_10()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_10_OFST);})
#define GET_SDE_CFG_11()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_11_OFST);})
#define GET_SDE_CFG_12()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_12_OFST);})
#define GET_SDE_CFG_13()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_13_OFST);})
#define GET_SDE_CFG_14()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_14_OFST);})
#define GET_SDE_CFG_15()   	({read_reg(SDE_VBASE, SDE_SLV_CFG_15_OFST);})

#define GET_SDE_DBG_0()   	({read_reg(SDE_VBASE, SDE_DBG_0_OFST);})
#define GET_SDE_DBG_1()   	({read_reg(SDE_VBASE, SDE_DBG_1_OFST);})
#define GET_SDE_DBG_2()   	({read_reg(SDE_VBASE, SDE_DBG_2_OFST);})
#define GET_SDE_DBG_3()   	({read_reg(SDE_VBASE, SDE_DBG_3_OFST);})
#define GET_SDE_DBG_4()   	({read_reg(SDE_VBASE, SDE_DBG_4_OFST);})
#define GET_SDE_DBG_5()   	({read_reg(SDE_VBASE, SDE_DBG_5_OFST);})
#define GET_SDE_DBG_6()   	({read_reg(SDE_VBASE, SDE_DBG_6_OFST);})


#define JZM_SDE_WMODE_AUTO 0
#define JZM_SDE_WMODE_STEP 1
#define JZM_SDE_WMODE_DEBUG 2

/*codec ID*/
#define SDE_ID_H264_DEC         1<<0
#define SDE_ID_H264_ENC         1<<1
#define SDE_ID_VP8_DEC          1<<2
#define SDE_ID_VC1_DEC          1<<3
#define SDE_ID_MPEG2_DEC        1<<4

#endif //T_SDE_H
