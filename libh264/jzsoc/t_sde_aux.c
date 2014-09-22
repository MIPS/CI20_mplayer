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

// configure and start HW

void cfg_sde(int mb_x,int mb_num, unsigned char mb_width, unsigned int *neighbor_info, unsigned int sde_ctrl_dout_caddr, 
	     unsigned int sde_res_out_addr,struct NEIGHBOR_INFO *aux_sde_top_info_ptr)
{
  int i, j, k;

  int left_avail= (mb_num > 0) && (mb_x > 0);
  int top_avail = (mb_num >= mb_width);
  int topleft_avail = (mb_num >= (mb_width + 1)) && (mb_x > 0);
  int topright_avail = (mb_num  >= (mb_width - 1)) && (mb_x < mb_width-1);

  unsigned int neighbor_avail = ((!!left_avail << 0) + (!!top_avail << 1) +
				 (!!topleft_avail << 2) + (!!topright_avail << 3));
  int ni=0;
  neighbor_info[ni++] = mb_num + (neighbor_avail << 24);
  neighbor_info[ni++] = mb_num + (0x12<<24);
  neighbor_info[ni++] = aux_do_get_phy_addr(sde_ctrl_dout_caddr);
  neighbor_info[ni++] = aux_do_get_phy_addr(sde_res_out_addr);

  int neighbor_info_size = (((sizeof(struct NEIGHBOR_INFO))+3)/4);
  for (i=0; i<neighbor_info_size;i++){
    neighbor_info[ni++] = ((int *)(&aux_sde_top_info_ptr[mb_x].mb_type))[i];
  }
  // topright info
  neighbor_info[ni++] = aux_sde_top_info_ptr[mb_x+1].mb_type;
  neighbor_info[ni++] = aux_sde_top_info_ptr[mb_x+1].pred_mode_or_ref;
  neighbor_info[ni++] = (*(int *)(aux_sde_top_info_ptr[mb_x+1].mv[0][0]));
  neighbor_info[ni++] = (*(int *)(aux_sde_top_info_ptr[mb_x+1].mv[1][0]));
}

int backup_sde_bac_aux(int mb_x, int ref_di,
		       unsigned int sde_ctrl_dout_caddr,
		       struct NEIGHBOR_INFO * aux_sde_top_info_ptr
		       )
{
  unsigned int src = (unsigned int *)sde_ctrl_dout_caddr;
  unsigned int dst = (unsigned int *)&aux_sde_top_info_ptr[mb_x];
  S32I2M(xr15, (ref_di<<24)| 0xFFFFFF);
  /* xr1[15:0]: mb_type; 
     xr2[31:24]: direct, xr2[15:0]: cbp;
     xr3[23:16]: nnz_br;
  */
  S32LDD(xr1, src, 0);
  S32LDI(xr2, src, 4);
  S32LDI(xr3, src, 20);
  S32AND(xr2, xr2, xr15);
  S32STD(xr1, dst, 0);
  S32SFL(xr4, xr3, xr2, xr0, ptn3);
  D32SLR(xr4, xr4, xr0, xr0, 8);
  S32SFL(xr0, xr4, xr2, xr4, ptn3);
  /* xr4[31:24]: nnz_br, xr4[23:16]: direct, xr4[15:0]: cbp; */
  S32SDI(xr4, dst, 4);
  unsigned int mb_type = S32M2I(xr1);
  S32LDI(xr7, src, 4);
  S32LDI(xr8, src, 4);
  S32LDI(xr9, src, 4);
  if (mb_type & 0x4){
    struct MB_INTRA_BAC * mb_intra = (struct MB_INTRA_BAC *)sde_ctrl_dout_caddr;
    unsigned int intra4x4_pred_mode1 = mb_intra->intra4x4_pred_mode1;
    unsigned char luma_neighbor      = mb_intra->luma_neighbor;
    unsigned char chroma_neighbor    = mb_intra->chroma_neighbor;
    aux_sde_top_info_ptr[mb_x].pred_mode_or_ref = ((chroma_neighbor << 16) + 
						   ((intra4x4_pred_mode1 & 0xFF000000) >> 16) +
						   ((intra4x4_pred_mode1 & 0xF000) >> 8) +
						   (((luma_neighbor) & 0xF0) >> 4)
						   );
  }else{
    S32SFL(xr8, xr8, xr7, xr0, ptn3);
    S32SDI(xr8, dst, 4);
    S32LDI(xr1, src, 12);
    S32LDI(xr2, src, 4);
    S32LDI(xr3, src, 4);
    S32LDI(xr4, src, 4);
    S32LDI(xr5, src, 20);
    S32LDI(xr6, src, 4);
    S32LDI(xr7, src, 4);
    S32LDI(xr8, src, 4);
    S32LDI(xr9, src, 20);
    S32LDI(xr10, src, 4);
    S32LDI(xr11, src, 4);
    S32LDI(xr12, src, 4);
    S32SDI(xr1, dst, 4);
    S32SDI(xr2, dst, 4);
    S32SDI(xr3, dst, 4);
    S32SDI(xr4, dst, 4);
    S32SDI(xr9, dst, 4);
    S32SDI(xr10, dst, 4);
    S32SDI(xr11, dst, 4);
    S32SDI(xr12, dst, 4);
    S32LDI(xr1, src, 20);
    S32LDI(xr2, src, 4);
    S32LDI(xr3, src, 4);
    S32LDI(xr4, src, 4);
    S32SDI(xr5, dst, 4);
    S32SDI(xr6, dst, 4);
    S32SDI(xr7, dst, 4);
    S32SDI(xr8, dst, 4);
    S32SDI(xr1, dst, 4);
    S32SDI(xr2, dst, 4);
    S32SDI(xr3, dst, 4);
    S32SDI(xr4, dst, 4);
  }
  int next_aux_sde_mb_terminate = (mb_type >> 24) & 1;
  return next_aux_sde_mb_terminate;
}
