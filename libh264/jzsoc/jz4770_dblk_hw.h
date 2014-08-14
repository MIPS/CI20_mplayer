#ifndef __JZ4770_DBLK_HW_H__
#define __JZ4770_DBLK_HW_H__

// DATA_FMT
#ifdef P1_USE_PADDR
#define DBLK0_BASE  0x13270000
#define DBLK1_BASE  0x132D0000
#else
#define DBLK0_BASE dblk0_base
#define DBLK1_BASE dblk1_base
#endif

#define DBLK_CHN_NEXT_DHA_IDX 12//38
#define DBLK_CHN_SCH_ID_IDX 14 //40

#define VIDEO_FMT 1
#define DBLK_YSYN_VALUE          (0xa5a5)
#define DBLK_CSYN_VALUE          (0xb5b5)

unsigned int DBLK_BASE;

#define DBLK_OFST_00 0x00 //DBLK_CTRL
#define DBLK_OFST_04 0x04 //DATA_FMT
#define DBLK_OFST_08 0x08 //H264_QP_UY
#define DBLK_OFST_0C 0x0C //H264_QP_VU
#define DBLK_OFST_10 0x10 //NNZ
#define DBLK_OFST_14	0x14 //MV_ADDR
#define DBLK_OFST_18	0x18 //DEPTH_OFFSET
#define DBLK_OFST_1C	0x1C //y_in_addr;        
#define DBLK_OFST_20	0x20 //u_in_addr;        
#define DBLK_OFST_24	0x24 //v_in_addr;        
#define DBLK_OFST_28	0x28 //y_up_out_addr;       
#define DBLK_OFST_2C	0x2C //u_up_out_addr;       
#define DBLK_OFST_30	0x30 //v_up_out_addr;       
#define DBLK_OFST_34	0x34 //y_up_in_addr;           
#define DBLK_OFST_38	0x38 //u_up_in_addr;           
#define DBLK_OFST_3C	0x3C //v_up_in_addr;           
#define DBLK_OFST_40	0x40 //y_out_addr;          
#define DBLK_OFST_44	0x44 //u_out_addr;          
#define DBLK_OFST_48	0x48 //v_out_addr;          
#define DBLK_OFST_4C	0x4C
#define DBLK_OFST_50	0x50
#define DBLK_OFST_54	0x54
#define DBLK_OFST_58	0x58
#define DBLK_OFST_5C	0x5C
#define DBLK_OFST_60	0x60
#define DBLK_OFST_64	0x64 //y_up_in_stride;       
#define DBLK_OFST_68	0x68 //uv_up_in_stride;      
#define DBLK_OFST_6C	0x6C //y_up_out_stride;      
#define DBLK_OFST_70	0x70 //uv_up_out_stride;     
#define DBLK_OFST_74	0x74 //y_in_stride;          
#define DBLK_OFST_78	0x78 //uv_in_stride;         
#define DBLK_OFST_7C	0x7C //y_out_stride;         
#define DBLK_OFST_80	0x80 //uv_out_stride;        

#define DBLK_OFST_90	0x90 //BS_EDGE_7_0
#define DBLK_OFST_94	0x94 //BS_EDGE_15_8
#define DBLK_OFST_98	0x98 //BS_EDGE_23_16
#define DBLK_OFST_9C	0x9C //BS_EDGE_31_24

#define DBLK_OFST_A0	0xA0
#define DBLK_OFST_A4	0xA4
#define DBLK_OFST_A8	0xA8
#define DBLK_OFST_AC	0xAC
#define DBLK_OFST_B0	0xB0
#define DBLK_OFST_B4	0xB4
#define DBLK_OFST_B8	0xB8
#define DBLK_OFST_BC	0xBC
#define DBLK_OFST_C0	0xC0
#define DBLK_OFST_C4	0xC4
#define DBLK_OFST_C8	0xC8
#define DBLK_OFST_CC	0xCC
#define DBLK_OFST_D0	0xD0
#define DBLK_OFST_D4	0xD4
#define DBLK_OFST_D8	0xD8
#define DBLK_OFST_DC	0xDC
#define DBLK_OFST_E0	0xE0
#define DBLK_OFST_E4	0xE4
#define DBLK_OFST_E8	0xE8
#define DBLK_OFST_EC	0xEC
#define DBLK_OFST_F0	0xF0
#define DBLK_OFST_F4	0xF4
#define DBLK_OFST_F8	0xF8
#define DBLK_OFST_FC	0xFC
#define DBLK_OFST_100	0x100
#define DBLK_OFST_104	0x104
#define DBLK_OFST_108	0x108
#define DBLK_OFST_10C	0x10C
#define DBLK_OFST_110	0x110
#define DBLK_OFST_114	0x114
#define DBLK_OFST_118	0x118
#define DBLK_OFST_11C	0x11C
#define DBLK_OFST_120	0x120
#define DBLK_OFST_124	0x124
#define DBLK_OFST_128	0x128
#define DBLK_OFST_12C	0x12C

#define DBLK_OFST_130	0x130 //BS_EDGE_7_0
#define DBLK_OFST_134	0x134 //BS_EDGE_15_8
#define DBLK_OFST_138	0x138 //BS_EDGE_23_16
#define DBLK_OFST_13C	0x13C //BS_EDGE_31_24

#define DBLK_OFST_AB 0xA0 
#define DBLK_OFST_TC0 0xE8 

#define DBLK_OFST_DCS 0x84
#define DBLK_OFST_DHA 0x88
#define DBLK_OFST_CSA 0x8C


#define SET_DBLK_REG00( dblk_end, dblk_rst, dblk_en)		\
  ({write_reg((DBLK_BASE + DBLK_OFST_00),				\
   (((dblk_end & 0x1) 	<< 2)  |				\
    ((dblk_rst &0x1 )	<< 1)  |				\
	                   (dblk_en & 0x1)));})

#define SET_DBLK_REG04( mv_smooth, edge_mask, row_end,		\
			yuv_flag, dma_cfg_bs, mv_reduce,	\
			intra_left, intra_curr, intra_up,	\
			hw_bs_h264, video_fmt)			\
  ({write_reg((DBLK_BASE + DBLK_OFST_04),				\
	      ((mv_smooth << 24) |				\
	       (edge_mask << 16) |				\
	       (row_end << 15)   |				\
	       (yuv_flag << 8)   |				\
	       (dma_cfg_bs << 7) |				\
	       (mv_reduce << 6)  |				\
	       (intra_left << 5) |				\
	       (intra_curr << 4) |				\
	       (intra_up << 3)   |				\
	       (hw_bs_h264 << 2) |				\
	       video_fmt));})

#define SET_DBLK_REG08( u_qp_up, y_qp_left, y_qp_curr, y_qp_up)	\
  ({write_reg((DBLK_BASE + DBLK_OFST_08),				\
	      ((u_qp_up << 18) 	 |				\
	       (y_qp_left << 12) |				\
	       (y_qp_curr << 6)  |				\
	       y_qp_up));})

#define SET_DBLK_REG0C( v_qp_left, v_qp_curr, u_qp_up,	\
			u_qp_left, u_qp_curr)		\
  ({write_reg((DBLK_BASE + DBLK_OFST_0C),			\
	      ((v_qp_left << 24) |			\
	       (v_qp_curr << 18) |			\
	       (v_qp_up << 12)   |			\
	       (u_qp_left << 6)  |			\
	       u_qp_curr));})

#define SET_DBLK_REG10(nnz) ({write_reg((DBLK_BASE + DBLK_OFST_10),	\
                                        nnz); })

//mv_addr
#define SET_DBLK_REG14(addr) ({write_reg((DBLK_BASE + DBLK_OFST_14), \
                                        addr); })

#define SET_DBLK_REG18(offset_a, offset_b, b_slice)	\
  ({write_reg((DBLK_BASE + DBLK_OFST_18),			\
	      (((offset_a & 0xff) << 24) |		\
	       ((offset_b & 0xff) << 16) |		\
	       b_slice));})

#define SET_DBLK_REG1C(addr) ({write_reg((DBLK_BASE + DBLK_OFST_1C), \
                                        addr); })

#define SET_DBLK_REG90( edge7, edge6, edge5, edge4,	\
			edge3, edge2, edge1, edge0)	\
  ({write_reg((DBLK_BASE + DBLK_OFST_90),			\
	      ((edge7 << 28) +				\
	       (edge6 << 24) +				\
	       (edge5 << 20) +				\
	       (edge4 << 16) +				\
	       (edge3 << 12) +				\
	       (edge2 << 8) +				\
	       (edge1 << 4) +				\
	       edge0));})

#define SET_DBLK_REG94(edge15, edge14, edge13, edge12,	\
		       edge11, edge10, edge9, edge8)	\
  ({write_reg((DBLK_BASE + DBLK_OFST_94),			\
	      ((edge15 << 28) +				\
	       (edge14 << 24) +				\
	       (edge13 << 20) +				\
	       (edge12 << 16) +				\
	       (edge11 << 12) +				\
	       (edge10 << 8) +				\
	       (edge9 << 4) +				\
	       edge8));})

#define SET_DBLK_REG98(edge23, edge22, edge21, edge20,	\
		       edge19, edge18, edge17, edge16)	\
  ({write_reg((DBLK_BASE + DBLK_OFST_98),			\
	      ((edge23 << 28) +				\
	       (edge22 << 24) +				\
	       (edge21 << 20) +				\
	       (edge20 << 16) +				\
	       (edge19 << 12) +				\
	       (edge18 << 8) +				\
	       (edge17 << 4) +				\
	       edge16));})

#define SET_DBLK_REG9C(edge31, edge30, edge29, edge28,	\
		       edge27, edge26, edge25, edge24)	\
  ({write_reg((DBLK_BASE + DBLK_OFST_9C),			\
	      ((edge31 << 28) +				\
	       (edge30 << 24) +				\
	       (edge29 << 20) +				\
	       (edge28 << 16) +				\
	       (edge27 << 12) +				\
	       (edge26 << 8) +				\
	       (edge25 << 4) +				\
	       edge24));})


#define SET_AB_LUT(offset, ab) \
({write_reg((DBLK_BASE + DBLK_OFST_AB + offset), ab);})

#define SET_TC0_LUT(offset, tc0) \
({write_reg((DBLK_BASE + DBLK_OFST_TC0 + offset), tc0);})

#define RUN_DBLK() \
({ write_reg( (DBLK_BASE +  DBLK_OFST_00), (read_reg(DBLK_BASE, DBLK_OFST_00)) | 0x1); \
})
#define POLLING_END_FLAG() \
({ (read_reg(DBLK_BASE, DBLK_OFST_00)) & 0x4; \
})

#define READ_DCS() \
({ (read_reg(DBLK_BASE, DBLK_OFST_DCS)); \
})
#define READ_DHA() \
({ (read_reg(DBLK_BASE, DBLK_OFST_DHA)); \
})
#define READ_CSA() \
({ (read_reg(DBLK_BASE, DBLK_OFST_CSA)); \
})

#define READ_DBLK_REG00()	   \
({ (read_reg(DBLK_BASE, DBLK_OFST_00)); \
})
#define READ_DBLK_REG04() \
({ (read_reg(DBLK_BASE, DBLK_OFST_04)); \
})
#define READ_DBLK_REG08() \
({ (read_reg(DBLK_BASE, DBLK_OFST_08)); \
})
#define READ_DBLK_REG0C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_0C)); \
})
#define READ_DBLK_REG10() \
({ (read_reg(DBLK_BASE, DBLK_OFST_10)); \
})
#define READ_DBLK_REG14() \
({ (read_reg(DBLK_BASE, DBLK_OFST_14)); \
})
#define READ_DBLK_REG18() \
({ (read_reg(DBLK_BASE, DBLK_OFST_18)); \
})
#define READ_DBLK_REG1C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_1C)); \
})
#define READ_DBLK_REG20() \
({ (read_reg(DBLK_BASE, DBLK_OFST_20)); \
})
#define READ_DBLK_REG24() \
({ (read_reg(DBLK_BASE, DBLK_OFST_24)); \
})
#define READ_DBLK_REG28() \
({ (read_reg(DBLK_BASE, DBLK_OFST_28)); \
})
#define READ_DBLK_REG2C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_2C)); \
})
#define READ_DBLK_REG30() \
({ (read_reg(DBLK_BASE, DBLK_OFST_30)); \
})
#define READ_DBLK_REG34() \
({ (read_reg(DBLK_BASE, DBLK_OFST_34)); \
})
#define READ_DBLK_REG38() \
({ (read_reg(DBLK_BASE, DBLK_OFST_38)); \
})
#define READ_DBLK_REG3C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_3C)); \
})
#define READ_DBLK_REG40() \
({ (read_reg(DBLK_BASE, DBLK_OFST_40)); \
})
#define READ_DBLK_REG44() \
({ (read_reg(DBLK_BASE, DBLK_OFST_44)); \
})
#define READ_DBLK_REG48() \
({ (read_reg(DBLK_BASE, DBLK_OFST_48)); \
})
#define READ_DBLK_REG4C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_4C)); \
})
#define READ_DBLK_REG50() \
({ (read_reg(DBLK_BASE, DBLK_OFST_50)); \
})
#define READ_DBLK_REG54() \
({ (read_reg(DBLK_BASE, DBLK_OFST_54)); \
})
#define READ_DBLK_REG58() \
({ (read_reg(DBLK_BASE, DBLK_OFST_58)); \
})
#define READ_DBLK_REG5C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_5C)); \
})
#define READ_DBLK_REG60() \
({ (read_reg(DBLK_BASE, DBLK_OFST_60)); \
})
#define READ_DBLK_REG64() \
({ (read_reg(DBLK_BASE, DBLK_OFST_64)); \
})
#define READ_DBLK_REG68() \
({ (read_reg(DBLK_BASE, DBLK_OFST_68)); \
})
#define READ_DBLK_REG6C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_6C)); \
})
#define READ_DBLK_REG70() \
({ (read_reg(DBLK_BASE, DBLK_OFST_70)); \
})
#define READ_DBLK_REG74() \
({ (read_reg(DBLK_BASE, DBLK_OFST_74)); \
})
#define READ_DBLK_REG78() \
({ (read_reg(DBLK_BASE, DBLK_OFST_78)); \
})
#define READ_DBLK_REG7C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_7C)); \
})
#define READ_DBLK_REG80() \
({ (read_reg(DBLK_BASE, DBLK_OFST_80)); \
})
#define READ_DBLK_REG90() \
({ (read_reg(DBLK_BASE, DBLK_OFST_90)); \
})
#define READ_DBLK_REG94() \
({ (read_reg(DBLK_BASE, DBLK_OFST_94)); \
})
#define READ_DBLK_REG98() \
({ (read_reg(DBLK_BASE, DBLK_OFST_98)); \
})
#define READ_DBLK_REG9C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_9C)); \
})
#define READ_DBLK_REGA0() \
({ (read_reg(DBLK_BASE, DBLK_OFST_A0)); \
})
#define READ_DBLK_REGA4() \
({ (read_reg(DBLK_BASE, DBLK_OFST_A4)); \
})
#define READ_DBLK_REGA8() \
({ (read_reg(DBLK_BASE, DBLK_OFST_A8)); \
})
#define READ_DBLK_REGAC() \
({ (read_reg(DBLK_BASE, DBLK_OFST_AC)); \
})
#define READ_DBLK_REGB0() \
({ (read_reg(DBLK_BASE, DBLK_OFST_B0)); \
})
#define READ_DBLK_REGB4() \
({ (read_reg(DBLK_BASE, DBLK_OFST_B4)); \
})
#define READ_DBLK_REGB8() \
({ (read_reg(DBLK_BASE, DBLK_OFST_B8)); \
})
#define READ_DBLK_REGBC() \
({ (read_reg(DBLK_BASE, DBLK_OFST_BC)); \
})
#define READ_DBLK_REGC0() \
({ (read_reg(DBLK_BASE, DBLK_OFST_C0)); \
})
#define READ_DBLK_REGC4() \
({ (read_reg(DBLK_BASE, DBLK_OFST_C4)); \
})
#define READ_DBLK_REGC8() \
({ (read_reg(DBLK_BASE, DBLK_OFST_C8)); \
})
#define READ_DBLK_REGCC() \
({ (read_reg(DBLK_BASE, DBLK_OFST_CC)); \
})
#define READ_DBLK_REGD0() \
({ (read_reg(DBLK_BASE, DBLK_OFST_D0)); \
})
#define READ_DBLK_REGD4() \
({ (read_reg(DBLK_BASE, DBLK_OFST_D4)); \
})
#define READ_DBLK_REGD8() \
({ (read_reg(DBLK_BASE, DBLK_OFST_D8)); \
})
#define READ_DBLK_REGDC() \
({ (read_reg(DBLK_BASE, DBLK_OFST_DC)); \
})
#define READ_DBLK_REGE0() \
({ (read_reg(DBLK_BASE, DBLK_OFST_E0)); \
})
#define READ_DBLK_REGE4() \
({ (read_reg(DBLK_BASE, DBLK_OFST_E4)); \
})
#define READ_DBLK_REGE8() \
({ (read_reg(DBLK_BASE, DBLK_OFST_E8)); \
})
#define READ_DBLK_REGEC() \
({ (read_reg(DBLK_BASE, DBLK_OFST_EC)); \
})
#define READ_DBLK_REGF0() \
({ (read_reg(DBLK_BASE, DBLK_OFST_F0)); \
})
#define READ_DBLK_REGF4() \
({ (read_reg(DBLK_BASE, DBLK_OFST_F4)); \
})
#define READ_DBLK_REGF8() \
({ (read_reg(DBLK_BASE, DBLK_OFST_F8)); \
})
#define READ_DBLK_REGFC() \
({ (read_reg(DBLK_BASE, DBLK_OFST_FC)); \
})
#define READ_DBLK_REG100() \
({ (read_reg(DBLK_BASE, DBLK_OFST_100)); \
})
#define READ_DBLK_REG104() \
({ (read_reg(DBLK_BASE, DBLK_OFST_104)); \
})
#define READ_DBLK_REG108() \
({ (read_reg(DBLK_BASE, DBLK_OFST_108)); \
})
#define READ_DBLK_REG10C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_10C)); \
})
#define READ_DBLK_REG110() \
({ (read_reg(DBLK_BASE, DBLK_OFST_110)); \
})
#define READ_DBLK_REG114() \
({ (read_reg(DBLK_BASE, DBLK_OFST_114)); \
})
#define READ_DBLK_REG118() \
({ (read_reg(DBLK_BASE, DBLK_OFST_118)); \
})
#define READ_DBLK_REG11C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_11C)); \
})
#define READ_DBLK_REG120() \
({ (read_reg(DBLK_BASE, DBLK_OFST_120)); \
})
#define READ_DBLK_REG124() \
({ (read_reg(DBLK_BASE, DBLK_OFST_124)); \
})
#define READ_DBLK_REG128() \
({ (read_reg(DBLK_BASE, DBLK_OFST_128)); \
})
#define READ_DBLK_REG12C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_12C)); \
})
#define READ_DBLK_REG130() \
({ (read_reg(DBLK_BASE, DBLK_OFST_130)); \
})
#define READ_DBLK_REG134() \
({ (read_reg(DBLK_BASE, DBLK_OFST_134)); \
})
#define READ_DBLK_REG138() \
({ (read_reg(DBLK_BASE, DBLK_OFST_138)); \
})
#define READ_DBLK_REG13C() \
({ (read_reg(DBLK_BASE, DBLK_OFST_13C)); \
})

#define SET_DCS(val) \
({write_reg((DBLK_BASE + DBLK_OFST_DCS), val);})

#define SET_DHA(val) \
({write_reg((DBLK_BASE + DBLK_OFST_DHA), val);})

#define SET_CSA(val) \
({write_reg((DBLK_BASE + DBLK_OFST_CSA), val);})

#define DBLK_REG04( mv_smooth, edge_mask, row_end,		\
			yuv_flag, dma_cfg_bs, mv_reduce,	\
			intra_left, intra_curr, intra_up,	\
			hw_bs_h264, video_fmt)			\
	      ((mv_smooth << 24) |				\
	       (edge_mask << 16) |				\
	       (row_end << 15)   |				\
	       (yuv_flag << 8)   |				\
	       (dma_cfg_bs << 7) |				\
	       (mv_reduce << 6)  |				\
	       (intra_left << 5) |				\
	       (intra_curr << 4) |				\
	       (intra_up << 3)   |				\
	       (hw_bs_h264 << 2) |				\
	       video_fmt)

#define DBLK_REG08( mv_len, u_qp_up, y_qp_left, y_qp_curr, y_qp_up)	\
  ((mv_len << 24)    |							\
   (u_qp_up << 18)   |							\
   (y_qp_left << 12) |							\
   (y_qp_curr << 6)  |							\
   y_qp_up)

#define DBLK_REG0C( v_qp_left, v_qp_curr, v_qp_up,	\
		    u_qp_left, u_qp_curr)		\
  ((v_qp_left << 24) |					\
   (v_qp_curr << 18) |					\
   (v_qp_up << 12)   |					\
   (u_qp_left << 6)  |					\
   u_qp_curr)

#define DBLK_REG10( mv_mask, nnz)			\
  ((mv_mask << 24) |					\
   nnz)

#define DBLK_REG18(offset_a, offset_b, b_slice)		\
	      (((offset_a & 0xff) << 24) |		\
	       ((offset_b & 0xff) << 16) |		\
	       b_slice)

#define Y_NUM_STRD(mb_num, strd)		        \
              (((mb_num & 0xff) << 16) |	        \
	       strd)

#define H264_EDGE_BS( edge7, edge6, edge5, edge4,	\
		     edge3, edge2, edge1, edge0)	\
  (((edge7 & 0xf) << 28) |					\
   ((edge6 & 0xf) << 24) |					\
   ((edge5 & 0xf) << 20) |					\
   ((edge4 & 0xf) << 16) |					\
   ((edge3 & 0xf) << 12) |					\
   ((edge2 & 0xf) << 8) |					\
   ((edge1 & 0xf) << 4) |					\
   (edge0 & 0xf))


#define FIN_HEIGHT 16
#define FIN_WIDTH 20
#define FUP_HEIGHT 4
#define FUP_WIDTH 20
#define FOUT_HEIGHT 16
#define FOUT_WIDTH 20
#define YBS_ARY 32
#define FINUV_HEIGHT 8
#define FINUV_WIDTH 12
#define FUPUV_HEIGHT 4
#define FUPUV_WIDTH 12
#define FOUTUV_HEIGHT 8
#define FOUTUV_WIDTH 12
#define REF_MV_WNUM 53
#endif /*__JZ4770_DBLK_HW_H__*/
