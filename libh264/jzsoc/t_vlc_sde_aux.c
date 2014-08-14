// configure and start HW

void vlc_cfg_sde(int mb_x,int mb_num, unsigned char mb_width, unsigned int *neighbor_info, unsigned int sde_ctrl_dout_caddr, 
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

void backup_sde_vlc_aux(int mb_x,
		       unsigned int sde_ctrl_dout_caddr,
		       struct NEIGHBOR_INFO *aux_sde_top_info_ptr
		       )
{
  int i, j, k;
  struct NEIGHBOR_INFO *sde_bak_top_info_ptr = &aux_sde_top_info_ptr[mb_x].mb_type;
  struct MB_INTRA_VLC *mb_intra;
  struct MB_INTER_P_VLC *mb_inter_p;
  struct MB_INTER_B_VLC *mb_inter_b;
  mb_intra   = (struct MB_INTRA_VLC   *)sde_ctrl_dout_caddr;
  mb_inter_p = (struct MB_INTER_P_VLC *)sde_ctrl_dout_caddr;
  mb_inter_b = (struct MB_INTER_B_VLC *)sde_ctrl_dout_caddr;

  unsigned short mb_type = mb_intra->mb_type;
  sde_bak_top_info_ptr->mb_type = mb_type;
  sde_bak_top_info_ptr->nnz_cbp = mb_intra->nnz_bottom;

  if (mb_type & 0x4){
    unsigned int intra4x4_pred_mode1 = mb_intra->intra4x4_pred_mode1;
    unsigned char luma_neighbor      = mb_intra->luma_neighbor;
    unsigned char chroma_neighbor    = mb_intra->chroma_neighbor;
    sde_bak_top_info_ptr->pred_mode_or_ref = ((chroma_neighbor << 16) + 
					      ((intra4x4_pred_mode1 & 0xFF000000) >> 16) +
					      ((intra4x4_pred_mode1 & 0xF000) >> 8) +
					      (((luma_neighbor) & 0xF0) >> 4)
					      );
  }else{
    int * sde_ref_p = &mb_inter_b->ref[0][0];
    sde_bak_top_info_ptr->pred_mode_or_ref = (sde_ref_p[0] >> 16) | (sde_ref_p[1] & 0xFFFF0000);
    int * src = &mb_inter_b->mv_bottom_l0[0][0];
    int * top_dst = &sde_bak_top_info_ptr->mv[0][0][0];
    for (i=0; i<4; i++) {
      // mv top l0
      top_dst[i] = src[i];
      // mv top l1
      top_dst[i + 4] = src[i + 8];
    }
  }
}


