/*****************************************************************************
 *
 * JZC RV9 Decoder Architecture
 *
 ****************************************************************************/

#ifndef __JZC_RV9_ARCH_H__
#define __JZC_RV9_ARCH_H__

typedef struct RV9_Slice_GlbARGs{
    uint8_t pict_type;
    uint8_t si_type;
    uint8_t mb_width;
    uint8_t mb_height;

    uint8_t *last_data[3];
    uint8_t *next_data[3];
    uint8_t refqp; 
    uint16_t linesize;
    uint16_t uvlinesize;
}RV9_Slice_GlbARGs;

typedef struct RV9_MB_DecARGs{
    int8_t new_slice;
    int8_t  er_slice;
    uint8_t mb_x;
    uint8_t mb_y; 

    // MC
    int8_t mbtype;  // this use RV40BlockTypes
    int next_bt;

    // VMAU
    int cbp;
    uint8_t qscale;
    uint8_t mau_na; //neighbor avail
    uint16_t mau_mcbp;//main cbp high 8 bits
    uint16_t mau_mlen;//main data len
    uint32_t mau_ypm[2];//y_pred_mode
    uint32_t mau_cpm;//c_pred_mode

    // DBLK
    int8_t mbtype_cur;
    int8_t mbtype_above;

    int cur_cbp; // current MB coef indicator
    int up_cbp; // neighbour MB coef indicator
    int cur_coef;
    int up_coef;

    // other test used, take and maybe used in future
    uint32_t  is16;
    int cost[1];  // used for i_pref addr 32 bytes

    short motion_val[2][4][2]; 
    DCTELEM block16[16];
    DCTELEM block[6][64]; // 144+192=336 words
}RV9_MB_DecARGs;

#define SLICE_T_CC_LINE ((sizeof(struct RV9_Slice_GlbARGs)+31)/32)//=58
#endif
