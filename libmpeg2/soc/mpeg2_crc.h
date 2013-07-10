#ifndef __MPEG2_CRC_H__
#define __MPEG2_CRC_H__

#ifdef MPEG2_SCH_CONTROL
static void prin_crc(const char * src_y, const char * src_uv, int mb_num, int slice_num)
{
    unsigned char * crc_y = src_y;
    unsigned char * crc_uv = src_uv;
    int i, j;
    FILE * fd = fopen("lee_rst.c", "a");

    fprintf(fd, "---------------------- slice : %d ----------------------\n", slice_num);
    for(i = 0; i < mb_num; i++){
      fprintf(fd, "--------------- block : %d -----------------\n", i);
      fprintf(fd, "-------------------- Y ---------------------\n");
      for(j = 0; j < 16; j++){
        fprintf(fd, "%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n", crc_y[0], crc_y[1], crc_y[2], crc_y[3], crc_y[4], crc_y[5], crc_y[6], crc_y[7], crc_y[8], crc_y[9], crc_y[10], crc_y[11], crc_y[12], crc_y[13], crc_y[14], crc_y[15]);
        crc_y += 16;
      }
      fprintf(fd, "-------------------- C ---------------------\n");
      for(j = 0; j < 8; j++){
        fprintf(fd, "%2x %2x %2x %2x %2x %2x %2x %2x ** %2x %2x %2x %2x %2x %2x %2x %2x\n", crc_uv[0], crc_uv[1], crc_uv[2], crc_uv[3], crc_uv[4], crc_uv[5], crc_uv[6], crc_uv[7], crc_uv[8], crc_uv[9], crc_uv[10], crc_uv[11], crc_uv[12], crc_uv[13], crc_uv[14], crc_uv[15]);
        crc_uv += 16;
      }
    }
    fprintf(fd, "\n");

    fclose(fd);
}
#else
static void prin_crc(const char * src_y, const char * src_u, const char * src_v, int mb_num, int slice_num)
{
    unsigned char * crc_y = src_y;
    unsigned char * crc_u = src_u;
    unsigned char * crc_v = src_v;
    int i, j;
    FILE * fd = fopen("Ama_src.c", "a");
    
    fprintf(fd, "---------------------- slice : %d ----------------------\n", slice_num);
    for(i = 0; i < mb_num; i++){
      fprintf(fd, "--------------- block : %d -----------------\n", i);
      fprintf(fd, "-------------------- Y ---------------------\n");
      for(j = 0; j < 16; j++){
        fprintf(fd, "%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n", crc_y[0], crc_y[1], crc_y[2], crc_y[3], crc_y[4], crc_y[5], crc_y[6], crc_y[7], crc_y[8], crc_y[9], crc_y[10], crc_y[11], crc_y[12], crc_y[13], crc_y[14], crc_y[15]);
        crc_y += 16;
      }
      fprintf(fd, "-------------------- C ---------------------\n");
      for(j = 0; j < 8; j++){
        fprintf(fd, "%2x %2x %2x %2x %2x %2x %2x %2x ** %2x %2x %2x %2x %2x %2x %2x %2x\n", crc_u[0], crc_u[1], crc_u[2], crc_u[3], crc_u[4], crc_u[5], crc_u[6], crc_u[7], crc_v[0], crc_v[1], crc_v[2], crc_v[3], crc_v[4], crc_v[5], crc_v[6], crc_v[7]);
        crc_u += 8;
        crc_v += 8;
      }
    }
    fprintf(fd, "\n");

    fclose(fd);
}
#endif

#ifdef MPEG2_SCH_CONTROL
extern int save_copied;

void print_slice_info(_M2D_SliceInfo * s, int * data)
{
    _M2D_BitStream *bs= &s->bs_buf;
    FILE * fd = fopen("data.c", "a");
    volatile int * chn = vdma_base;
    int i, j;
 
    fprintf(fd, "------------------ slice ----------------------\n");
    /* slice info */
    fprintf(fd, "%08x %08x %08x %08x\n", s->mb_width, s->mb_height, s->mb_pos_x, s->mb_pos_y);
    /* vlc table */
    for(i = 0; i < 4; i++){
      fprintf(fd, "%08x %08x %08x %08x %08x %08x %08x %08x\n", s->coef_qt[i][0], s->coef_qt[i][1], s->coef_qt[i][2], s->coef_qt[i][3], s->coef_qt[i][4], s->coef_qt[i][5], s->coef_qt[i][6], s->coef_qt[i][7]);
      fprintf(fd, "%08x %08x %08x %08x %08x %08x %08x %08x\n", s->coef_qt[i][8], s->coef_qt[i][9], s->coef_qt[i][10], s->coef_qt[i][11], s->coef_qt[i][12], s->coef_qt[i][13], s->coef_qt[i][14], s->coef_qt[i][15]);
    }
    /* scan table */
    fprintf(fd, "%08x %08x %08x %08x %08x %08x %08x %08x\n", s->scan[0], s->scan[1], s->scan[2], s->scan[3], s->scan[4], s->scan[5], s->scan[6], s->scan[7]);
    fprintf(fd, "%08x %08x %08x %08x %08x %08x %08x %08x\n", s->scan[8], s->scan[9], s->scan[10], s->scan[11], s->scan[12], s->scan[13], s->scan[14], s->scan[15]);
    /* first mb info */
    fprintf(fd, "%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", s->coding_type, s->fr_pred_fr_dct, s->pic_type, s->conceal_mv, s->intra_dc_pre, s->intra_vlc_format, s->mpeg1, s->top_fi_first, s->q_scale_type, s->qs_code, s->sec_fld);
    /* f_code */
    fprintf(fd, "f_code : %08x %08x %08x %08x\n\n", s->f_code[0][0], s->f_code[0][1], s->f_code[1][0], s->f_code[1][1]);
    /* bs info */
    fprintf(fd, "bs->buffer = %08x, bs->bit_ofst = %08x\n", bs->buffer, bs->bit_ofst);
    fprintf(fd, "--------------- bs data --------------\n");
    int * copy_data = data;
    for(i = 0; i < (save_copied / 4); i+=8)
      fprintf(fd, "0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", copy_data[i], copy_data[i + 1], copy_data[i + 2], copy_data[i + 3], copy_data[i + 4], copy_data[i + 5], copy_data[i + 6], copy_data[i + 7]);
    fprintf(fd, "\n");
    
    for(i = 0; i < 0x100; i++){
      fprintf(fd, "(0x%08x, 0x%08x) (0x%08x, 0x%08x) (0x%08x, 0x%08x) (0x%08x, 0x%08x)\n", *chn++, *chn++, *chn++, *chn++, *chn++, *chn++, *chn++, *chn++);
      if(chn[1] == 0)
        break;
    }

    fclose(fd);
}
#endif

#endif
