
unsigned int left_mb_type;
unsigned int ref_left_dblk[2];
unsigned int mv_left_dblk[2][4];
unsigned int nnz_left_dblk;
short mb_qp_left;
unsigned char mb_qp_up[MB_W_1080P];

void inline hw_dblk_mb_aux_c(unsigned int *sde_ptr,int slice_type,
			     unsigned int mb_x ,unsigned int mb_y,
			     int y_qp ,int u_qp,int v_qp,
			     unsigned int *y_chain, unsigned int *uv_chain, 
			     unsigned int id,NEIGHBOR_INFO_p aux_sde_top_info_ptr,
			     unsigned int dblk_mv_addr_base,
			     int h264_depth_offset, int deblock_filter
			     ) {  

  static const short mv_msk_table[8]={0x0,0xee,0xae,0xee, 0xea,0xee,0xae,0xea};

  static const short edg_msk_table[8] = {0,0,0xaa,0xaa, 0,0xee,0xaa,0xee};
  static const char (*vpu_chr_tab)[256] = CHR_QT_TAB_BASE;
		       
  struct MB_INTER_P_BAC * mb_inter_p;
  struct MB_INTER_B_BAC * mb_inter_b;

  mb_inter_p = (struct MB_INTER_P_BAC *)(sde_ptr);
  mb_inter_b = (struct MB_INTER_B_BAC *)(sde_ptr);
  
  int mb_type= mb_inter_p->mb_type;

  int b_slice = slice_type == SDE_B_TYPE; 
  int i_slice = slice_type == SDE_I_TYPE; 
  int is_intra = !!(mb_type&0x3);


  if(mb_x == 0){
    mb_qp_left = 0;
    nnz_left_dblk = 0;

    ref_left_dblk[0] = 0;
    ref_left_dblk[1] = 0;
    
    mv_left_dblk[0][0] = 0;
    mv_left_dblk[0][1] = 0;
    mv_left_dblk[0][2] = 0;
    mv_left_dblk[0][3] = 0;

    mv_left_dblk[1][0] = 0;
    mv_left_dblk[1][1] = 0;
    mv_left_dblk[1][2] = 0;
    mv_left_dblk[1][3] = 0;
  }


  if(mb_y == 0){
    mb_qp_up[mb_x] = 0;
  }

  //reg 0x10
  int nnz_br = aux_sde_top_info_ptr->nnz_cbp;
  int nnz_dct_tmp = mb_inter_p->nnz_dct;

  int nnz_dct = ((nnz_dct_tmp  & 0xC3C3) |
	      ((nnz_dct_tmp & 0x0C0C )<<2) |
	      ((nnz_dct_tmp & 0x3030 )>>2) );

  nnz_br = (nnz_br&0xF000000)>>8;
  int nnz = (nnz_dct | 
	     (nnz_br) | 
	     (nnz_left_dblk));

  nnz_left_dblk = ((nnz_dct_tmp &( 1 << 5))<<15 |
				 (nnz_dct_tmp &( 1 << 7 ))<<14 |
				 (nnz_dct_tmp &( 1 << 13))<<9 |
				 (nnz_dct_tmp &( 1 << 15))<<8 );
  int mv_msk = 0;
  mv_msk = mv_msk_table[(mb_type&0x38)>>3];
  //reg10 : h264_mv_mask_nnz;
  uv_chain[4] = y_chain[4] = DBLK_REG10( mv_msk, nnz);//30
  //reg18 : h264_depth_offset; 28
  /*
  uv_chain[2] = y_chain[2] = DBLK_REG18( ofst_a, 
					   ofst_b, 
					   b_slice);
  */
  uv_chain[2] = y_chain[2] = h264_depth_offset;
  int mv_reduce_len;
  if(i_slice){
    mv_reduce_len = 0;
  }else{
    mv_reduce_len = mb_inter_b->mv_len + 10*b_slice +11;  
  }

  int mb_up_type = aux_sde_top_info_ptr->mb_type;
  char dblk_use_chroma_qp_left0 = vpu_chr_tab[0][mb_qp_left];
  char dblk_use_chroma_qp_top0  = vpu_chr_tab[0][mb_qp_up[mb_x] ];//dblk_use_qp_top);

  char dblk_use_chroma_qp_left1 = vpu_chr_tab[1][mb_qp_left];
  char dblk_use_chroma_qp_top1  = vpu_chr_tab[1][mb_qp_up[mb_x] ];//dblk_use_qp_top);

  //char dblk_use_chroma_qp_left0 = *(unsigned char *)(CHR_QT_TAB_BASE + mb_qp_left);
  //char dblk_use_chroma_qp_top0  = *(unsigned char *)(CHR_QT_TAB_BASE + mb_qp_up[mb_x]);//dblk_use_qp_top);

  //char dblk_use_chroma_qp_left1 = *(unsigned char *)(CHR_QT_TAB_BASE+256 + mb_qp_left);
  //char dblk_use_chroma_qp_top1  = *(unsigned char *)(CHR_QT_TAB_BASE+256 + mb_qp_up[mb_x]);//dblk_use_qp_top);

  int qp_y_left = ((y_qp + mb_qp_left + 1) >> 1);
  int qp_y_up   = ((y_qp + mb_qp_up[mb_x] + 1) >> 1);
  int qp_u_left = ((u_qp + dblk_use_chroma_qp_left0 + 1) >> 1);
  int qp_u_up   = ((u_qp + dblk_use_chroma_qp_top0  + 1) >> 1);
  int qp_v_left = ((v_qp + dblk_use_chroma_qp_left1 + 1) >> 1);
  int qp_v_up   = ((v_qp + dblk_use_chroma_qp_top1  + 1) >> 1);

  //reg0C : h264_qp_vu;//31
  uv_chain[5] = y_chain[5] = DBLK_REG0C( qp_v_left, v_qp, qp_v_up, \
			  qp_u_left, u_qp);
  uv_chain[6] = y_chain[6] = DBLK_REG08( mv_reduce_len, qp_u_up, qp_y_left, \
			  y_qp, qp_y_up);

  int v0_smth = (((mb_type& (1<<SDE_MB_TYPE_16x16)) | (mb_type& ( 1<<SDE_MB_TYPE_8x16 ))) &
		  (left_mb_type& ( 1<<SDE_MB_TYPE_16x16 | 1<<SDE_MB_TYPE_8x16)))? 0x1 : 0;
  int h0_smth = ((mb_type& ( 1<<SDE_MB_TYPE_16x16)) &
		 (mb_up_type& (1<<SDE_MB_TYPE_16x16 | 1<<SDE_MB_TYPE_16x8))) ? 0x10 : 0;
  int v2_smth = ( mb_type& ( 1<<SDE_MB_TYPE_8x16 )) ? 0x4 : 0;
  int h2_smth = ( mb_type& ( 1<<SDE_MB_TYPE_16x8 )) ? 0x40 : 0;
  int mv_smth = (v0_smth | v2_smth | h0_smth | h2_smth);

  int dblk_start  = (mb_x == 0)? 1 : 0;
  dblk_start |= (mb_y == 0) ? 0x10 : 0;

  int edg_msk;
  if (!deblock_filter) {
    edg_msk = 0xFFFF;
  }else{
    edg_msk = edg_msk_table[((mb_type &0x8)>>3) | ((mb_type&0xc00)>>9)];
    edg_msk |= dblk_start;
  }

  int intra_curr = !!(mb_type&0x3 );
  int intra_left = !!(left_mb_type&0x3 );
  int intra_up   = !!(mb_up_type  &0x3 );
  /* mv_smooth,edg_msk,row_end,yuv_flag,dma_cfg_bs,mv_reduce,
     intra_left,intra_curr,intra_up,hw_bs_h264,video_fmt */
  //reg04
  y_chain[7] =  DBLK_REG04( mv_smth, edg_msk, 0, \
			    0x4, 0, 1, \
			    intra_left, intra_curr, intra_up, \
			    1, 1);

  y_chain[DBLK_CHN_SCH_ID_IDX] = id;
  uv_chain[DBLK_CHN_SCH_ID_IDX] = id;
  uv_chain[7] = DBLK_REG04( mv_smth, edg_msk, 0, \
			     0x3, 0, 1, \
			     intra_left, intra_curr, intra_up, \
			     1, 1);
  {

  int i;

  if( !i_slice){
    if(!is_intra){
      int *dblk_mv_addr = dblk_mv_addr_base;
      //int *dblk_mv_addr = *(y_chain + 3); //badly!!

      int cc;
      int up_ref;
      int tmp;
      int *ptr;
      int ptr_tmp[4];

      up_ref = aux_sde_top_info_ptr->pred_mode_or_ref & 0xFFFF;

      tmp = *(int *)(&mb_inter_b->ref[0][0]);
      *(dblk_mv_addr) = tmp;
      *(dblk_mv_addr+1) = (up_ref |ref_left_dblk[0]); //left + up ref
      dblk_mv_addr +=2;

      //backup new
      tmp = tmp & 0xFF000000 | ((tmp & 0xFF00)<<8 );
      ref_left_dblk[0] = tmp;
      
      if(b_slice){
	up_ref = (aux_sde_top_info_ptr->pred_mode_or_ref&0xFFFF0000)>>16;

	tmp = *(int *)(&mb_inter_b->ref[1][0]);
	*(dblk_mv_addr) = tmp;
	*(dblk_mv_addr+1) = (up_ref |ref_left_dblk[1]); //left + up ref
	dblk_mv_addr +=2;

	//backup new
	tmp = tmp & 0xFF000000 | ((tmp & 0xFF00)<<8 );
	ref_left_dblk[1] = tmp;

      }

      //----------------- calculate mb_type_info--------
      //int mb_type_info = mb_type_info_table[0][(mb_type&0x78)>>3];
      unsigned int mv_mode = mb_inter_b->mv_mode;
      int mb_type_info = mv_mode & 0x3;
      int sub_mb_type_info_tmp ;

      sub_mb_type_info_tmp  = (mv_mode & 0xc)<<2 ;
      sub_mb_type_info_tmp |= (mv_mode & 0x30)<<4 ;
      sub_mb_type_info_tmp |= (mv_mode & 0xc0)<<10 ;
      sub_mb_type_info_tmp |= (mv_mode & 0x300)<<16 ;

      mb_type_info = mb_type_info | sub_mb_type_info_tmp;
      
      *(dblk_mv_addr++) = mb_type_info;

      ptr = (int *)(&aux_sde_top_info_ptr->mv[0][0][0]);
      S32LDD(xr1 , ptr ,0);
      S32LDD(xr2 , ptr ,4);
      S32LDD(xr3 , ptr ,8);
      S32LDD(xr4 , ptr ,12);

      ptr = (int *)(&mb_inter_p->mv_right_l0[0][0]);
      S32LDD(xr9  , ptr ,0);
      S32LDD(xr10 , ptr ,4);
      S32LDD(xr11 , ptr ,8);
      S32LDD(xr12 , ptr ,12);

      ptr = (int *)(&mv_left_dblk[0][0]);
      S32LDD(xr5 , ptr ,0);
      S32LDD(xr6 , ptr ,4);
      S32LDD(xr7 , ptr ,8);
      S32LDD(xr8 , ptr ,12);

      //backup left
      S32STD(xr9  ,ptr ,0);
      S32STD(xr10 ,ptr ,4);
      S32STD(xr11 ,ptr ,8);
      S32STD(xr12 ,ptr ,12);

      S32STD(xr1 ,dblk_mv_addr ,0);
      S32STD(xr2 ,dblk_mv_addr ,4);
      S32STD(xr3 ,dblk_mv_addr ,8);
      S32STD(xr4 ,dblk_mv_addr ,12);


      S32STD(xr5 ,dblk_mv_addr ,16);
      S32STD(xr6 ,dblk_mv_addr ,20);
      S32STD(xr7 ,dblk_mv_addr ,24);
      S32STD(xr8 ,dblk_mv_addr ,28);


      dblk_mv_addr +=8;

      int len = mb_inter_b->mv_len;

      if(b_slice){

	ptr = (int *)(&aux_sde_top_info_ptr->mv[1][0][0]);
	S32LDD(xr1 , ptr ,0);
	S32LDD(xr2 , ptr ,4);
	S32LDD(xr3 , ptr ,8);
	S32LDD(xr4 , ptr ,12);
	
	ptr = (int *)(&mb_inter_b->mv_right_l1[0][0]);
	S32LDD(xr9  , ptr ,0);
	S32LDD(xr10 , ptr ,4);
	S32LDD(xr11 , ptr ,8);
	S32LDD(xr12 , ptr ,12);

	ptr = (int *)(&mv_left_dblk[1][0]);
	S32LDD(xr5 , ptr ,0);
	S32LDD(xr6 , ptr ,4);
	S32LDD(xr7 , ptr ,8);
	S32LDD(xr8 , ptr ,12);

	//backup left
	S32STD(xr9  ,ptr ,0);
	S32STD(xr10 ,ptr ,4);
	S32STD(xr11 ,ptr ,8);
	S32STD(xr12 ,ptr ,12);
	
	S32STD(xr1 ,dblk_mv_addr ,0);
	S32STD(xr2 ,dblk_mv_addr ,4);
	S32STD(xr3 ,dblk_mv_addr ,8);
	S32STD(xr4 ,dblk_mv_addr ,12);
	
	S32STD(xr5 ,dblk_mv_addr ,16);
	S32STD(xr6 ,dblk_mv_addr ,20);
	S32STD(xr7 ,dblk_mv_addr ,24);
	S32STD(xr8 ,dblk_mv_addr ,28);
	
	dblk_mv_addr +=8;
	

	ptr = (int *)(&mb_inter_b->mv[0][0]);
	ptr--;
	dblk_mv_addr--;
	while(len--){
	  S32LDI(xr1 , ptr ,4);
	  S32SDI(xr1 ,dblk_mv_addr ,4);
	};

      }else{

	ptr = (int *)(&mb_inter_p->mv[0][0]);
	ptr--;
	dblk_mv_addr--;
	while(len--){
	  S32LDI(xr1 , ptr ,4);
	  S32SDI(xr1 ,dblk_mv_addr ,4);
	};

      }
   
    }else{
    //May need do something here?
    ref_left_dblk[0] = 0;
    ref_left_dblk[1] = 0;

    }
  }
}
  //bakup qp and left_mb_type
  mb_qp_up[mb_x] = y_qp;
  mb_qp_left = y_qp;
  left_mb_type = mb_type;
}
