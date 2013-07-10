#ifndef __JZM_VC1_API_H__
#define __JZM_VC1_API_H__

typedef struct JZM_VC1{
    int profile_advanced;      /* v->profile == PROFILE_ADVANCED */
    int pict_type;             /* v->s.pict_type. 1:FF_I_TYPE, 2:FF_P_TYPE, 3:FF_B_TYPE */
    int bi_type;               /* v->bi_type */
    int p_frame_skipped;       /* v->p_frame_skipped */
    int pq;                    /* v->pq */
    int pqindex;               /* v->pqindex */
    int altpq;                 /* v->altpq */
    int halfpq;                /* v->halfpq */
    int dquantfrm;             /* v->dquantfrm */
    int dqbilevel;             /* v->dqbilevel */
    int dqprofile;             /* v->dqprofile */
    int dqsbedge;              /* v->dqsbedge */
    int y_ac_table_index;      /* v->y_ac_table_index */
    int c_ac_table_index;      /* v->c_ac_table_index */
    int mvrange;               /* v->mvrange */
    int quarter_sample;        /* v->s.quarter_sample */
    int ttmbf;                 /* v->ttmbf */
    int ttfrm;                 /* v->ttfrm */
    int dmb_is_raw;            /* v->dmb_is_raw */
    int mv_type_is_raw;        /* v->mv_type_is_raw */
    int skip_is_raw;           /* v->skip_is_raw */
    int acpred_is_raw;         /* v->acpred_is_raw */
    int overflg_is_raw;        /* v->overflg_is_raw */
    int condover;              /* v->condover */
    int overlap;               /* v->overlap */
    int bfraction;             /* v->bfraction */
    int fastuvmc;              /* v->fastuvmc */
    int mb_width;              /* v->s.mb_width */
    int mb_height;             /* v->s.mb_height */
    int mb_stride;             /* v->s.mb_stride */
    int linesize;              /* v->s.linesize */
    int mv_table_index;        /* v->s.mv_table_index */
    int dc_table_index;        /* v->s.dc_table_index */
    int tt_index;              /* v->tt_index */
    int rangeredfrm;           /* v->rangeredfrm */
    int res_fasttx;            /* v->res_fasttx */
    int res_x8;                /* v->res_x8 */
    int pquantizer;            /* v->pquantizer */
    int mv_mode_its;           /* v->mv_mode == MV_PMODE_INTENSITY_COMP */
    int mspel;                 /* v->mspel */
    int lumscale;              /* v->lumscale */
    int lumshift;              /* v->lumshift */
    int frm_edge;              /* */
    int frm_codingset;         /* */
    int frm_codingset2;        /* */
    int cbpcy_table_index;     /* */
    unsigned int bs_buffer;    /* */
    unsigned int bs_index;     /* */
    unsigned int frm_mv_addr;  /* only effect on b frame */
    unsigned int frm_plane[1<<10];         /* 4K byte (2560x1440) */
    unsigned int frm_no_dir_mv;            /* s->next_picture.sde_frm_no_mv */
    unsigned int curr_picture_data_y;      /* s->curr_picture.data[4] */
    unsigned int curr_picture_data_uv;     /* s->curr_picture.data[5] */
    unsigned int last_picture_data_y;      /* s->last_picture.data[4] */
    unsigned int last_picture_data_uv;     /* s->last_picture.data[5] */
    unsigned int next_picture_data_y;      /* s->next_picture.data[4] */
    unsigned int next_picture_data_uv;     /* s->next_picture.data[5] */
    int update_its_scale;    /* */
    int use_ic;
    unsigned char  its_scale;
    unsigned short its_rnd_y;
    unsigned short  its_rnd_c;
    int * des_va, * des_pa;
} JzmVc1_t;

#endif // __JZM_VC1_API_H__
