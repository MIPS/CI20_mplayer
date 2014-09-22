/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "mem_def_falcon.h"
#include "jz4770_dblk_hw.h"
#undef printf

void init_dblk_chain_fifo(int mb_width, int mbx){
const int alpha_table_dblk[52] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
    255, 255
};

const int beta_table_dblk[52] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18
};
const int tc0_table_dblk[52][3] = {
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};
 
 int i; 
 { // dblk1 chain fixed(unchanged) part
   DBLK_BASE = DBLK0_BASE; //dblk1
   // alpha beta lut
   int tmp = (beta_table_dblk[17] << 24) + (alpha_table_dblk[17] << 16);
   SET_AB_LUT(0, tmp);
   for (i = 1; i < 18; i++) { 
     tmp = (beta_table_dblk[17 + i*2] << 24) + (alpha_table_dblk[17 + i*2] << 16) +
       (beta_table_dblk[16 + i*2] << 8) + alpha_table_dblk[16 + i*2];       
     SET_AB_LUT((i*4), tmp);
   }
   for (i = 0; i < 18; i++) {
     tmp = (tc0_table_dblk[17+i*2][2] << 25) + (tc0_table_dblk[17+i*2][1] << 20) +
       (tc0_table_dblk[17+i*2][0] << 16) +
       (tc0_table_dblk[16+i*2][2] << 9) + (tc0_table_dblk[16+i*2][1] << 4) +
       tc0_table_dblk[16+i*2][0];
     SET_TC0_LUT((i*4), tmp);
   }
 }//dblk1 chain fixed part over!
 /*------------------------------------------------------------------------*/
 //
 /*------------------------------------------------------------------------*/
      
 { // dblk2 chain fixed(unchanged) part
   DBLK_BASE = DBLK1_BASE; //dblk2
   // alpha beta lut
   int tmp = (beta_table_dblk[17] << 24) + (alpha_table_dblk[17] << 16);
   SET_AB_LUT(0, tmp);
   for (i = 1; i < 18; i++) { 
     tmp = (beta_table_dblk[17 + i*2] << 24) + (alpha_table_dblk[17 + i*2] << 16) +
       (beta_table_dblk[16 + i*2] << 8) + alpha_table_dblk[16 + i*2];       
      SET_AB_LUT((i*4), tmp);
   }
   
   for (i = 0; i < 18; i++) {
     tmp = (tc0_table_dblk[17+i*2][2] << 25) + (tc0_table_dblk[17+i*2][1] << 20) +
       (tc0_table_dblk[17+i*2][0] << 16) +
       (tc0_table_dblk[16+i*2][2] << 9) + (tc0_table_dblk[16+i*2][1] << 4) +
       tc0_table_dblk[16+i*2][0];
     SET_TC0_LUT((i*4), tmp);
   }
 }//dblk2 chain fixed part over!

 unsigned int *y_chain = (unsigned int *)DBLK_CHAIN_INIT_BASE;  
 //data node
 {
   //reg00 : dblk_ctrl;  
   write_reg((&y_chain[34]), ((0<<31) | /*only maintain the addr, no transfer and filter*/
			      (0<<30) | /*transfer for ever*/
			      (0<<29) | /*maintain mb_out addr*/
			      (0<<28) | /*mb_out exchange*/
			      (0<<27) | /*mb_out wrap*/
			      (0<<26) | /*maintain mb_in addr*/
			      (0<<25) | /*mb_in exchange*/
			      (0<<24) | /*mb_in wrap*/
			      (0<<23) | /*maintain mb_up_out addr*/
			      (0<<22) | /*mb_up_out incremental*/
			      (0<<21) | /*maintain mb_up_in addr*/
			      (0<<20) | /*mb_up_in incremental*/
			      (1<< 1) | //dblk_rst
			      (1))); /*dblk enable*/
   
   write_reg((&y_chain[35]), 0x80000000);

   write_reg((&y_chain[36]), ((0x4<<24) | /*CSA*/
			      (0x2<<16) | /*DCT*/
			      (1<<6) |  /*need handsake*/
			      (0<<4) | /*0:data node, 1:ofst, 2:addr */
			      (0<<3) | //ND_LINK
			      (0<<2) | /*ND_CKG*/
			      (0))); /*return to dha*/ 
   
   write_reg((&y_chain[37]), DBLK_REG04( 0, 0xff, 0, 0x0, 0, 1,0,1,0,1, 1));
   //reg00 : dblk_ctrl;
   write_reg((&y_chain[38]), ((0<<31) | /*only maintain the addr, no transfer and filter*/
			      (0<<30) | /*transfer for ever*/
			      (1<<29) | /*maintain mb_out addr*/
			      (0<<28) | /*mb_out exchange*/
			      (1<<27) | /*mb_out wrap*/
			      (0<<26) | /*maintain mb_in addr*/
			      (0<<25) | /*mb_in exchange*/
			      (0<<24) | /*mb_in wrap*/
			      (0<<23) | /*maintain mb_up_out addr*/
			      (0<<22) | /*mb_up_out incremental*/
			      (0<<21) | /*maintain mb_up_in addr*/
			      (0<<20) | /*mb_up_in incremental*/
			      (1))); /*dblk enable*/

   //printf(" +++- - - - - - - - -dblk init chain , should run only once !!!! - - - -- - - - \n ");
   write_reg((DBLK0_BASE + DBLK_OFST_DHA), do_get_phy_addr(DBLK_CHAIN_INIT_BASE));
   write_reg((DBLK0_BASE + DBLK_OFST_DCS), 0x1); //start dblk 1
   
   write_reg((DBLK1_BASE + DBLK_OFST_DHA), do_get_phy_addr(DBLK_CHAIN_INIT_BASE));
   write_reg((DBLK1_BASE + DBLK_OFST_DCS), 0x1); //start dblk 2

   do{
   }while(!(read_reg(DBLK0_BASE, DBLK_OFST_00) & 0x4));  
    
   do{
   }while(!(read_reg(DBLK1_BASE, DBLK_OFST_00) & 0x4));
 }

 int mm;
 write_reg(DBLK0_BASE + DBLK_OFST_04, DBLK_REG04(0, 0xff, 0, 0x0, 0, 1,0,1,0,1, 1));
 write_reg(DBLK1_BASE + DBLK_OFST_04, DBLK_REG04(0, 0xff, 0, 0x0, 0, 1,0,1,0,1, 1));
 for(mm=0; mm<mbx; mm++){
   write_reg(DBLK0_BASE + DBLK_OFST_00, ((1<<31) | /*only maintain the addr, no transfer and filter*/
					 (0<<30) | /*transfer for ever*/
					 (0<<29) | /*maintain mb_out addr*/
					 (0<<28) | /*mb_out exchange*/
					 (0<<27) | /*mb_out wrap*/
					 (0<<26) | /*maintain mb_in addr*/
					 (0<<25) | /*mb_in exchange*/
					 (0<<24) | /*mb_in wrap*/
					 (0<<23) | /*maintain mb_up_out addr*/
					 (0<<22) | /*mb_up_out incremental*/
					 (1<<21) | /*maintain mb_up_in addr*/
					 (0<<20) | /*mb_up_in incremental*/
					 (1))); /*dblk enable*/

   write_reg(DBLK1_BASE + DBLK_OFST_00, ((1<<31) | /*only maintain the addr, no transfer and filter*/
					 (0<<30) | /*transfer for ever*/
					 (0<<29) | /*maintain mb_out addr*/
					 (0<<28) | /*mb_out exchange*/
					 (0<<27) | /*mb_out wrap*/
					 (0<<26) | /*maintain mb_in addr*/
					 (0<<25) | /*mb_in exchange*/
					 (0<<24) | /*mb_in wrap*/
					 (0<<23) | /*maintain mb_up_out addr*/
					 (0<<22) | /*mb_up_out incremental*/
					 (1<<21) | /*maintain mb_up_in addr*/
					 (0<<20) | /*mb_up_in incremental*/
					 (1))); /*dblk enable*/
   do{
   }while(!(read_reg(DBLK0_BASE, DBLK_OFST_00) & 0x4));     
   do{
   }while(!(read_reg(DBLK1_BASE, DBLK_OFST_00) & 0x4));     
 }

 DBLK_BASE = DBLK0_BASE; //dblk1  
 SET_DHA(do_get_phy_addr(DBLK_CHAIN0_P_BASE));
 DBLK_BASE = DBLK1_BASE; //dblk2
 SET_DHA(do_get_phy_addr(DBLK_CHAIN1_P_BASE));    
}

#if 0
void init_dblk_chain_frame(int mb_width){
  unsigned int *y_chain = (unsigned int *)DBLK_CHAIN_INIT_BASE;  
  //data node
  {
    write_reg((&y_chain[0]), 0x80000000);
    write_reg((&y_chain[1]), ((0x80<<24) | /*CSA*/
			      (0x21<<16) | /*DCT*/
			      (1<<6) |  /*need handsake*/
			      (0<<4) | /*0:data node, 1:ofst, 2:addr */
			      (1<<3) | //ND_LINK
			      (0<<2) | /*ND_CKG*/
			      (0))); /*return to dha*/ 
    
    //reg80 : uv_out_stride;
    write_reg((&y_chain[2]), MAU_UVDST_STR);
    //reg7C : y_out_stride;
    write_reg((&y_chain[3]), Y_NUM_STRD(DBK_SFIFO_DEP, MAU_YDST_STR));
    //reg78 : uv_in_stride;
    write_reg((&y_chain[4]), MAU_UVDST_STR);
    //reg74 : y_in_stride;
    write_reg((&y_chain[5]), Y_NUM_STRD(DBK_SFIFO_DEP, MAU_YDST_STR));
    //reg70 : uv_up_out_stride;
    write_reg((&y_chain[6]), 8*MAU_SFIFO_DEP);
    //reg6C : y_up_out_stride;
    write_reg((&y_chain[7]), Y_NUM_STRD(DBK_SFIFO_DEP, 16*MAU_SFIFO_DEP)); 
    //reg68 : uv_up_in_stride;
    write_reg((&y_chain[8]), (MB_W_1080P*8+4));
    //reg64 : y_up_in_stride;
    write_reg((&y_chain[9]), Y_NUM_STRD(mb_width, (MB_W_1080P*16+4)));
    //reg60 : v_xchg_out_addr;
    write_reg((&y_chain[10]), 0x0);
    //reg5C : u_xchg_out_addr;
    write_reg((&y_chain[11]), 0x0);
    //reg58 : y_xchg_out_addr;
    write_reg((&y_chain[12]), 0x0);
    //reg54 : v_xchg_in_addr;
    write_reg((&y_chain[13]), 0x0);
    //reg50 : u_xchg_in_addr;
    write_reg((&y_chain[14]), 0x0);
    //reg4C : y_xchg_in_addr;
    write_reg((&y_chain[15]), 0x0);
    
    //reg48 : v_out_addr;
    write_reg((&y_chain[16]),(do_get_phy_addr(DBLK_VDST_BASE)));
    //reg44 : u_out_addr;
    write_reg((&y_chain[17]),(do_get_phy_addr(DBLK_UDST_BASE)));
    //reg40 : y_out_addr;
    write_reg((&y_chain[18]),(do_get_phy_addr(DBLK_YDST_BASE)));
    //reg3C : v_up_in_addr;
    write_reg((&y_chain[19]),do_get_phy_addr(DBLK_V_UPL_BASE+4));
    //reg38 : u_up_in_addr;
    write_reg((&y_chain[20]),do_get_phy_addr(DBLK_U_UPL_BASE+4));
    //reg34 : y_up_in_addr;
    write_reg((&y_chain[21]),do_get_phy_addr(DBLK_Y_UPL_BASE+4));
    //reg30 : v_up_out_addr;
    write_reg((&y_chain[22]),(do_get_phy_addr(DBLK_V_UPOUT_BASE)));
    //reg2C : u_up_out_addr;
    write_reg((&y_chain[23]),(do_get_phy_addr(DBLK_U_UPOUT_BASE)));
    //reg28 : y_up_out_addr;
    write_reg((&y_chain[24]), (do_get_phy_addr(DBLK_Y_UPOUT_BASE)));
    //reg24 : v_in_addr;
    write_reg((&y_chain[25]),(do_get_phy_addr(MAU_VDST_BASE)));
    //reg20 : u_in_addr;
    write_reg((&y_chain[26]),(do_get_phy_addr(MAU_UDST_BASE)));
    //reg1C : y_in_addr;
    write_reg((&y_chain[27]), (do_get_phy_addr(MAU_YDST_BASE)));
    write_reg((&y_chain[28]),DBLK_REG18(0,0,0));//offset
    
    //reg14 : h264_mv_addr;
    write_reg((&y_chain[29]), (do_get_phy_addr(DBLK_MV_ADDR_V_BASE)));
    
    
    write_reg((&y_chain[30]), 0);
    write_reg((&y_chain[31]), 0);
    write_reg((&y_chain[32]), 0);
    write_reg((&y_chain[33]), DBLK_REG04(0, 0xff, 0, 0x0, 0, 1,0,1,0,1, 1));
    
    int i;
    unsigned int dblk_mv_addr = do_get_phy_addr(DBLK_MV_ADDR_V_BASE);//+ ctrl_fifo_idx*56*4;
    y_chain = (unsigned int *)(DBLK_CHAIN0_P_BASE);  
    
    for(i = 0 ; i < DBK_SFIFO_DEP; i++){
      write_reg((&y_chain[0]), 0x80000000);
      write_reg((&y_chain[1]), ((0x18<<24) | /*CSA*/
				(0x7<<16) | /*DCT*/ //13
				(1<<6) | /*cfg nddn*//*need handsake*/
				(0<<4) | /*0:data node, 1:ofst, 2:addr */
				(1<<3) | //ND_LINK
				(0<<2) | /*ND_CKG*/
				(0))); /*return to dha*/ 
      
    
      //write_reg((&y_chain[2],DBLK_REG18( 0,0, 0)));//reg18 : h264_depth_offset;
      
      //reg14 : h264_mv_addr;
      write_reg((&y_chain[3]), (dblk_mv_addr + i*56*4));
      
      
      //write_reg((&y_chain[4]), 0);//reg10 : h264_mv_mask_nnz;
      //write_reg((&y_chain[5]), 0);//reg0C : h264_qp_vu;
      //write_reg((&y_chain[6]), 0);//DBLK_REG08
      //write_reg((&y_chain[7]), DBLK_REG04( 0, 0xff, 0, 0x0, 0, 1,0,1,0,1, 1))); //fmt
      
      //reg00 : dblk_ctrl;
      write_reg((&y_chain[8]), ((0<<31) | /*only maintain the addr, no transfer and filter*/
				(0<<30) | /*transfer for ever*/
				(1<<29) | /*maintain mb_out addr*/
				(0<<28) | /*mb_out exchange*/
				(1<<27) | /*mb_out wrap*/
				(1<<26) | /*maintain mb_in addr*/
				(0<<25) | /*mb_in exchange*/
				(1<<24) | /*mb_in wrap*/
				(1<<23) | /*maintain mb_up_out addr*/
				(0<<22) | /*mb_up_out incremental*/
				(1<<21) | /*maintain mb_up_in addr*/
				(0<<20) | /*mb_up_in incremental*/
				(1))); /*dblk enable*/
    
      /*######### DHA node chain ##########*/
      write_reg((&y_chain[9]), 0x80000000);
      write_reg((&y_chain[10]), (0x2<<16 | /*DCT*/
				 1<<6 | //ND_DN : node done
				 0x2<<4 | // Address node
				 8)); //handshake
      write_reg((&y_chain[11]), do_get_phy_addr(DBLK0_BASE + DBLK_OFST_DHA));   //dblk dha addr
      if ( i == DBK_SFIFO_DEP -1 ){//next address
	write_reg((&y_chain[12]),do_get_phy_addr(DBLK_CHAIN0_P_BASE));
      }else {
	write_reg((&y_chain[12]),do_get_phy_addr(y_chain+DBLK_CHAIN_WD));
      }
      
      write_reg((&y_chain[13]), do_get_phy_addr(VPU_V_BASE + VPU_SCHE2));   //sch
      write_reg((&y_chain[14]), 0x0); //sch id
      y_chain +=  DBLK_CHAIN_WD;
    }
    
    unsigned int *uv_chain = (unsigned int *)(DBLK_CHAIN1_P_BASE);
    for(i = 0 ; i < DBK_SFIFO_DEP; i++){
      write_reg((&uv_chain[0]), 0x80000000);
      write_reg((&uv_chain[1]), ((0x18<<24) | /*CSA*/
				 (0x7<<16) | /*DCT*/ //13
				 (1<<6) | /*cfg nddn*/
				 (0<<4) | /*0:data node, 1:ofst, 2:addr */
				 (1<<3) | /*need handsake*/
				 (0<<2) | /*ND_CKG*/
				 (0))); /*return to dha*/ 
      
      //write_reg((&uv_chain[2],DBLK_REG18( 0,0, 0)));
      
      //reg14 : h264_mv_addr;
      write_reg((&uv_chain[3]), (dblk_mv_addr + i*56*4));
      
      
      //write_reg((&uv_chain[4]), 0);
      //write_reg((&uv_chain[5]), 0);
      //write_reg((&uv_chain[6]), 0);
      //write_reg((&uv_chain[7]), DBLK_REG04( 0, 0xff, 0, 0x0, 0, 1,0,1,0,1, 1)));
      
      //reg00 : dblk_ctrl;
      write_reg((&uv_chain[8]), ((0<<31) | /*only maintain the addr, no transfer and filter*/
				 (0<<30) | /*transfer for ever*/
				 (1<<29) | /*maintain mb_out addr*/
				 (0<<28) | /*mb_out exchange*/
				 (1<<27) | /*mb_out wrap*/
				 (1<<26) | /*maintain mb_in addr*/
				 (0<<25) | /*mb_in exchange*/
				 (1<<24) | /*mb_in wrap*/
				 (1<<23) | /*maintain mb_up_out addr*/
				 (0<<22) | /*mb_up_out incremental*/
				 (1<<21) | /*maintain mb_up_in addr*/
				 (0<<20) | /*mb_up_in incremental*/
				 (1))); /*dblk enable*/
    
      /*######### DHA node chain ##########*/
      write_reg((&uv_chain[9]), 0x80000000);
      write_reg((&uv_chain[10]), (0x2<<16 | /*DCT*/
				  1<<6 | //ND_DN : node done
				  0x2<<4 | // Address node
				  8)); //handshake
      write_reg((&uv_chain[11]), do_get_phy_addr(DBLK1_BASE + DBLK_OFST_DHA));   //dblk dha addr
      if ( i == DBK_SFIFO_DEP -1 ){//next address
	write_reg((&uv_chain[12]),do_get_phy_addr(DBLK_CHAIN1_P_BASE));
      }else {
	write_reg((&uv_chain[12]),do_get_phy_addr(uv_chain+DBLK_CHAIN_WD));
      }
      
      write_reg((&uv_chain[13]), do_get_phy_addr(VPU_V_BASE + VPU_SCHE3));   //sch
      write_reg((&uv_chain[14]), 0x0); //sch id
      uv_chain +=  DBLK_CHAIN_WD;
    }
  }
}
#endif
