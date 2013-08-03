/*
 * slice.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 2003      Peter Gubanov <peter@elecard.net.ru>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Modified for use with MPlayer, see libmpeg2_changes.diff for the exact changes.
 * detailed changelog at http://svn.mplayerhq.hu/mplayer/trunk/
 * $Id: slice.c,v 1.2 2012/09/10 03:49:03 kznan Exp $
 */

#include "config.h"
#include <stdio.h>

#include <inttypes.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

#include "mpeg2_config.h"
#include "../libjzcommon/jzasm.h"
#ifdef JZC_MXU_OPT
#include "../libjzcommon/jzmedia.h"
#define i_noreorder  ({  __asm__ __volatile__(".set noreorder"); })
#define i_reorder  ({  __asm__ __volatile__(".set reorder"); })
uint8_t idct_row_max,idct_row_max_intra;
#include "mpeg2_idct_mxu.c"
#endif

//#define JZM_DEBUG_MPEG2
#ifdef JZM_DEBUG_MPEG2
#include "../libjzcommon/crc.c"
#undef printf
#endif

//#include "api_data0.h"

extern mpeg2_mc_t mpeg2_mc;
extern void (* mpeg2_idct_copy) (int16_t * block, uint8_t * dest, int stride);
extern void (* mpeg2_idct_add) (int last, int16_t * block,
				uint8_t * dest, int stride);
extern void (* mpeg2_cpu_state_save) (cpu_state_t * state);
extern void (* mpeg2_cpu_state_restore) (cpu_state_t * state);

#include "vlc.h"

static inline int get_macroblock_modes (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int macroblock_modes;
    const MBtab * tab;

    switch (decoder->coding_type) {
    case I_TYPE:

	tab = MB_I + UBITS (bit_buf, 1);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if ((! (decoder->frame_pred_frame_dct)) &&
	    (decoder->picture_structure == FRAME_PICTURE)) {
	    macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
	    DUMPBITS (bit_buf, bits, 1);
	}

	return macroblock_modes;

    case P_TYPE:

	tab = MB_P + UBITS (bit_buf, 5);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if (decoder->picture_structure != FRAME_PICTURE) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
	} else if (decoder->frame_pred_frame_dct) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
		macroblock_modes |= MC_FRAME << MOTION_TYPE_SHIFT;
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
	} else {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
		macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
		DUMPBITS (bit_buf, bits, 1);
	    }
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
	}

    case B_TYPE:

	tab = MB_B + UBITS (bit_buf, 6);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if (decoder->picture_structure != FRAME_PICTURE) {
	    if (! (macroblock_modes & MACROBLOCK_INTRA)) {
		macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    return macroblock_modes;
	} else if (decoder->frame_pred_frame_dct) {
	      /* if (! (macroblock_modes & MACROBLOCK_INTRA)) */
	    macroblock_modes |= MC_FRAME << MOTION_TYPE_SHIFT;
	    return macroblock_modes;
	} else {
	    if (macroblock_modes & MACROBLOCK_INTRA)
		goto intra;
	    macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
	    DUMPBITS (bit_buf, bits, 2);
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
	    intra:
		macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
		DUMPBITS (bit_buf, bits, 1);
	    }
	    return macroblock_modes;
	}

    case D_TYPE:

	DUMPBITS (bit_buf, bits, 1);
	return MACROBLOCK_INTRA;

    default:
	return 0;
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

#ifdef MPEG2_SCH_CONTROL
static int save_qs_code;
#endif

static inline void get_quantizer_scale (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    int quantizer_scale_code;

    quantizer_scale_code = UBITS (bit_buf, 5);
#ifdef MPEG2_SCH_CONTROL
    save_qs_code = quantizer_scale_code;
#endif
    DUMPBITS (bit_buf, bits, 5);
    decoder->quantizer_scale = decoder->quantizer_scales[quantizer_scale_code];

    decoder->quantizer_matrix[0] =
	decoder->quantizer_prescale[0][quantizer_scale_code];
    decoder->quantizer_matrix[1] =
	decoder->quantizer_prescale[1][quantizer_scale_code];
    decoder->quantizer_matrix[2] =
	decoder->chroma_quantizer[0][quantizer_scale_code];
    decoder->quantizer_matrix[3] =
	decoder->chroma_quantizer[1][quantizer_scale_code];
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_motion_delta (mpeg2_decoder_t * const decoder,
				    const int f_code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    int delta;
    int sign;
    const MVtab * tab;

    if (bit_buf & 0x80000000) {
	DUMPBITS (bit_buf, bits, 1);
	return 0;
    } else if (bit_buf >= 0x0c000000) {

	tab = MV_4 + UBITS (bit_buf, 4);
	delta = (tab->delta << f_code) + 1;
	bits += tab->len + f_code + 1;
	bit_buf <<= tab->len;

	sign = SBITS (bit_buf, 1);
	bit_buf <<= 1;

	if (f_code)
	    delta += UBITS (bit_buf, f_code);
	bit_buf <<= f_code;

	return (delta ^ sign) - sign;

    } else {

	tab = MV_10 + UBITS (bit_buf, 10);
	delta = (tab->delta << f_code) + 1;
	bits += tab->len + 1;
	bit_buf <<= tab->len;

	sign = SBITS (bit_buf, 1);
	bit_buf <<= 1;

	if (f_code) {
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    delta += UBITS (bit_buf, f_code);
	    DUMPBITS (bit_buf, bits, f_code);
	}

	return (delta ^ sign) - sign;

    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int bound_motion_vector (const int vector, const int f_code)
{
    return ((int32_t)vector << (27 - f_code)) >> (27 - f_code);
}

static inline int get_dmv (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const DMVtab * tab;

    tab = DMV_2 + UBITS (bit_buf, 2);
    DUMPBITS (bit_buf, bits, tab->len);
    return tab->dmv;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_coded_block_pattern (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const CBPtab * tab;

    NEEDBITS (bit_buf, bits, bit_ptr);

    if (bit_buf >= 0x20000000) {

	tab = CBP_7 + (UBITS (bit_buf, 7) - 16);
	DUMPBITS (bit_buf, bits, tab->len);
	return tab->cbp;

    } else {

	tab = CBP_9 + UBITS (bit_buf, 9);
	DUMPBITS (bit_buf, bits, tab->len);
	return tab->cbp;
    }

#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_luma_dc_dct_diff (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000) {
	tab = DC_lum_5 + UBITS (bit_buf, 5);
	size = tab->size;
	if (size) {
	    bits += tab->len + size;
	    bit_buf <<= tab->len;
	    dc_diff =
		UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	    bit_buf <<= size;
#ifdef JZC_MXU_OPT
            return dc_diff;
#else
	    return dc_diff << decoder->intra_dc_precision;
#endif
	} else {
	    DUMPBITS (bit_buf, bits, 3);
	    return 0;
	}
    } else {
	tab = DC_long + (UBITS (bit_buf, 9) - 0x1e0);
	size = tab->size;
	DUMPBITS (bit_buf, bits, tab->len);
	NEEDBITS (bit_buf, bits, bit_ptr);
	dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	DUMPBITS (bit_buf, bits, size);
#ifdef JZC_MXU_OPT
        return dc_diff;
#else
	return dc_diff << decoder->intra_dc_precision;
#endif
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_chroma_dc_dct_diff (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000) {
	tab = DC_chrom_5 + UBITS (bit_buf, 5);
	size = tab->size;
	if (size) {
	    bits += tab->len + size;
	    bit_buf <<= tab->len;
	    dc_diff =
		UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	    bit_buf <<= size;
#ifdef JZC_MXU_OPT
            return dc_diff;
#else
	    return dc_diff << decoder->intra_dc_precision;
#endif
	} else {
	    DUMPBITS (bit_buf, bits, 2);
	    return 0;
	}
    } else {
	tab = DC_long + (UBITS (bit_buf, 10) - 0x3e0);
	size = tab->size;
	DUMPBITS (bit_buf, bits, tab->len + 1);
	NEEDBITS (bit_buf, bits, bit_ptr);
	dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	DUMPBITS (bit_buf, bits, size);
#ifdef JZC_MXU_OPT
        return dc_diff;
#else
	return dc_diff << decoder->intra_dc_precision;
#endif
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

#ifdef JZC_MXU_OPT
#define SATURATE(val)                           \
    do {					\
	val <<= 4;				\
	if (unlikely (val != (int16_t) val))	\
	    val = (SBITS (val, 1) ^ 2047) << 4;	\
	val >>= 4;				\
    } while (0)
#else
#define SATURATE(val)				\
    do {					\
	val <<= 4;				\
	if (unlikely (val != (int16_t) val))	\
	    val = (SBITS (val, 1) ^ 2047) << 4;	\
    } while (0)
#endif

#ifdef JZC_MXU_OPT
static void get_intra_block_B14 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    S32CPS(xr11, xr0, xr0);   
    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
        int32_t my_run, my_len, my_lvl, tmp;
	if (bit_buf >= 0x28000000) {
	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level; 
	    i += my_run;
	    i_reorder;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    i_noreorder;
	    j = scan[i];
	    bit_buf <<= my_len;
	    bits += my_len + 1;
	    val = quant_matrix[j];
            S32I2M(xr10, j);
	    tmp = my_lvl;
            val *= tmp;
            S32MAX(xr11, xr11, xr10);
            tmp = SBITS (bit_buf, 1);
            val >>= 4;
            val = (val ^ tmp) - tmp;
            i_reorder;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    continue;

	} else if (bit_buf >= 0x04000000) {
	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

            i_noreorder;
	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
            tmp = quant_matrix[j];
            S32I2M(xr10, j);
            val = SBITS (bit_buf, 12);
            val *= tmp;
            S32MAX(xr11, xr11, xr10);
            val >>= 4;
            i_reorder; 

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder; 
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    val = S32M2I(xr11);
    dest[63] ^= mismatch & 1;
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    idct_row_max_intra = (val>>3) + 1;
}
#else
static void get_intra_block_B14 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;
   
    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = (tab->level * quant_matrix[j]) >> 4;

	      /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = (SBITS (bit_buf, 12) * quant_matrix[j]) / 16;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 16;
    DUMPBITS (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}
#endif

#ifdef JZC_MXU_OPT
static void get_intra_block_B15 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    S32CPS(xr11, xr0, xr0);
    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
        int32_t my_run, my_len, my_lvl, tmp;
	if (bit_buf >= 0x04000000) {
	    tab = DCT_B15_8 + (UBITS (bit_buf, 8) - 4);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
	    i += my_run;
            i_reorder;
	    if (i < 64) {

	    normal_code:
	        i_noreorder;
		j = scan[i];
		bit_buf <<= my_len;
		bits += my_len + 1;
                val = quant_matrix[j];
                S32I2M(xr10, j);
                tmp =  my_lvl;
                val *= tmp;
                S32MAX(xr11, xr11, xr10);
                tmp = SBITS (bit_buf, 1);
                val >>= 4;
                val = (val ^ tmp) - tmp;
                i_reorder;

		SATURATE (val);
		dest[j] = val;
		mismatch ^= val;

		bit_buf <<= 1;
		NEEDBITS (bit_buf, bits, bit_ptr);
		continue;

	    } else {

		  /* end of block. I commented out this code because if we */
		  /* do not exit here we will still exit at the later test :) */

		  /* if (i >= 128) break;	*/	/* end of block */

		  /* escape code */

		i += UBITS (bit_buf << 6, 6) - 64;
		if (i >= 64)
		    break;	/* illegal, check against buffer overflow */

		i_noreorder;
		j = scan[i];

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);
                tmp = quant_matrix[j];
                S32I2M(xr10, j);
                val = SBITS (bit_buf, 12); 
                val *= tmp;
                S32MAX(xr11, xr11, xr10);
                val >>= 4;
                i_reorder;         
      
		SATURATE (val);
		dest[j] = val;
		mismatch ^= val;

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);
		continue;

	    }
	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B15_10 + (UBITS (bit_buf, 10) - 8);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
	    i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder; 
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 1;
    DUMPBITS (bit_buf, bits, 4);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    D32SAR(xr11,xr11,xr0,xr0,3);
    idct_row_max_intra = S32M2I(xr11)+1;
}
#else
static void get_intra_block_B15 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B15_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64) {

	    normal_code:
		j = scan[i];
		bit_buf <<= tab->len;
		bits += tab->len + 1;
		val = (tab->level * quant_matrix[j]) >> 4;

		  /* if (bitstream_get (1)) val = -val; */
		val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

		SATURATE (val);
		dest[j] = val;
		mismatch ^= val;

		bit_buf <<= 1;
		NEEDBITS (bit_buf, bits, bit_ptr);

		continue;

	    } else {

		  /* end of block. I commented out this code because if we */
		  /* do not exit here we will still exit at the later test :) */

		  /* if (i >= 128) break;	*/	/* end of block */

		  /* escape code */

		i += UBITS (bit_buf << 6, 6) - 64;
		if (i >= 64)
		    break;	/* illegal, check against buffer overflow */

		j = scan[i];

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);
		val = (SBITS (bit_buf, 12) * quant_matrix[j]) / 16;

		SATURATE (val);
		dest[j] = val;
		mismatch ^= val;

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);

		continue;

	    }
	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B15_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 16;
    DUMPBITS (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}
#endif

#ifdef JZC_MXU_OPT
static int get_non_intra_block (mpeg2_decoder_t * const decoder,
				const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    S32CPS(xr11, xr0, xr0);
    i = -1;
    mismatch = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
        int32_t my_run, my_len, my_lvl, tmp;
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    i_noreorder;
	    j = scan[i];
	    bit_buf <<= my_len;
	    bits += my_len + 1;
	    val = quant_matrix[j];
	    S32I2M(xr10, j);
	    tmp = (my_lvl<<1) + 1;
	    val *= tmp;
	    S32MAX(xr11, xr11, xr10);
	    tmp = SBITS (bit_buf, 1);
	    val >>= 5;
	    val = (val ^ tmp) - tmp;
	    i_reorder;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;
	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    i_noreorder;
	    j = scan[i];
	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    tmp = quant_matrix[j];
	    S32I2M(xr10, j);
	    val = SBITS (bit_buf, 12) + SBITS (bit_buf, 1);	    
	    val = (val<<1) + 1;
	    val *= tmp;
	    val >>= 5;
	    S32MAX(xr11, xr11, xr10);
	    i_reorder;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
            i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    val = S32M2I(xr11);
    dest[63] ^= mismatch & 1;
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    idct_row_max = (val>>3)+1;
    return i;
}
#else
static int get_non_intra_block (mpeg2_decoder_t * const decoder,
				const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    i = -1;
    mismatch = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = ((2 * tab->level + 1) * quant_matrix[j]) >> 5;

	      /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = 2 * (SBITS (bit_buf, 12) + SBITS (bit_buf, 1)) + 1;
	    val = (val * quant_matrix[j]) / 32;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 16;
    DUMPBITS (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    return i;
}
#endif

#ifdef JZC_MXU_OPT
static void get_mpeg1_intra_block (mpeg2_decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const uint16_t * const quant_matrix = decoder->quantizer_matrix[0];
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    S32CPS(xr11, xr0, xr0);
    i = 0;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
        int32_t my_run, my_len, my_lvl, tmp;
	if (bit_buf >= 0x28000000) {
	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level; 
	    i += my_run;
	    i_reorder;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    i_noreorder;
	    j = scan[i];
	    bit_buf <<= my_len;
	    bits += my_len + 1;
	    val = quant_matrix[j];
            S32I2M(xr10, j);
	    tmp = my_lvl;
            val *= tmp;
            val = (val - 1) | 1;
            S32MAX(xr11, xr11, xr10);
            tmp = SBITS (bit_buf, 1);
            val >>= 4;
            val = (val ^ tmp) - tmp;
            i_reorder;

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {
	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    i_noreorder;
	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
            tmp = quant_matrix[j];
            S32I2M(xr10, j);

	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	      //val = (val * quant_matrix[j]) / 16;
            val *= tmp;
	    S32MAX(xr11, xr11, xr10);
            val >>= 4;
	      /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;
	    i_reorder;

	    SATURATE (val);
	    dest[j] = val;

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
            i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder; 
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i_noreorder;
            my_run = (uint32_t)tab->run;
            my_len = (uint32_t)tab->len;
            my_lvl = (uint32_t)tab->level;
            i += my_run;
            i_reorder;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    val = S32M2I(xr11);
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    idct_row_max_intra = (val>>3) + 1;;
}
#else
static void get_mpeg1_intra_block (mpeg2_decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const uint16_t * const quant_matrix = decoder->quantizer_matrix[0];
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    i = 0;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = (tab->level * quant_matrix[j]) >> 4;

	      /* oddification */
	    val = (val - 1) | 1;

	      /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	    val = (val * quant_matrix[j]) / 16;

	      /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;

	    SATURATE (val);
	    dest[j] = val;

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}
#endif

#ifdef JZC_MXU_OPT
static int get_mpeg1_non_intra_block (mpeg2_decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const uint16_t * const quant_matrix = decoder->quantizer_matrix[1];
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;
  
    S32CPS(xr11, xr0, xr0);
    i = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
        int32_t my_run, my_len, my_lvl, tmp;
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    i_noreorder;
	    j = scan[i];
	    bit_buf <<= my_len;
	    bits += my_len + 1;
	    val = quant_matrix[j];
	    S32I2M(xr10, j);
	    tmp = (my_lvl<<1) + 1;
	    val *= tmp;
	    val = (val - 1) | 1;
	    S32MAX(xr11, xr11, xr10);
	    tmp = SBITS (bit_buf, 1);
	    val >>= 5;
	    val = (val ^ tmp) - tmp;
	    i_reorder;

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    i_noreorder; 
	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    tmp = quant_matrix[j];
	    S32I2M(xr10, j);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	    val = 2 * (val + SBITS (val, 1)) + 1;
	    val *= tmp;
	    val >>= 5;

	      /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;
	    S32MAX(xr11, xr11, xr10);
	    i_reorder;

	    SATURATE (val);
	    dest[j] = val;

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
            i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i_noreorder;
	    my_run = (uint32_t)tab->run;
	    my_len = (uint32_t)tab->len;
	    my_lvl = (uint32_t)tab->level;
	    i += my_run;
	    i_reorder;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    val = S32M2I(xr11);
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    idct_row_max = (val>>3)+1;
    return i;
}
#else
static int get_mpeg1_non_intra_block (mpeg2_decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const uint16_t * const quant_matrix = decoder->quantizer_matrix[1];
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    i = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = ((2 * tab->level + 1) * quant_matrix[j]) >> 5;

	      /* oddification */
	    val = (val - 1) | 1;

	      /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	      /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	    val = 2 * (val + SBITS (val, 1)) + 1;
	    val = (val * quant_matrix[j]) / 32;

	      /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;

	    SATURATE (val);
	    dest[j] = val;

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    return i;
}
#endif

static inline void slice_intra_DCT (mpeg2_decoder_t * const decoder,
				    const int cc,
				    uint8_t * const dest, const int stride)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    NEEDBITS (bit_buf, bits, bit_ptr);
      /* Get the intra DC coefficient and inverse quantize it */
#ifdef JZC_MXU_OPT
    if (cc == 0)
	decoder->dc_dct_pred[0] += get_luma_dc_dct_diff (decoder);
    else
        decoder->dc_dct_pred[cc] += get_chroma_dc_dct_diff (decoder);
    decoder->DCTblock[0] =
        decoder->dc_dct_pred[cc] << (3 -decoder->intra_dc_precision);     
#else
    if (cc == 0)
	decoder->DCTblock[0] =
	    decoder->dc_dct_pred[0] += get_luma_dc_dct_diff (decoder);
    else
	decoder->DCTblock[0] =
	    decoder->dc_dct_pred[cc] += get_chroma_dc_dct_diff (decoder);
#endif
    if (decoder->mpeg1) {
	if (decoder->coding_type != D_TYPE)
	    get_mpeg1_intra_block (decoder);
    } else if (decoder->intra_vlc_format)
	get_intra_block_B15 (decoder, decoder->quantizer_matrix[cc ? 2 : 0]);
    else
	get_intra_block_B14 (decoder, decoder->quantizer_matrix[cc ? 2 : 0]);
#ifdef JZC_MXU_OPT
    mpeg2_idct_copy_mxu(decoder->DCTblock, dest, stride, idct_row_max_intra);
#else
    mpeg2_idct_copy (decoder->DCTblock, dest, stride);
#endif

#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline void slice_non_intra_DCT (mpeg2_decoder_t * const decoder,
					const int cc,
					uint8_t * const dest, const int stride)
{
    int last;

    if (decoder->mpeg1)
	last = get_mpeg1_non_intra_block (decoder);
    else
	last = get_non_intra_block (decoder,
				    decoder->quantizer_matrix[cc ? 3 : 1]);
#ifdef JZC_MXU_OPT
    mpeg2_idct_add_mxu (decoder->DCTblock, dest, stride,idct_row_max);
#else
    mpeg2_idct_add (last, decoder->DCTblock, dest, stride);
#endif
}

#define MOTION_420(table,ref,motion_x,motion_y,size,y)			\
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	\
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    table[xy_half] (decoder->dest[0] + y * decoder->stride + decoder->offset, \
		    ref[0] + (pos_x >> 1) + (pos_y >> 1) * decoder->stride, \
		    decoder->stride, size);				\
    motion_x /= 2;	motion_y /= 2;					\
    xy_half = ((motion_y & 1) << 1) | (motion_x & 1);			\
    offset = (((decoder->offset + motion_x) >> 1) +			\
	      ((((decoder->v_offset + motion_y) >> 1) + y/2) *		\
	       decoder->uv_stride));					\
    table[4+xy_half] (decoder->dest[1] + y/2 * decoder->uv_stride +	\
		      (decoder->offset >> 1), ref[1] + offset,		\
		      decoder->uv_stride, size/2);			\
    table[4+xy_half] (decoder->dest[2] + y/2 * decoder->uv_stride +	\
		      (decoder->offset >> 1), ref[2] + offset,		\
		      decoder->uv_stride, size/2)

#define MOTION_FIELD_420(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = decoder->v_offset + motion_y;				\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y)) {				\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		\
	motion_y = pos_y - decoder->v_offset;				\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    table[xy_half] (decoder->dest[0] + dest_field * decoder->stride +	\
		    decoder->offset,					\
		    (ref[0] + (pos_x >> 1) +				\
		     ((pos_y op) + src_field) * decoder->stride),	\
		    2 * decoder->stride, 8);				\
    motion_x /= 2;	motion_y /= 2;					\
    xy_half = ((motion_y & 1) << 1) | (motion_x & 1);			\
    offset = (((decoder->offset + motion_x) >> 1) +			\
	      (((decoder->v_offset >> 1) + (motion_y op) + src_field) *	\
	       decoder->uv_stride));					\
    table[4+xy_half] (decoder->dest[1] + dest_field * decoder->uv_stride + \
		      (decoder->offset >> 1), ref[1] + offset,		\
		      2 * decoder->uv_stride, 4);			\
    table[4+xy_half] (decoder->dest[2] + dest_field * decoder->uv_stride + \
		      (decoder->offset >> 1), ref[2] + offset,		\
		      2 * decoder->uv_stride, 4)

#define MOTION_DMV_420(table,ref,motion_x,motion_y)			\
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = decoder->v_offset + motion_y;				\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y)) {				\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		\
	motion_y = pos_y - decoder->v_offset;				\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + (pos_y & ~1) * decoder->stride;		\
    table[xy_half] (decoder->dest[0] + decoder->offset,			\
		    ref[0] + offset, 2 * decoder->stride, 8);		\
    table[xy_half] (decoder->dest[0] + decoder->stride + decoder->offset, \
		    ref[0] + decoder->stride + offset,			\
		    2 * decoder->stride, 8);				\
    motion_x /= 2;	motion_y /= 2;					\
    xy_half = ((motion_y & 1) << 1) | (motion_x & 1);			\
    offset = (((decoder->offset + motion_x) >> 1) +			\
	      (((decoder->v_offset >> 1) + (motion_y & ~1)) *		\
	       decoder->uv_stride));					\
    table[4+xy_half] (decoder->dest[1] + (decoder->offset >> 1),	\
		      ref[1] + offset, 2 * decoder->uv_stride, 4);	\
    table[4+xy_half] (decoder->dest[1] + decoder->uv_stride +		\
		      (decoder->offset >> 1),				\
		      ref[1] + decoder->uv_stride + offset,		\
		      2 * decoder->uv_stride, 4);			\
    table[4+xy_half] (decoder->dest[2] + (decoder->offset >> 1),	\
		      ref[2] + offset, 2 * decoder->uv_stride, 4);	\
    table[4+xy_half] (decoder->dest[2] + decoder->uv_stride +		\
		      (decoder->offset >> 1),				\
		      ref[2] + decoder->uv_stride + offset,		\
		      2 * decoder->uv_stride, 4)

#define MOTION_ZERO_420(table,ref)					\
    table[0] (decoder->dest[0] + decoder->offset,			\
	      (ref[0] + decoder->offset +				\
	       decoder->v_offset * decoder->stride), decoder->stride, 16); \
    offset = ((decoder->offset >> 1) +					\
	      (decoder->v_offset >> 1) * decoder->uv_stride);		\
    table[4] (decoder->dest[1] + (decoder->offset >> 1),		\
	      ref[1] + offset, decoder->uv_stride, 8);			\
    table[4] (decoder->dest[2] + (decoder->offset >> 1),		\
	      ref[2] + offset, decoder->uv_stride, 8)

#define MOTION_422(table,ref,motion_x,motion_y,size,y)			\
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	\
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + (pos_y >> 1) * decoder->stride;		\
    table[xy_half] (decoder->dest[0] + y * decoder->stride + decoder->offset, \
		    ref[0] + offset, decoder->stride, size);		\
    offset = (offset + (motion_x & (motion_x < 0))) >> 1;		\
    motion_x /= 2;							\
    xy_half = ((pos_y & 1) << 1) | (motion_x & 1);			\
    table[4+xy_half] (decoder->dest[1] + y * decoder->uv_stride +	\
		      (decoder->offset >> 1), ref[1] + offset,		\
		      decoder->uv_stride, size);			\
    table[4+xy_half] (decoder->dest[2] + y * decoder->uv_stride +	\
		      (decoder->offset >> 1), ref[2] + offset,		\
		      decoder->uv_stride, size)

#define MOTION_FIELD_422(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = decoder->v_offset + motion_y;				\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y)) {				\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		\
	motion_y = pos_y - decoder->v_offset;				\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + ((pos_y op) + src_field) * decoder->stride;	\
    table[xy_half] (decoder->dest[0] + dest_field * decoder->stride +	\
		    decoder->offset, ref[0] + offset,			\
		    2 * decoder->stride, 8);				\
    offset = (offset + (motion_x & (motion_x < 0))) >> 1;		\
    motion_x /= 2;							\
    xy_half = ((pos_y & 1) << 1) | (motion_x & 1);			\
    table[4+xy_half] (decoder->dest[1] + dest_field * decoder->uv_stride + \
		      (decoder->offset >> 1), ref[1] + offset,		\
		      2 * decoder->uv_stride, 8);			\
    table[4+xy_half] (decoder->dest[2] + dest_field * decoder->uv_stride + \
		      (decoder->offset >> 1), ref[2] + offset,		\
		      2 * decoder->uv_stride, 8)

#define MOTION_DMV_422(table,ref,motion_x,motion_y)			\
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = decoder->v_offset + motion_y;				\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y)) {				\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		\
	motion_y = pos_y - decoder->v_offset;				\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + (pos_y & ~1) * decoder->stride;		\
    table[xy_half] (decoder->dest[0] + decoder->offset,			\
		    ref[0] + offset, 2 * decoder->stride, 8);		\
    table[xy_half] (decoder->dest[0] + decoder->stride + decoder->offset, \
		    ref[0] + decoder->stride + offset,			\
		    2 * decoder->stride, 8);				\
    offset = (offset + (motion_x & (motion_x < 0))) >> 1;		\
    motion_x /= 2;							\
    xy_half = ((pos_y & 1) << 1) | (motion_x & 1);			\
    table[4+xy_half] (decoder->dest[1] + (decoder->offset >> 1),	\
		      ref[1] + offset, 2 * decoder->uv_stride, 8);	\
    table[4+xy_half] (decoder->dest[1] + decoder->uv_stride +		\
		      (decoder->offset >> 1),				\
		      ref[1] + decoder->uv_stride + offset,		\
		      2 * decoder->uv_stride, 8);			\
    table[4+xy_half] (decoder->dest[2] + (decoder->offset >> 1),	\
		      ref[2] + offset, 2 * decoder->uv_stride, 8);	\
    table[4+xy_half] (decoder->dest[2] + decoder->uv_stride +		\
		      (decoder->offset >> 1),				\
		      ref[2] + decoder->uv_stride + offset,		\
		      2 * decoder->uv_stride, 8)

#define MOTION_ZERO_422(table,ref)					\
    offset = decoder->offset + decoder->v_offset * decoder->stride;	\
    table[0] (decoder->dest[0] + decoder->offset,			\
	      ref[0] + offset, decoder->stride, 16);			\
    offset >>= 1;							\
    table[4] (decoder->dest[1] + (decoder->offset >> 1),		\
	      ref[1] + offset, decoder->uv_stride, 16);			\
    table[4] (decoder->dest[2] + (decoder->offset >> 1),		\
	      ref[2] + offset, decoder->uv_stride, 16)

#define MOTION_444(table,ref,motion_x,motion_y,size,y)			\
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	\
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + (pos_y >> 1) * decoder->stride;		\
    table[xy_half] (decoder->dest[0] + y * decoder->stride + decoder->offset, \
		    ref[0] + offset, decoder->stride, size);		\
    table[xy_half] (decoder->dest[1] + y * decoder->stride + decoder->offset, \
		    ref[1] + offset, decoder->stride, size);		\
    table[xy_half] (decoder->dest[2] + y * decoder->stride + decoder->offset, \
		    ref[2] + offset, decoder->stride, size)

#define MOTION_FIELD_444(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = decoder->v_offset + motion_y;				\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y)) {				\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		\
	motion_y = pos_y - decoder->v_offset;				\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + ((pos_y op) + src_field) * decoder->stride;	\
    table[xy_half] (decoder->dest[0] + dest_field * decoder->stride +	\
		    decoder->offset, ref[0] + offset,			\
		    2 * decoder->stride, 8);				\
    table[xy_half] (decoder->dest[1] + dest_field * decoder->stride +	\
		    decoder->offset, ref[1] + offset,			\
		    2 * decoder->stride, 8);				\
    table[xy_half] (decoder->dest[2] + dest_field * decoder->stride +	\
		    decoder->offset, ref[2] + offset,			\
		    2 * decoder->stride, 8)

#define MOTION_DMV_444(table,ref,motion_x,motion_y)			\
    pos_x = 2 * decoder->offset + motion_x;				\
    pos_y = decoder->v_offset + motion_y;				\
    if (unlikely (pos_x > decoder->limit_x)) {				\
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		\
	motion_x = pos_x - 2 * decoder->offset;				\
    }									\
    if (unlikely (pos_y > decoder->limit_y)) {				\
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		\
	motion_y = pos_y - decoder->v_offset;				\
    }									\
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				\
    offset = (pos_x >> 1) + (pos_y & ~1) * decoder->stride;		\
    table[xy_half] (decoder->dest[0] + decoder->offset,			\
		    ref[0] + offset, 2 * decoder->stride, 8);		\
    table[xy_half] (decoder->dest[0] + decoder->stride + decoder->offset, \
		    ref[0] + decoder->stride + offset,			\
		    2 * decoder->stride, 8);				\
    table[xy_half] (decoder->dest[1] + decoder->offset,			\
		    ref[1] + offset, 2 * decoder->stride, 8);		\
    table[xy_half] (decoder->dest[1] + decoder->stride + decoder->offset, \
		    ref[1] + decoder->stride + offset,			\
		    2 * decoder->stride, 8);				\
    table[xy_half] (decoder->dest[2] + decoder->offset,			\
		    ref[2] + offset, 2 * decoder->stride, 8);		\
    table[xy_half] (decoder->dest[2] + decoder->stride + decoder->offset, \
		    ref[2] + decoder->stride + offset,			\
		    2 * decoder->stride, 8)

#define MOTION_ZERO_444(table,ref)					\
    offset = decoder->offset + decoder->v_offset * decoder->stride;	\
    table[0] (decoder->dest[0] + decoder->offset,			\
	      ref[0] + offset, decoder->stride, 16);			\
    table[4] (decoder->dest[1] + decoder->offset,			\
	      ref[1] + offset, decoder->stride, 16);			\
    table[4] (decoder->dest[2] + decoder->offset,			\
	      ref[2] + offset, decoder->stride, 16)

#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

static void motion_mp1 (mpeg2_decoder_t * const decoder,
			motion_t * const motion,
			mpeg2_mc_fct * const * const table)
{
    int motion_x, motion_y;
    unsigned int pos_x, pos_y, xy_half, offset;

    NEEDBITS (bit_buf, bits, bit_ptr);
    motion_x = (motion->pmv[0][0] +
		(get_motion_delta (decoder,
				   motion->f_code[0]) << motion->f_code[1]));
    motion_x = bound_motion_vector (motion_x,
				    motion->f_code[0] + motion->f_code[1]);
    motion->pmv[0][0] = motion_x;

    NEEDBITS (bit_buf, bits, bit_ptr);
    motion_y = (motion->pmv[0][1] +
		(get_motion_delta (decoder,
				   motion->f_code[0]) << motion->f_code[1]));
    motion_y = bound_motion_vector (motion_y,
				    motion->f_code[0] + motion->f_code[1]);
    motion->pmv[0][1] = motion_y;

    MOTION_420 (table, motion->ref[0], motion_x, motion_y, 16, 0);
}

#define MOTION_FUNCTIONS(FORMAT,MOTION,MOTION_FIELD,MOTION_DMV,MOTION_ZERO) \
									\
    static void motion_fr_frame_##FORMAT (mpeg2_decoder_t * const decoder, \
					  motion_t * const motion,	\
					  mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y;						\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_x = motion->pmv[0][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[1][0] = motion->pmv[0][0] = motion_x;		\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_y = motion->pmv[0][1] + get_motion_delta (decoder,	\
							 motion->f_code[1]); \
	motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	\
	motion->pmv[1][1] = motion->pmv[0][1] = motion_y;		\
									\
	MOTION (table, motion->ref[0], motion_x, motion_y, 16, 0);	\
    }									\
									\
    static void motion_fr_field_##FORMAT (mpeg2_decoder_t * const decoder, \
					  motion_t * const motion,	\
					  mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y, field;					\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	field = UBITS (bit_buf, 1);					\
	DUMPBITS (bit_buf, bits, 1);					\
									\
	motion_x = motion->pmv[0][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[0][0] = motion_x;					\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_y = ((motion->pmv[0][1] >> 1) +				\
		    get_motion_delta (decoder, motion->f_code[1]));	\
	/* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */ \
	motion->pmv[0][1] = motion_y << 1;				\
									\
	MOTION_FIELD (table, motion->ref[0], motion_x, motion_y, 0, & ~1, field); \
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	field = UBITS (bit_buf, 1);					\
	DUMPBITS (bit_buf, bits, 1);					\
									\
	motion_x = motion->pmv[1][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[1][0] = motion_x;					\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_y = ((motion->pmv[1][1] >> 1) +				\
		    get_motion_delta (decoder, motion->f_code[1]));	\
	/* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */ \
	motion->pmv[1][1] = motion_y << 1;				\
									\
	MOTION_FIELD (table, motion->ref[0], motion_x, motion_y, 1, & ~1, field); \
    }									\
									\
    static void motion_fr_dmv_##FORMAT (mpeg2_decoder_t * const decoder, \
					motion_t * const motion,	\
					mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y, dmv_x, dmv_y, m, other_x, other_y;	\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_x = motion->pmv[0][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[1][0] = motion->pmv[0][0] = motion_x;		\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	dmv_x = get_dmv (decoder);					\
									\
	motion_y = ((motion->pmv[0][1] >> 1) +				\
		    get_motion_delta (decoder, motion->f_code[1]));	\
	/* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */ \
	motion->pmv[1][1] = motion->pmv[0][1] = motion_y << 1;		\
	dmv_y = get_dmv (decoder);					\
									\
	m = decoder->top_field_first ? 1 : 3;				\
	other_x = ((motion_x * m + (motion_x > 0)) >> 1) + dmv_x;	\
	other_y = ((motion_y * m + (motion_y > 0)) >> 1) + dmv_y - 1;	\
	MOTION_FIELD (mpeg2_mc.put, motion->ref[0], other_x, other_y, 0, | 1, 0); \
									\
	m = decoder->top_field_first ? 3 : 1;				\
	other_x = ((motion_x * m + (motion_x > 0)) >> 1) + dmv_x;	\
	other_y = ((motion_y * m + (motion_y > 0)) >> 1) + dmv_y + 1;	\
	MOTION_FIELD (mpeg2_mc.put, motion->ref[0], other_x, other_y, 1, & ~1, 0); \
									\
	MOTION_DMV (mpeg2_mc.avg, motion->ref[0], motion_x, motion_y);	\
    }									\
									\
    static void motion_reuse_##FORMAT (mpeg2_decoder_t * const decoder,	\
				       motion_t * const motion,		\
				       mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y;						\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	motion_x = motion->pmv[0][0];					\
	motion_y = motion->pmv[0][1];					\
									\
	MOTION (table, motion->ref[0], motion_x, motion_y, 16, 0);	\
    }									\
									\
    static void motion_zero_##FORMAT (mpeg2_decoder_t * const decoder,	\
				      motion_t * const motion,		\
				      mpeg2_mc_fct * const * const table) \
    {									\
	unsigned int offset;						\
									\
	motion->pmv[0][0] = motion->pmv[0][1] = 0;			\
	motion->pmv[1][0] = motion->pmv[1][1] = 0;			\
									\
	MOTION_ZERO (table, motion->ref[0]);				\
    }									\
									\
    static void motion_fi_field_##FORMAT (mpeg2_decoder_t * const decoder, \
					  motion_t * const motion,	\
					  mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y;						\
	uint8_t ** ref_field;						\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	ref_field = motion->ref2[UBITS (bit_buf, 1)];			\
	DUMPBITS (bit_buf, bits, 1);					\
									\
	motion_x = motion->pmv[0][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[1][0] = motion->pmv[0][0] = motion_x;		\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_y = motion->pmv[0][1] + get_motion_delta (decoder,	\
							 motion->f_code[1]); \
	motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	\
	motion->pmv[1][1] = motion->pmv[0][1] = motion_y;		\
									\
	MOTION (table, ref_field, motion_x, motion_y, 16, 0);		\
    }									\
									\
    static void motion_fi_16x8_##FORMAT (mpeg2_decoder_t * const decoder, \
					 motion_t * const motion,	\
					 mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y;						\
	uint8_t ** ref_field;						\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	ref_field = motion->ref2[UBITS (bit_buf, 1)];			\
	DUMPBITS (bit_buf, bits, 1);					\
									\
	motion_x = motion->pmv[0][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[0][0] = motion_x;					\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_y = motion->pmv[0][1] + get_motion_delta (decoder,	\
							 motion->f_code[1]); \
	motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	\
	motion->pmv[0][1] = motion_y;					\
									\
	MOTION (table, ref_field, motion_x, motion_y, 8, 0);		\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	ref_field = motion->ref2[UBITS (bit_buf, 1)];			\
	DUMPBITS (bit_buf, bits, 1);					\
									\
	motion_x = motion->pmv[1][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[1][0] = motion_x;					\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_y = motion->pmv[1][1] + get_motion_delta (decoder,	\
							 motion->f_code[1]); \
	motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	\
	motion->pmv[1][1] = motion_y;					\
									\
	MOTION (table, ref_field, motion_x, motion_y, 8, 8);		\
    }									\
									\
    static void motion_fi_dmv_##FORMAT (mpeg2_decoder_t * const decoder, \
					motion_t * const motion,	\
					mpeg2_mc_fct * const * const table) \
    {									\
	int motion_x, motion_y, other_x, other_y;			\
	unsigned int pos_x, pos_y, xy_half, offset;			\
									\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	motion_x = motion->pmv[0][0] + get_motion_delta (decoder,	\
							 motion->f_code[0]); \
	motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	\
	motion->pmv[1][0] = motion->pmv[0][0] = motion_x;		\
	NEEDBITS (bit_buf, bits, bit_ptr);				\
	other_x = ((motion_x + (motion_x > 0)) >> 1) + get_dmv (decoder); \
									\
	motion_y = motion->pmv[0][1] + get_motion_delta (decoder,	\
							 motion->f_code[1]); \
	motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	\
	motion->pmv[1][1] = motion->pmv[0][1] = motion_y;		\
	other_y = (((motion_y + (motion_y > 0)) >> 1) + get_dmv (decoder) + \
		   decoder->dmv_offset);				\
									\
	MOTION (mpeg2_mc.put, motion->ref[0], motion_x, motion_y, 16, 0); \
	MOTION (mpeg2_mc.avg, motion->ref[1], other_x, other_y, 16, 0);	\
    }									\

MOTION_FUNCTIONS (420, MOTION_420, MOTION_FIELD_420, MOTION_DMV_420,
		  MOTION_ZERO_420)
MOTION_FUNCTIONS (422, MOTION_422, MOTION_FIELD_422, MOTION_DMV_422,
		  MOTION_ZERO_422)
MOTION_FUNCTIONS (444, MOTION_444, MOTION_FIELD_444, MOTION_DMV_444,
		  MOTION_ZERO_444)

/* like motion_frame, but parsing without actual motion compensation */
static void motion_fr_conceal (mpeg2_decoder_t * const decoder)
{
    int tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][0] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[0]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[0]);
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[0][0] = tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][1] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[1]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[1]);
    decoder->f_motion.pmv[1][1] = decoder->f_motion.pmv[0][1] = tmp;

    DUMPBITS (bit_buf, bits, 1); /* remove marker_bit */
}

static void motion_fi_conceal (mpeg2_decoder_t * const decoder)
{
    int tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    DUMPBITS (bit_buf, bits, 1); /* remove field_select */

    tmp = (decoder->f_motion.pmv[0][0] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[0]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[0]);
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[0][0] = tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][1] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[1]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[1]);
    decoder->f_motion.pmv[1][1] = decoder->f_motion.pmv[0][1] = tmp;

    DUMPBITS (bit_buf, bits, 1); /* remove marker_bit */
}

#undef bit_buf
#undef bits
#undef bit_ptr

#define MOTION_CALL(routine,direction)					\
    do {								\
	if ((direction) & MACROBLOCK_MOTION_FORWARD)			\
	    routine (decoder, &(decoder->f_motion), mpeg2_mc.put);	\
	if ((direction) & MACROBLOCK_MOTION_BACKWARD)			\
	    routine (decoder, &(decoder->b_motion),			\
		     ((direction) & MACROBLOCK_MOTION_FORWARD ?		\
		      mpeg2_mc.avg : mpeg2_mc.put));			\
    } while (0)

#define NEXT_MACROBLOCK							\
    do {								\
	if(decoder->quant_store) {					\
	    if (decoder->picture_structure == TOP_FIELD)		\
		decoder->quant_store[2 * decoder->quant_stride		\
				     * (decoder->v_offset >> 4)		\
				     + (decoder->offset >> 4)]		\
		    = decoder->quantizer_scale;				\
	    else if (decoder->picture_structure == BOTTOM_FIELD)	\
		decoder->quant_store[2 * decoder->quant_stride		\
				     * (decoder->v_offset >> 4)		\
				     + decoder->quant_stride		\
				     + (decoder->offset >> 4)]		\
		    = decoder->quantizer_scale;				\
	      /*else							\
		decoder->quant_store[decoder->quant_stride		\
		* (decoder->v_offset >> 4)				\
		+ (decoder->offset >> 4)]				\
		= decoder->quantizer_scale;*/				\
	}								\
	decoder->offset += 16;						\
	if (decoder->offset == decoder->width) {			\
	    do { /* just so we can use the break statement */		\
		if (decoder->convert) {					\
		    decoder->convert (decoder->convert_id, decoder->dest, \
				      decoder->v_offset);		\
		    if (decoder->coding_type == B_TYPE)			\
			break;						\
		}							\
		decoder->dest[0] += decoder->slice_stride;		\
		decoder->dest[1] += decoder->slice_uv_stride;		\
		decoder->dest[2] += decoder->slice_uv_stride;		\
	    } while (0);						\
	    decoder->v_offset += 16;					\
	    if (decoder->v_offset > decoder->limit_y) {			\
		if (mpeg2_cpu_state_restore)				\
		    mpeg2_cpu_state_restore (&cpu_state);		\
		return;							\
	    }								\
	    decoder->offset = 0;					\
	}								\
    } while (0)

/**
 * Dummy motion decoding function, to avoid calling NULL in
 * case of malformed streams.
 */
static void motion_dummy (mpeg2_decoder_t * const decoder,
                          motion_t * const motion,
                          mpeg2_mc_fct * const * const table)
{
}

void mpeg2_init_fbuf (mpeg2_decoder_t * decoder, uint8_t * current_fbuf[3],
		      uint8_t * forward_fbuf[3], uint8_t * backward_fbuf[3])
{
    int offset, stride, height, bottom_field;
    char * ftype[4]={"I type", "P type", "B type", "D type"};
    
    stride = decoder->stride_frame;
    bottom_field = (decoder->picture_structure == BOTTOM_FIELD);
    offset = bottom_field ? stride : 0;
    height = decoder->height;

    decoder->picture_dest[0] = current_fbuf[0] + offset;
    decoder->picture_dest[1] = current_fbuf[1] + (offset >> 1);
    decoder->picture_dest[2] = current_fbuf[2] + (offset >> 1);

    decoder->f_motion.ref[0][0] = forward_fbuf[0] + offset;
    decoder->f_motion.ref[0][1] = forward_fbuf[1] + (offset >> 1);
    decoder->f_motion.ref[0][2] = forward_fbuf[2] + (offset >> 1);

    decoder->b_motion.ref[0][0] = backward_fbuf[0] + offset;
    decoder->b_motion.ref[0][1] = backward_fbuf[1] + (offset >> 1);
    decoder->b_motion.ref[0][2] = backward_fbuf[2] + (offset >> 1);

      //printf("cur : %x %x %x, f : %x %x %x, b : %x %x %x\n", decoder->picture_dest[0], decoder->picture_dest[1], decoder->picture_dest[2], decoder->f_motion.ref[0][0], decoder->f_motion.ref[0][1], decoder->f_motion.ref[0][2], decoder->b_motion.ref[0][0], decoder->b_motion.ref[0][1], decoder->b_motion.ref[0][2]);

    if (decoder->picture_structure != FRAME_PICTURE) {
	decoder->dmv_offset = bottom_field ? 1 : -1;
	decoder->f_motion.ref2[0] = decoder->f_motion.ref[bottom_field];
	decoder->f_motion.ref2[1] = decoder->f_motion.ref[!bottom_field];
	decoder->b_motion.ref2[0] = decoder->b_motion.ref[bottom_field];
	decoder->b_motion.ref2[1] = decoder->b_motion.ref[!bottom_field];
	offset = stride - offset;

	if (decoder->second_field && (decoder->coding_type != B_TYPE))
	    forward_fbuf = current_fbuf;

	decoder->f_motion.ref[1][0] = forward_fbuf[0] + offset;
	decoder->f_motion.ref[1][1] = forward_fbuf[1] + (offset >> 1);
	decoder->f_motion.ref[1][2] = forward_fbuf[2] + (offset >> 1);

	decoder->b_motion.ref[1][0] = backward_fbuf[0] + offset;
	decoder->b_motion.ref[1][1] = backward_fbuf[1] + (offset >> 1);
	decoder->b_motion.ref[1][2] = backward_fbuf[2] + (offset >> 1);

	stride <<= 1;
	height >>= 1;
    }

    decoder->stride = stride;
    decoder->uv_stride = stride >> 1;
    decoder->slice_stride = 16 * stride;
    decoder->slice_uv_stride =
	decoder->slice_stride >> (2 - decoder->chroma_format);
    decoder->limit_x = 2 * decoder->width - 32;
    decoder->limit_y_16 = 2 * height - 32;
    decoder->limit_y_8 = 2 * height - 16;
    decoder->limit_y = height - 16;

    if (decoder->mpeg1) {
	decoder->motion_parser[0] = motion_zero_420;
        decoder->motion_parser[MC_FIELD] = motion_dummy;
 	decoder->motion_parser[MC_FRAME] = motion_mp1;
        decoder->motion_parser[MC_DMV] = motion_dummy;
	decoder->motion_parser[4] = motion_reuse_420;
    } else if (decoder->picture_structure == FRAME_PICTURE) {
	if (decoder->chroma_format == 0) {
	    decoder->motion_parser[0] = motion_zero_420;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_420;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_420;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_420;
	    decoder->motion_parser[4] = motion_reuse_420;
	} else if (decoder->chroma_format == 1) {
	    decoder->motion_parser[0] = motion_zero_422;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_422;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_422;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_422;
	    decoder->motion_parser[4] = motion_reuse_422;
	} else {
	    decoder->motion_parser[0] = motion_zero_444;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_444;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_444;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_444;
	    decoder->motion_parser[4] = motion_reuse_444;
	}
    } else {
	if (decoder->chroma_format == 0) {
	    decoder->motion_parser[0] = motion_zero_420;
	    decoder->motion_parser[MC_FIELD] = motion_fi_field_420;
	    decoder->motion_parser[MC_16X8] = motion_fi_16x8_420;
	    decoder->motion_parser[MC_DMV] = motion_fi_dmv_420;
	    decoder->motion_parser[4] = motion_reuse_420;
	} else if (decoder->chroma_format == 1) {
	    decoder->motion_parser[0] = motion_zero_422;
	    decoder->motion_parser[MC_FIELD] = motion_fi_field_422;
	    decoder->motion_parser[MC_16X8] = motion_fi_16x8_422;
	    decoder->motion_parser[MC_DMV] = motion_fi_dmv_422;
	    decoder->motion_parser[4] = motion_reuse_422;
	} else {
	    decoder->motion_parser[0] = motion_zero_444;
	    decoder->motion_parser[MC_FIELD] = motion_fi_field_444;
	    decoder->motion_parser[MC_16X8] = motion_fi_16x8_444;
	    decoder->motion_parser[MC_DMV] = motion_fi_dmv_444;
	    decoder->motion_parser[4] = motion_reuse_444;
	}
    }
}

static inline int slice_init (mpeg2_decoder_t * const decoder, int code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int offset;
    const MBAtab * mba;

    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
#ifdef JZC_MXU_OPT
        decoder->dc_dct_pred[2] = 128 << decoder->intra_dc_precision;
#else
    decoder->dc_dct_pred[2] = 16384;
#endif
    decoder->f_motion.pmv[0][0] = decoder->f_motion.pmv[0][1] = 0;
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[1][1] = 0;
    decoder->b_motion.pmv[0][0] = decoder->b_motion.pmv[0][1] = 0;
    decoder->b_motion.pmv[1][0] = decoder->b_motion.pmv[1][1] = 0;

    if (decoder->vertical_position_extension) {
	code += UBITS (bit_buf, 3) << 7;
	DUMPBITS (bit_buf, bits, 3);
    }
    decoder->v_offset = (code - 1) * 16;
    offset = 0;
    if (!(decoder->convert) || decoder->coding_type != B_TYPE)
	offset = (code - 1) * decoder->slice_stride;

    decoder->dest[0] = decoder->picture_dest[0] + offset;
    offset >>= (2 - decoder->chroma_format);
    decoder->dest[1] = decoder->picture_dest[1] + offset;
    decoder->dest[2] = decoder->picture_dest[2] + offset;

    get_quantizer_scale (decoder);

      /* ignore intra_slice and all the extra data */
    while (bit_buf & 0x80000000) {
	DUMPBITS (bit_buf, bits, 9);
	NEEDBITS (bit_buf, bits, bit_ptr);
    }

      /* decode initial macroblock address increment */
    offset = 0;
    while (1) {
	if (bit_buf >= 0x08000000) {
	    mba = MBA_5 + (UBITS (bit_buf, 6) - 2);
	    break;
	} else if (bit_buf >= 0x01800000) {
	    mba = MBA_11 + (UBITS (bit_buf, 12) - 24);
	    break;
	} else switch (UBITS (bit_buf, 12)) {
	    case 8:		/* macroblock_escape */
		offset += 33;
		DUMPBITS (bit_buf, bits, 11);
		NEEDBITS (bit_buf, bits, bit_ptr);
		continue;
	    case 15:	/* macroblock_stuffing (MPEG1 only) */
		bit_buf &= 0xfffff;
		DUMPBITS (bit_buf, bits, 11);
		NEEDBITS (bit_buf, bits, bit_ptr);
		continue;
	    default:	/* error */
		return 1;
	    }
    }
    DUMPBITS (bit_buf, bits, mba->len + 1);
    decoder->offset = (offset + mba->mba) << 4;

    while (decoder->offset - decoder->width >= 0) {
	decoder->offset -= decoder->width;
	if (!(decoder->convert) || decoder->coding_type != B_TYPE) {
	    decoder->dest[0] += decoder->slice_stride;
	    decoder->dest[1] += decoder->slice_uv_stride;
	    decoder->dest[2] += decoder->slice_uv_stride;
	}
	decoder->v_offset += 16;
    }
    if (decoder->v_offset > decoder->limit_y)
	return 1;

    return 0;
#undef bit_buf
#undef bits
#undef bit_ptr
}

#ifdef MPEG2_SCH_CONTROL
#include "soc/mpeg2_dcore.h"
#include "soc/jzm_mpeg2_dec.c"
#include "soc/jzm_mpeg2_dec.h"
#include "soc/jzm_vpu.h"
#define write_vpu_reg(off, value) (*((volatile unsigned int *)(off)) = (value))
#define read_vpu_reg(off, ofst)   (*((volatile unsigned int *)(off + ofst)))

extern volatile unsigned char *gp0_base;
extern volatile unsigned char *vpu_base;
extern volatile unsigned char *sde_base;

uint32_t coef_qt_hw[4][16];
int frame_num;
#endif

//#define JZC_SDE_HW_DEBUG
static comp_slif(_M2D_SliceInfo *s1, _M2D_SliceInfo *s2){
#ifdef JZC_SDE_HW_DEBUG
    if (s1->des_va != s2->des_va)
	printf("des_va %x %x\n", s1->des_va, s2->des_va);

    if (s1->des_pa != s2->des_pa)
	printf("des_Pa %x %x\n", s1->des_pa, s2->des_pa);

    _M2D_BitStream *bs1 = &(s1->bs_buf);
    _M2D_BitStream *bs2 = &(s2->bs_buf);
    if (bs1->bit_ofst != bs2->bit_ofst)
	printf("bit_ofst %d %d\n", bs1->bit_ofst, bs2->bit_ofst);
    unsigned char *bbuf1 = (unsigned char *)s1->bs_buf.buffer;
    unsigned char *bbuf2 = (unsigned char *)bs_data;
#if 0
    if (memcmp(bbuf1, bbuf2, bs_len*4)){
	printf("buffer\n");
	unsigned char *tb1 = (unsigned char *)s1->bs_buf.buffer;
	unsigned char *tb2 = (unsigned char *)bs_data;
	tb1 += bs_len;
	tb2 += bs_len;
	int i;
	for (i = 0; i < (bs_len + 7) / 8; i++){
	      //printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", tb1[i*8+0], tb1[i*8+1], tb1[i*8+2], tb1[i*8+3], tb1[i*8+4], tb1[i*8+5], tb1[i*8+6], tb1[i*8+7]);
	      //printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", tb2[i*8+0], tb2[i*8+1], tb2[i*8+2], tb2[i*8+3], tb2[i*8+4], tb2[i*8+5], tb2[i*8+6], tb2[i*8+7]);
	}
    }
#endif
      //if (memcmp(s1->pic_cur, s2->pic_cur, 8))
      //printf("pic_cur");

    if (memcmp(s1->f_code, s2->f_code, 4)){
	printf("f_code\n");
	printf("%d %d %d %d\n", s1->f_code[0][0], s1->f_code[0][1], s1->f_code[1][0], s1->f_code[1][1]);
	printf("%d %d %d %d\n", s2->f_code[0][0], s2->f_code[0][1], s2->f_code[1][0], s2->f_code[1][1]);
    }

    if (s1->sec_fld != s2->sec_fld)
	printf("sec_fld %x %x\n", s1->sec_fld, s2->sec_fld);
    if (s1->dmv_ofst != s2->dmv_ofst)
	printf("dmv_ofst %x %x\n", s1->dmv_ofst, s2->dmv_ofst);
    if (s1->qs_code != s2->qs_code)
	printf("qs_code %x %x\n", s1->qs_code, s2->qs_code);
    if (s1->q_scale_type != s2->q_scale_type)
	printf("q_scale_type %x %x\n", s1->q_scale_type, s2->q_scale_type);
    if (s1->top_fi_first != s2->top_fi_first)
	printf("top_fi_first %x %x\n", s1->top_fi_first, s2->top_fi_first);
    if (s1->mpeg1 != s2->mpeg1)
	printf("mpeg1 %x %x\n", s1->mpeg1, s2->mpeg1);
    if (s1->intra_vlc_format != s2->intra_vlc_format)
	printf("intra_vlc_format %x %x\n", s1->intra_vlc_format, s2->intra_vlc_format);
    if (s1->intra_dc_pre != s2->intra_dc_pre)
	printf("intra_dc_pre %x %x\n", s1->intra_dc_pre, s2->intra_dc_pre);
    if (s1->conceal_mv != s2->conceal_mv)
	printf("conceal_mv %x %x\n", s1->conceal_mv, s2->conceal_mv);
    if (s1->pic_type != s2->pic_type)
	printf("pic_type %x %x\n", s1->pic_type, s2->pic_type);
    if (s1->fr_pred_fr_dct != s2->fr_pred_fr_dct)
	printf("fr_pred_fr_dct %x %x\n", s1->fr_pred_fr_dct, s2->fr_pred_fr_dct);
    if (s1->coding_type != s2->coding_type)
	printf("coding_type %x %x\n", s1->coding_type, s2->coding_type);
    if (s1->mb_width != s2->mb_width)
	printf("mb_width %x %x\n", s1->mb_width, s2->mb_width);
    if (s1->mb_height != s2->mb_height)
	printf("mb_height %x %x\n", s1->mb_height, s2->mb_height);
    if (s1->mb_pos_x != s2->mb_pos_x)
	printf("mb_pos_x %x %x\n", s1->mb_pos_x, s2->mb_pos_x);
    if (s1->mb_pos_y != s2->mb_pos_y)
	printf("mb_pos_y %x %x\n", s1->mb_pos_y, s2->mb_pos_y);

    if (memcmp(s1->coef_qt, s2->coef_qt, 4*16*4))
	printf("coef_qt\n");
    if (memcmp(s1->scan, s2->scan, 4*16))
	printf("scan\n");
#endif
    return;
}

#ifdef MPEG2_SCH_CONTROL
#undef sprintf
#undef fprintf
void dump_mpeg2_slice_info(_M2D_SliceInfo *s)
{
    int i;
    char filename[100];
    sprintf(filename, "frame%d.txt", frame_num);
    FILE * fp = fopen(filename, "w+");
    /* fprintf(fp, "w : %d, h : %d, pos_x : %d, pos_y : %d\n", */
    /* 	   s->mb_width, s->mb_height, s->mb_pos_x, s->mb_pos_y); */

    fprintf(fp, "mb_width = 0x%08x\n", s->mb_width);
    fprintf(fp, "mb_height = 0x%08x\n", s->mb_height);
    fprintf(fp, "mb_pos_x = 0x%08x\n", s->mb_pos_x);
    fprintf(fp, "mb_pos_y = 0x%08x\n", s->mb_pos_y);

    fprintf(fp, "coding_type = 0x%08x\n", s->coding_type);
    fprintf(fp, "fr_pred_fr_dct = 0x%08x\n", s->fr_pred_fr_dct);
    fprintf(fp, "pic_type = 0x%08x\n", s->pic_type);
    fprintf(fp, "conceal_mv = 0x%08x\n", s->conceal_mv);
    fprintf(fp, "intra_dc_pre = 0x%08x\n", s->intra_dc_pre);
    fprintf(fp, "intra_vlc_format = 0x%08x\n", s->intra_vlc_format);
    fprintf(fp, "mpeg1 = 0x%08x\n", s->mpeg1);
    fprintf(fp, "top_fi_first = 0x%08x\n", s->top_fi_first);
    fprintf(fp, "q_scale_type = 0x%08x\n", s->q_scale_type);
    fprintf(fp, "qs_code = 0x%08x\n", s->qs_code);
    fprintf(fp, "dmv_ofst = 0x%08x\n", s->dmv_ofst);
    fprintf(fp, "sec_fld = 0x%08x\n", s->sec_fld);

    fprintf(fp, "coef_qt :\n");
    for(i = 0; i < 4; i++){
	unsigned int * data = s->coef_qt[i];
	fprintf(fp, "0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x"
	       " 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
	       data[9], data[10], data[11], data[12], data[13], data[14], data[i], data[15]);
    }

    fprintf(fp, "scan : 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x"
	   " 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   s->scan[0], s->scan[1], s->scan[2], s->scan[3],
	   s->scan[4], s->scan[5], s->scan[6], s->scan[7],
	   s->scan[9], s->scan[10], s->scan[11], s->scan[12],
	   s->scan[13], s->scan[14], s->scan[i], s->scan[15]);

    fprintf(fp, "f_code : 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   s->f_code[0][0], s->f_code[0][1], s->f_code[1][0], s->f_code[1][1]);

    fprintf(fp, "cur : 0x%08x 0x%08x, ref_f : 0x%08x 0x%08x, ref_b : 0x%08x 0x%08x\n",
	   s->pic_ref[0][0], s->pic_ref[0][1], s->pic_ref[1][1], s->pic_ref[1][1],
	   s->pic_cur[0], s->pic_cur[1]);

    /* fprintf(fp, "C : 0x%08x 0x%08x, F : 0x%08x 0x%08x, B : 0x%08x 0x%08x\n", */
    /* 	   s->pic_cur[0], s->pic_cur[1], */
    /* 	   s->pic_ref[0][0], s->pic_ref[1][0], s->pic_ref[0][1], s->pic_ref[1][1]); */

    fclose(fp);
}

void mpeg2_slice (mpeg2_decoder_t * const decoder, const int code,
		  const uint8_t * const buffer)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    cpu_state_t cpu_state;

    bitstream_init (decoder, buffer);

    if (slice_init (decoder, code))
	return;

    if (mpeg2_cpu_state_save)
	mpeg2_cpu_state_save (&cpu_state);

      /* ------------ init slice info ------------- */
      //printf("--------------- init %d slice info -------------------\n", decoder->v_offset / 16);
    _M2D_SliceInfo *s= &(decoder->slice_info_hw);
      //memset(s, 0, sizeof(_M2D_SliceInfo));
    _M2D_BitStream *bs= &s->bs_buf;
    
    s->des_va= decoder->vdma_base;
    s->des_pa= get_phy_addr(decoder->vdma_base);

    NEEDBITS(bit_buf,bits,bit_ptr);
      /*---------------assign bs begin--------------------*/
    bs->buffer= bit_ptr - 4;   /*BS buffer physical address*/
    bs->bit_ofst= bits + 16;  /*bit offset for first word*/
    uint8_t tmp_bs= (bs->buffer& 0x03)*8 + bs->bit_ofst;      
    if (tmp_bs >= 32){  /*align one words*/
        bs->buffer= (((bs->buffer>>2)+ 1)<< 2);
        bs->bit_ofst= tmp_bs- 32;
    } else {
        bs->buffer= ((bs->buffer>>2)<<2);
        bs->bit_ofst= tmp_bs;
    }

      // -------------- copy bs data, soft buffer can not be used
    int i, j;
    uint32_t *bs_data_tmp = (unsigned int *)bs->buffer;
      //memset(bs_data, 0, 0x5000);
    for(i = 0; i < ((decoder->tbslen + 3) / 4); i++){
        decoder->tbsbuf[i] = bs_data_tmp[i];
    }
    bs->buffer = get_phy_addr(decoder->tbsbuf);
#ifdef JZM_DEBUG_MPEG2
    mp_msg(NULL, NULL, "crc_code = 0x%08x\n", crc(decoder->tbsbuf, decoder->tbslen, 0));
#endif

      // -------------- evaluate struct s
    s->mb_width= decoder->stride_frame>>4;
    s->mb_height= decoder->height>>4;
    s->mb_pos_x=decoder->offset / 16;
    s->mb_pos_y=decoder->v_offset / 16;

    for(i = 0; i < 4; i++){
        for(j = 0; j < 16; j++)
            s->coef_qt[i][j] = coef_qt_hw[i][j]; /* coef quantizer table */
    }

    uint32_t * scan_table = decoder->scan;
    for(i = 0; i < 16; i++)
        s->scan[i] = scan_table[i];   /* scan table */

    s->coding_type=decoder->coding_type;
    s->fr_pred_fr_dct=decoder->frame_pred_frame_dct;
    s->pic_type=decoder->picture_structure;
    s->conceal_mv=decoder->concealment_motion_vectors;
    s->intra_dc_pre=decoder->intra_dc_precision;
    s->intra_vlc_format=decoder->intra_vlc_format;
    s->mpeg1=decoder->mpeg1;
    s->top_fi_first=decoder->top_field_first;
    s->q_scale_type=decoder->q_scale_type;
    s->qs_code=save_qs_code;
    s->dmv_ofst=decoder->dmv_offset;
    s->sec_fld=decoder->second_field;

    s->f_code[0][0]=decoder->f_motion.f_code[0];
    s->f_code[0][1]=decoder->f_motion.f_code[1];
    s->f_code[1][0]=decoder->b_motion.f_code[0];
    s->f_code[1][1]=decoder->b_motion.f_code[1];

    s->pic_ref[0][0]= get_phy_addr(decoder->f_motion.ref[0][0]);
    s->pic_ref[0][1]= get_phy_addr(decoder->b_motion.ref[0][0]);
    s->pic_ref[1][0]= get_phy_addr(decoder->f_motion.ref[0][1]);
    s->pic_ref[1][1]= get_phy_addr(decoder->b_motion.ref[0][1]);
    s->pic_cur[0]= get_phy_addr(decoder->picture_dest[0]);
    s->pic_cur[1]= get_phy_addr(decoder->picture_dest[1]);

#ifdef JZC_SDE_HW_DEBUG
    if (frame_num == 2 && code == 19){
	printf("comp_slif## start\n");
	comp_slif(&slice_info_hw, &tslice_info_hw);
	printf("comp_slif## end\n");
    }
#endif

    if (code == 1){
	M2D_SliceInit(s);
	jz_dcache_wb();
    }else{
	M2D_SliceInit_ext(s);
	{//decache struct and bs
	    int i, va;

	    va = (int)s;
	    for (i = 0; i < (sizeof(_M2D_SliceInfo) + 32) / 32; i++){
		i_cache(0x19, va, 0);
		va += 32;	
	    }

	    va = (int)decoder->tbsbuf;
	    for (i = 0; i < (decoder->tbslen+3)/4; i++){
		i_cache(0x19, va, 0);
		va += 32;	
	    }

	    i_sync();
	}
    }

#ifdef JZM_DEBUG_MPEG2
    if (s->mb_pos_x == 0 && s->mb_pos_y == 0) {
	dump_mpeg2_slice_info(s);
    }
#endif

      /* --------------- start ACFG config -------------- */
    *((volatile int *)(gp0_base + 0xC)) = 0x0;
    *((volatile int *)(sde_base + 0x0)) = 0x0;
    *((volatile int *)(vpu_base + 0x34)) = 0x0;
    
    // open sch interrupt and close TLB before VDMA config, then VDMA will config regs without TLB
    // Or we are not ensure the TLB is not enable, which may bring out error.
    //write_vpu_reg(vpu_base+ REG_SCH_GLBC, (SCH_INTE_ACFGERR | SCH_INTE_BSERR | SCH_INTE_ENDF) );
    write_vpu_reg(vpu_base+ REG_SCH_GLBC, SCH_GLBC_HIAXI );

    write_vpu_reg(gp0_base + 0x8, (VDMA_ACFG_DHA(s->des_pa) | VDMA_ACFG_RUN));

    //-------- waiting for end
    //while( (read_vpu_reg(gp0_base + 0xC, 0) & 0x4) == 0x0 );
    //while( ((read_vpu_reg(sde_base + 0x0, 0x0) & 0x2) == 0) );  // sde slice done

    int timeout = 0;
    while( ((read_vpu_reg(vpu_base + 0x34, 0x0) & 0x1) == 0) ){
	timeout++;
	if (timeout > 1000000){
	    mp_msg(NULL, NULL, "vpu status = 0x%x, vdma status = 0x%x, vdma dha = 0x%x\n"
		   "sde id = 0x%x, sde cfg0 = 0x%x, sde bsaddr = 0x%x\n",
		   *(volatile unsigned int *)(vpu_base + 0x34),
		   *(volatile unsigned int *)(gp0_base + 0xC),
		   *(volatile unsigned int *)(gp0_base + 0x8),
		   *(volatile unsigned int *)(sde_base + 0x10),
		   *(volatile unsigned int *)(sde_base + 0x14),
		   *(volatile unsigned int *)(sde_base + 0x1c)
		   );
	}
    };

    if (mpeg2_cpu_state_restore)
        mpeg2_cpu_state_restore (&cpu_state);
#undef bit_buf
#undef bits
#undef bit_ptr
}

#else

void mpeg2_slice (mpeg2_decoder_t * const decoder, const int code,
		  const uint8_t * const buffer)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    cpu_state_t cpu_state;

    bitstream_init (decoder, buffer);
    
    if (slice_init (decoder, code))
	return;

    if (mpeg2_cpu_state_save)
	mpeg2_cpu_state_save (&cpu_state);
    
      /* 1/2  1 3 0 7 0 0 0 0 0 0  -- Amazing... */
      /* 1/2  1 3 0 7 0 0 0 0 ce0610 0  -- Dolphin... */
      /* 1/2  1 3 0 7 0 0 0 0 fb98775d 0  -- Discover... */
/*     printf("%x %x %x %x %x %x %x %x %x %x %x\n", decoder->coding_type, decoder->frame_pred_frame_dct, decoder->picture_structure, decoder->concealment_motion_vectors, decoder->intra_dc_precision, decoder->intra_vlc_format, decoder->mpeg1, decoder->top_field_first, decoder->q_scale_type, decoder->dmv_offset, decoder->second_field); */

      /* e e e e | e e 2 2 | e e 3 3 | e e 2 2  -- Amazing... */
      /* e e e e | e e 0 0 | e e 0 0 | e e 0 0  -- Dolphin... (e e 1/2 1/2)*/
      /* e e e e | e e 0 0 | e e 0 0 | e e 0 0  -- Discover... (e e 4 4)*/
/*     printf("%x %x %x %x\n", decoder->b_motion.f_code[0], decoder->b_motion.f_code[1], decoder->f_motion.f_code[0], decoder->f_motion.f_code[1]); */
    while (1) {
	int macroblock_modes;
	int mba_inc;
	const MBAtab * mba;

	NEEDBITS (bit_buf, bits, bit_ptr);

	macroblock_modes = get_macroblock_modes (decoder);

	  /* maybe integrate MACROBLOCK_QUANT test into get_macroblock_modes ? */
	if (macroblock_modes & MACROBLOCK_QUANT)
	    get_quantizer_scale (decoder);

	if (macroblock_modes & MACROBLOCK_INTRA) {

	    int DCT_offset, DCT_stride;
	    int offset;
	    uint8_t * dest_y;

	    if (decoder->concealment_motion_vectors) {
		if (decoder->picture_structure == FRAME_PICTURE)
		    motion_fr_conceal (decoder);
		else
		    motion_fi_conceal (decoder);
	    } else {
		decoder->f_motion.pmv[0][0] = decoder->f_motion.pmv[0][1] = 0;
		decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[1][1] = 0;
		decoder->b_motion.pmv[0][0] = decoder->b_motion.pmv[0][1] = 0;
		decoder->b_motion.pmv[1][0] = decoder->b_motion.pmv[1][1] = 0;
	    }

	    if (macroblock_modes & DCT_TYPE_INTERLACED) {
		DCT_offset = decoder->stride;
		DCT_stride = decoder->stride * 2;
	    } else {
		DCT_offset = decoder->stride * 8;
		DCT_stride = decoder->stride;
	    }

	    offset = decoder->offset;
	    dest_y = decoder->dest[0] + offset;
	    slice_intra_DCT (decoder, 0, dest_y, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + 8, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + DCT_offset, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + DCT_offset + 8, DCT_stride);
	    if (likely (decoder->chroma_format == 0)) {
		slice_intra_DCT (decoder, 1, decoder->dest[1] + (offset >> 1),
				 decoder->uv_stride);
		slice_intra_DCT (decoder, 2, decoder->dest[2] + (offset >> 1),
				 decoder->uv_stride);
		if (decoder->coding_type == D_TYPE) {
		    NEEDBITS (bit_buf, bits, bit_ptr);
		    DUMPBITS (bit_buf, bits, 1);
		}
	    } else if (likely (decoder->chroma_format == 1)) {
		uint8_t * dest_u = decoder->dest[1] + (offset >> 1);
		uint8_t * dest_v = decoder->dest[2] + (offset >> 1);
		DCT_stride >>= 1;
		DCT_offset >>= 1;
		slice_intra_DCT (decoder, 1, dest_u, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset, DCT_stride);
	    } else {
		uint8_t * dest_u = decoder->dest[1] + offset;
		uint8_t * dest_v = decoder->dest[2] + offset;
		slice_intra_DCT (decoder, 1, dest_u, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + 8, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + 8, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset + 8,
				 DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset + 8,
				 DCT_stride);
	    }
	} else {

	    motion_parser_t * parser;

	    if (   ((macroblock_modes >> MOTION_TYPE_SHIFT) < 0)
		   || ((macroblock_modes >> MOTION_TYPE_SHIFT) >=
		       (int)(sizeof(decoder->motion_parser)
			     / sizeof(decoder->motion_parser[0])))
		   ) {
		break; // Illegal !
	    }

	    parser =
		decoder->motion_parser[macroblock_modes >> MOTION_TYPE_SHIFT];
	    MOTION_CALL (parser, macroblock_modes);

	    if (macroblock_modes & MACROBLOCK_PATTERN) {
		int coded_block_pattern;
		int DCT_offset, DCT_stride;

		if (macroblock_modes & DCT_TYPE_INTERLACED) {
		    DCT_offset = decoder->stride;
		    DCT_stride = decoder->stride * 2;
		} else {
		    DCT_offset = decoder->stride * 8;
		    DCT_stride = decoder->stride;
		}

		coded_block_pattern = get_coded_block_pattern (decoder);

		if (likely (decoder->chroma_format == 0)) {
		    int offset = decoder->offset;
		    uint8_t * dest_y = decoder->dest[0] + offset;
		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);
		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + (offset >> 1),
					     decoder->uv_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + (offset >> 1),
					     decoder->uv_stride);
		} else if (likely (decoder->chroma_format == 1)) {
		    int offset;
		    uint8_t * dest_y;

		    coded_block_pattern |= bit_buf & (3 << 30);
		    DUMPBITS (bit_buf, bits, 2);

		    offset = decoder->offset;
		    dest_y = decoder->dest[0] + offset;
		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);

		    DCT_stride >>= 1;
		    DCT_offset = (DCT_offset + offset) >> 1;
		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + (offset >> 1),
					     DCT_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + (offset >> 1),
					     DCT_stride);
		    if (coded_block_pattern & (2 << 30))
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (1 << 30))
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + DCT_offset,
					     DCT_stride);
		} else {
		    int offset;
		    uint8_t * dest_y, * dest_u, * dest_v;

		    coded_block_pattern |= bit_buf & (63 << 26);
		    DUMPBITS (bit_buf, bits, 6);

		    offset = decoder->offset;
		    dest_y = decoder->dest[0] + offset;
		    dest_u = decoder->dest[1] + offset;
		    dest_v = decoder->dest[2] + offset;

		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);

		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1, dest_u, DCT_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2, dest_v, DCT_stride);
		    if (coded_block_pattern & (32 << 26))
			slice_non_intra_DCT (decoder, 1, dest_u + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (16 << 26))
			slice_non_intra_DCT (decoder, 2, dest_v + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (8 << 26))
			slice_non_intra_DCT (decoder, 1, dest_u + 8,
					     DCT_stride);
		    if (coded_block_pattern & (4 << 26))
			slice_non_intra_DCT (decoder, 2, dest_v + 8,
					     DCT_stride);
		    if (coded_block_pattern & (2 << 26))
			slice_non_intra_DCT (decoder, 1,
					     dest_u + DCT_offset + 8,
					     DCT_stride);
		    if (coded_block_pattern & (1 << 26))
			slice_non_intra_DCT (decoder, 2,
					     dest_v + DCT_offset + 8,
					     DCT_stride);
		}
	    }

	    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
#ifdef JZC_MXU_OPT
	        decoder->dc_dct_pred[2] = 128 << decoder->intra_dc_precision; 
#else
	    decoder->dc_dct_pred[2] = 16384;
#endif
	}

	NEXT_MACROBLOCK;

	NEEDBITS (bit_buf, bits, bit_ptr);
	mba_inc = 0;
	while (1) {
	    if (bit_buf >= 0x10000000) {
		mba = MBA_5 + (UBITS (bit_buf, 5) - 2);
		break;
	    } else if (bit_buf >= 0x03000000) {
		mba = MBA_11 + (UBITS (bit_buf, 11) - 24);
		break;
	    } else switch (UBITS (bit_buf, 11)) {
		case 8:		/* macroblock_escape */
		    mba_inc += 33;
		      /* pass through */
		case 15:	/* macroblock_stuffing (MPEG1 only) */
		    DUMPBITS (bit_buf, bits, 11);
		    NEEDBITS (bit_buf, bits, bit_ptr);
		    continue;
		default:	/* end of slice, or error */
		    if (mpeg2_cpu_state_restore)
			mpeg2_cpu_state_restore (&cpu_state);
                
/*                 prin_crc(decoder->dest[0], decoder->dest[1], decoder->dest[2], decoder->stride_frame>>4,  */
/*                          decoder->v_offset / 16); */
		    return;
		}
	}
	DUMPBITS (bit_buf, bits, mba->len);
	mba_inc += mba->mba;

	if (mba_inc) {
	    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
#ifdef JZC_MXU_OPT
                decoder->dc_dct_pred[2] = 128 << decoder->intra_dc_precision;
#else
	    decoder->dc_dct_pred[2] = 16384;
#endif

	    if (decoder->coding_type == P_TYPE) {
		do {
		    MOTION_CALL (decoder->motion_parser[0],
				 MACROBLOCK_MOTION_FORWARD);
		    NEXT_MACROBLOCK;
		} while (--mba_inc);
	    } else {
		do {
		    MOTION_CALL (decoder->motion_parser[4], macroblock_modes);
		    NEXT_MACROBLOCK;
		} while (--mba_inc);
	    }
	}
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}
#endif
