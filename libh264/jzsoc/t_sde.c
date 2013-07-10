// configure and start HW
#undef printf
extern short mpFrame;

void sde_slice_end(H264Context *h)
{
  MpegEncContext * const s = &h->s;
  int i, j;


  // set slice end
  unsigned int slice_info_0 = (((1) << 24) +
			       ((sde_slice_info_ptr->slice_type & 0x7) << 0) +
			       ((sde_slice_info_ptr->picture_structure & 0x3) << 3) +
			       (((!!sde_slice_info_ptr->transform_8x8_mode) & 0x1) << 5) +
			       (((!!sde_slice_info_ptr->constrained_intra_pred) & 0x1) << 6) +
			       (((!!sde_slice_info_ptr->direct_8x8_inference_flag) & 0x1) << 7) +
			       (((!!sde_slice_info_ptr->direct_spatial_mv_pred) & 0x1) << 8) +
			       ((sde_slice_info_ptr->x264_build & 0x7F) << 9) +
			       (((sde_slice_info_ptr->ref0_count) & 0xF) << 16) +
			       (((sde_slice_info_ptr->ref1_count) & 0xF) << 20)
			       );
  SET_SLICE_INFO_0(slice_info_0);

  // polling slice end done
  POLL_SLICE_END();

#if 0
  // write slice_end to frm_ctrl
  {
    int * slice_ctrl_ptr = curr_slice_info_ctrl + 2*(mb_num_sde - 1);
    unsigned int last_mb_ctrl0 = slice_ctrl_ptr[0];
    last_mb_ctrl0 |= (1 << 25);
    slice_ctrl_ptr[0] = last_mb_ctrl0;
  }

  // check dir dma output
  int sde_fmv_error = 0;
  int slice_first_mb_num;
  slice_first_mb_num = sde_slice_info_ptr->start_mx + sde_slice_info_ptr->start_my * sde_slice_info_ptr->mb_width;
  int slice_last_mb_num;
  slice_last_mb_num = mx_sde + my_sde * sde_slice_info_ptr->mb_width;
  int slice_max_mb_num = sde_slice_info_ptr->mb_width * sde_slice_info_ptr->mb_height;
  if (slice_last_mb_num >= slice_max_mb_num)
    slice_last_mb_num = slice_max_mb_num;
  int slice_mb_num = slice_last_mb_num - slice_first_mb_num;
  printf(" slice_first_mb_num:%d, slice_last_mb_num:%d, \n",slice_first_mb_num,slice_last_mb_num);

  int * hw_ctrl_p = s->current_picture.frm_info_ctrl + 2*slice_first_mb_num;
  int * hw_mv_p = s->current_picture.frm_info_mv + 32*slice_first_mb_num;
  for (i=0; i<2*slice_mb_num; i++){
    //if (mpFrame == 2) printf(" dir dout ctrl error at %d, hw:0x%x, sw:0x%x \n",i,hw_ctrl_p[i],curr_slice_info_ctrl[i]);
    if (hw_ctrl_p[i] != curr_slice_info_ctrl[i]){
      printf(" dir dout ctrl error at %d, hw:0x%x, sw:0x%x \n",i,hw_ctrl_p[i],curr_slice_info_ctrl[i]);
      sde_fmv_error = 1;
    }
  }

  for (i=0; i<slice_mv_cnt; i++){
    if (hw_mv_p[i] != curr_slice_info_mv[i]){
      printf(" dir dout mv error at %d, hw:0x%x, sw:0x%x \n",i,hw_mv_p[i],curr_slice_info_mv[i]);
      sde_fmv_error = 1;
    }
  }

  //printf(" slice end check done \n");

  if (sde_fmv_error){
    printf(" ---------------------------------------------- \n");
    printf(" * SDE DIR DOOUT ERROR at frame:%d. \n",mpFrame);
    printf(" * EXITING MPLAYER \n");
    printf(" ---------------------------------------------- \n");
    exit_player_with_rc(NULL,0);
    //while(1);
  }
#endif

}

void sde_slice_init(H264Context *h)
{
  MpegEncContext * s = &h->s;
  int mb_num= s->mb_x + s->mb_y*s->mb_width;
  int i, j, k;
  sde_slice_info_ptr = &sde_slice_info;//AUX_SLICE_INFO_BASE;//;
  mb_num_sde = 0;
  frm_info_mv_cnt = 0;
  slice_mv_cnt = 0;

  int slice_type_new = ( (h->slice_type == FF_B_TYPE) ? 4 : ((h->slice_type == FF_P_TYPE) ? 2 : 1) );
  sde_slice_info_ptr->slice_type = slice_type_new;
  sde_slice_info_ptr->picture_structure = (s->picture_structure != PICT_FRAME);
  sde_slice_info_ptr->qscale = s->qscale;
  sde_slice_info_ptr->transform_8x8_mode = h->pps.transform_8x8_mode;
  sde_slice_info_ptr->constrained_intra_pred = h->pps.constrained_intra_pred;
  sde_slice_info_ptr->direct_8x8_inference_flag = h->sps.direct_8x8_inference_flag;
  sde_slice_info_ptr->direct_spatial_mv_pred = h->direct_spatial_mv_pred;
  sde_slice_info_ptr->x264_build = h->x264_build>33 || !h->x264_build;
  sde_slice_info_ptr->ref0_count = (h->ref_count[0] == 16) ? 15 : h->ref_count[0];
  sde_slice_info_ptr->ref1_count = (h->ref_count[1] == 16) ? 15 : h->ref_count[1];
  sde_slice_info_ptr->list_count = h->list_count;
  sde_slice_info_ptr->start_mx = s->mb_x;
  sde_slice_info_ptr->start_my = s->mb_y;
  sde_slice_info_ptr->mb_width = s->mb_width;
  sde_slice_info_ptr->mb_height = s->mb_height;  

  if (frm_info_slice_start_mb_init == 0) {
    for (i=0; i<32; i++)
      s->current_picture.frm_info_slice_start_mb[i] = INT_MAX;
    frm_info_slice_start_mb_init = 1;
  }

  int ref_frm_start_mb = 0;
  if ( (h->slice_type == FF_B_TYPE) && (slice_num_sde)) {
    for (i=0; i<32; i++){
      if ( (mb_num >= h->ref_list[1][0].frm_info_slice_start_mb[i]) &&
	   (mb_num < h->ref_list[1][0].frm_info_slice_start_mb[i+1]) )
	break;
    }
    ref_frm_start_mb = h->ref_list[1][0].frm_info_slice_start_mb[i];
  }
  //printf(" ref_frm_start_mb: %d \n",ref_frm_start_mb);

  s->current_picture.frm_info_slice_start_mb[slice_num_sde] = mb_num;
  slice_num_sde++;

#ifdef SDE_DBG_HW
  if (!(mpFrame >= DBG_SDE_FRM))
    return 0;
#endif

  // global ctrl, codec_id
  int auto_syn_en = 1;
  unsigned int codec_id = h->pps.cabac ? 1 : 3;
  SET_GLOBAL_CTRL(
		  (codec_id << 4) +
		  (auto_syn_en << 3) + 
		  1
		  );

  volatile int * sde_sync_end = SDE_SYNC_DOUT_ADDR;
  SET_SDE_SYNC_END(do_get_phy_addr((unsigned int)sde_sync_end));
  write_reg( (((unsigned int)sde_sync_end)), (0) );

  // bs init
  unsigned int bs_addr;
  unsigned int bs_ofst;
  if (h->pps.cabac) {
    bs_addr = s->gb.buffer + get_bits_count(&s->gb)/8;
    bs_ofst = (bs_addr & 0x3) << 3;
    bs_addr = bs_addr & 0xFFFFFFFC;
  } else {
    bs_addr = (unsigned int)(s->gb.buffer + (s->gb.index >> 3)) & 0xFFFFFFFC;
    bs_ofst = ((((unsigned int)s->gb.buffer & 0x3) << 3) + ((unsigned int)s->gb.index & 0x1F)) & 0x1F;
  }
  sde_init_ofst = get_bits_count(&s->gb) - bs_ofst;

  int * wb_p = curr_slice_info_mv - 1;
#define INFO_MV_LEN ((1<<12)>>3)    
  for(i=0; i<INFO_MV_LEN; i++){
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
    S32SDI(xr0,wb_p,0x4);
  }

  SET_BS_ADDR(do_get_phy_addr((unsigned int)bs_addr));
  write_reg((SDE_VBASE + SDE_BS_OFST_OFST), (bs_ofst));

  // direct prediction
  int32_t * ref_frm_ctrl;
  int32_t * ref_frm_mv;
  int32_t * curr_frm_ctrl;
  int32_t * curr_frm_mv;
  ref_frm_ctrl = h->ref_list[1][0].frm_info_ctrl + ref_frm_start_mb*2;
  ref_frm_mv   = h->ref_list[1][0].frm_info_mv   + ref_frm_start_mb*32;
  curr_frm_ctrl= s->current_picture.frm_info_ctrl+ mb_num*2;
  curr_frm_mv  = s->current_picture.frm_info_mv  + mb_num*32;
  SET_SDE_DIRECT_CIN((unsigned int)(do_get_phy_addr(ref_frm_ctrl)));
  SET_SDE_DIRECT_MIN((unsigned int)(do_get_phy_addr(ref_frm_mv)));
  SET_SDE_DIRECT_COUT((unsigned int)(do_get_phy_addr(curr_frm_ctrl)));
  SET_SDE_DIRECT_MOUT((unsigned int)(do_get_phy_addr(curr_frm_mv)));
#if 0
  printf("mpframe = %d,ref_frm_ctrl:0x%x-%x,  ref_frm_mv:0x%x-%x \n",mpFrame,ref_frm_ctrl,
	 do_get_phy_addr(ref_frm_ctrl),
	 ref_frm_mv, do_get_phy_addr(ref_frm_mv));
  printf(" curr_frm_ctrl:0x%x, curr_frm_mv:0x%x \n",curr_frm_ctrl,curr_frm_mv);
#endif

  unsigned int slice_info_1 = (((unsigned int)mb_num & 0xFFFF) +
			       (((unsigned int)(mb_num - ref_frm_start_mb) & 0xFFFF) << 16)
			       );
  SET_SLICE_INFO_1(slice_info_1);

  // slice begin and polling end
  unsigned int slice_info_0 = (((1) << 31) +
			       ((sde_slice_info_ptr->slice_type & 0x7) << 0) +
			       ((sde_slice_info_ptr->picture_structure & 0x3) << 3) +
			       (((!!sde_slice_info_ptr->transform_8x8_mode) & 0x1) << 5) +
			       (((!!sde_slice_info_ptr->constrained_intra_pred) & 0x1) << 6) +
			       (((!!sde_slice_info_ptr->direct_8x8_inference_flag) & 0x1) << 7) +
			       (((!!sde_slice_info_ptr->direct_spatial_mv_pred) & 0x1) << 8) +
			       ((sde_slice_info_ptr->x264_build & 0x7F) << 9) +
			       (((sde_slice_info_ptr->ref0_count) & 0xF) << 16) +
			       (((sde_slice_info_ptr->ref1_count) & 0xF) << 20)
			       );

  //SET_VPU_GLBC(0, 0, 0, 0, 0);

  SET_SLICE_INFO_0(slice_info_0);

  POLL_SLICE_INIT();
  //printf(" frm: %d, slice init done,  slice_type:%d \n",mpFrame,slice_type_new);

  unsigned int * sde_table_base = SDE_VBASE + SDE_CTX_TBL_OFST;

  if (h->pps.cabac) {
    // cabac ctx table
    for(i=0;i<460;i+=2) {
      sde_table_base[i] = lps_comb[h->cabac_state[i]];
      sde_table_base[i+1] = lps_comb[h->cabac_state[i+1]];
    }
    sde_table_base[276] = lps_comb[126];
  } else {
    // cavlc ctx table
    int tbl, size;
    for (tbl = 0; tbl < 7; tbl++) {
      unsigned int * hw_base = sde_table_base + (sde_vlc2_sta[tbl].ram_ofst >> 1);
      unsigned int * tbl_base = &sde_vlc2_table[tbl][0];
      int size = (sde_vlc2_sta[tbl].size + 1) >> 1;
      for (i = 0; i < size; i++){
	hw_base[i] = tbl_base[i];
      }
    }
  }

  // direct prediction scal table
  int *map_col_to_list0[2] = {h->map_col_to_list0[0], h->map_col_to_list0[1]};
  int *dist_scale_factor = h->dist_scale_factor;
  for (i=0; i<16; i++){
    int tmp = map_col_to_list0[0][i] + (map_col_to_list0[1][i] << 5) + (dist_scale_factor[i] << 16);
    sde_table_base[480 + i] = tmp;
  }

}

void sde_frame_init(H264Context *h)
{
  MpegEncContext * s = &h->s;
  int i;
  slice_num_sde = 0;
  frm_info_slice_start_mb_init = 0;
}
