/*
 * decode.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
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
 */

#include "config.h"

#include <string.h>	/* memcmp/memset, try to remove */
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

#include "mpeg2_config.h"
#ifdef JZC_MXU_OPT
#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jzmedia.h"
#endif

#ifdef MPEG2_SCH_CONTROL
#include "soc/mpeg2_dcore.h"
#endif

//#define MPEG2_CRC_CODE
#ifdef MPEG2_CRC_CODE
#include "libjzcommon/crc.c"
short crc_code = 0;
#endif

static int mpeg2_accels = 0;
extern int use_jz_buf;
extern volatile unsigned char *cpm_base;

#define CPM_VPU_SWRST    (cpm_base + 0xC4)
#define CPM_VPU_SR     	 (0x1<<31)
#define CPM_VPU_STP    	 (0x1<<30)
#define CPM_VPU_ACK    	 (0x1<<29)

#define write_cpm_reg(a)    (*(volatile unsigned int *)(CPM_VPU_SWRST) = a)
#define read_cpm_reg()      (*(volatile unsigned int *)(CPM_VPU_SWRST))

#define RST_VPU()            \
{\
     write_cpm_reg(read_cpm_reg() | CPM_VPU_STP); \
     while( !(read_cpm_reg() & CPM_VPU_ACK) );  \
     write_cpm_reg( (read_cpm_reg() | CPM_VPU_SR) & (~CPM_VPU_STP) ); \
     write_cpm_reg( read_cpm_reg() & (~CPM_VPU_SR) & (~CPM_VPU_STP) ); \
}

#define BUFFER_SIZE (1194 * 1024)

const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec)
{
    return &(mpeg2dec->info);
}

static inline int skip_chunk (mpeg2dec_t * mpeg2dec, int bytes)
{
    uint8_t * current;
    uint32_t shift;
    uint8_t * limit;
    uint8_t byte;

    if (!bytes)
	return 0;

    current = mpeg2dec->buf_start;
    shift = mpeg2dec->shift;
    limit = current + bytes;

    do {
	byte = *current++;
	if (shift == 0x00000100) {
	    int skipped;

	    mpeg2dec->shift = 0xffffff00;
	    skipped = current - mpeg2dec->buf_start;
	    mpeg2dec->buf_start = current;
	    return skipped;
	}
	shift = (shift | byte) << 8;
    } while (current < limit);

    mpeg2dec->shift = shift;
    mpeg2dec->buf_start = current;
    return 0;
}

static inline int copy_chunk (mpeg2dec_t * mpeg2dec, int bytes)
{
    uint8_t * current;
    uint32_t shift;
    uint8_t * chunk_ptr;
    uint8_t * limit;
    uint8_t byte;

    if (!bytes)
	return 0;

    current = mpeg2dec->buf_start;
    shift = mpeg2dec->shift;
    chunk_ptr = mpeg2dec->chunk_ptr;
    limit = current + bytes;

    do {
	byte = *current++;
	if (shift == 0x00000100) {
	    int copied;

	    mpeg2dec->shift = 0xffffff00;
	    mpeg2dec->chunk_ptr = chunk_ptr + 1;
	    copied = current - mpeg2dec->buf_start;
	    mpeg2dec->buf_start = current;
	    return copied;
	}
	shift = (shift | byte) << 8;
	*chunk_ptr++ = byte;
    } while (current < limit);

    mpeg2dec->shift = shift;
    mpeg2dec->buf_start = current;
    return 0;
}

void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end)
{
    mpeg2dec->buf_start = start;
    mpeg2dec->buf_end = end;
}

int mpeg2_getpos (mpeg2dec_t * mpeg2dec)
{
    return mpeg2dec->buf_end - mpeg2dec->buf_start;
}

static inline mpeg2_state_t seek_chunk (mpeg2dec_t * mpeg2dec)
{
    int size, skipped;

    size = mpeg2dec->buf_end - mpeg2dec->buf_start;
    skipped = skip_chunk (mpeg2dec, size);
    if (!skipped) {
	mpeg2dec->bytes_since_tag += size;
	return STATE_BUFFER;
    }
    mpeg2dec->bytes_since_tag += skipped;
    mpeg2dec->code = mpeg2dec->buf_start[-1];
    return STATE_INTERNAL_NORETURN;
}

mpeg2_state_t mpeg2_seek_header (mpeg2dec_t * mpeg2dec)
{
    while (!(mpeg2dec->code == 0xb3 ||
	     ((mpeg2dec->code == 0xb7 || mpeg2dec->code == 0xb8 ||
	       !mpeg2dec->code) && mpeg2dec->sequence.width != (unsigned)-1)))
	if (seek_chunk (mpeg2dec) == STATE_BUFFER)
	    return STATE_BUFFER;
    mpeg2dec->chunk_start = mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->user_data_len = 0;
    return ((mpeg2dec->code == 0xb7) ?
	    mpeg2_header_end (mpeg2dec) : mpeg2_parse_header (mpeg2dec));
}

#define RECEIVED(code,state) (((state) << 8) + (code))

extern int frame_num;

static void fprint_refcall(mpeg2dec_t *mpeg2dec){
  FILE *fp = fopen("./reftt2", "w+");
  if (fp == NULL){
    printf("fopen failure\n");
    return NULL;
  }

  unsigned char *yptr = mpeg2dec->decoder.f_motion.ref[0][0];
  unsigned char *cptr = mpeg2dec->decoder.f_motion.ref[0][1];

  int mb_width = mpeg2dec->decoder.width / 16;
  int mb_height = mpeg2dec->decoder.height / 16;
  int i, j, m, n, o;
  int sft[4] = {0, 8, 8*16, 8*16+8};

  fprintf(fp, "ref mpFrame:%d\n", frame_num);
  yptr = mpeg2dec->decoder.f_motion.ref[0][0];
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 16; n++){
	for (m = 0; m < 16; m++){
	  fprintf(fp, "0x%02x, ", yptr[m]);
	}
	yptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  cptr = mpeg2dec->decoder.f_motion.ref[0][1];
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 8; n++){
	for (m = 0; m < 8; m++){
	  fprintf(fp, "0x%02x, ", cptr[m]);
	}
	cptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  cptr = mpeg2dec->decoder.f_motion.ref[0][1];
  cptr += 8;
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 8; n++){
	for (m = 0; m < 8; m++){
	  fprintf(fp, "0x%02x, ", cptr[m]);
	}
	cptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  yptr = mpeg2dec->decoder.b_motion.ref[0][0];
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 16; n++){
	for (m = 0; m < 16; m++){
	  fprintf(fp, "0x%02x, ", yptr[m]);
	}
	yptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  cptr = mpeg2dec->decoder.b_motion.ref[0][1];
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 8; n++){
	for (m = 0; m < 8; m++){
	  fprintf(fp, "0x%02x, ", cptr[m]);
	}
	cptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  cptr = mpeg2dec->decoder.b_motion.ref[0][1];
  cptr += 8;
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 8; n++){
	for (m = 0; m < 8; m++){
	  fprintf(fp, "0x%02x, ", cptr[m]);
	}
	cptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  fclose(fp);
  return;
}

static void fprint_call(mpeg2dec_t *mpeg2dec){
  FILE *fp = fopen("./tt2", "w+");
  if (fp == NULL){
    printf("fopen failure\n");
    return NULL;
  }

  unsigned char *yptr = mpeg2dec->decoder.picture_dest[0];
  unsigned char *cptr = mpeg2dec->decoder.picture_dest[1];
  int mb_width = mpeg2dec->decoder.width / 16;
  int mb_height = mpeg2dec->decoder.height / 16;
  int i, j, m, n, o;
  int sft[4] = {0, 8, 8*16, 8*16+8};

  fprintf(fp, "mpFrame:%d\n", frame_num);
  for (j = 0; j < mb_height; j++){
    //yptr = mpeg2dec->decoder.picture_dest[0];
    //yptr = mpeg2dec->decoder.t_yptr;
    //yptr += j * mpeg2dec->decoder.y_stride;
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 16; n++){
	for (m = 0; m < 16; m++){
	  fprintf(fp, "0x%02x, ", yptr[m]);
	}
	yptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  cptr = mpeg2dec->decoder.picture_dest[1];
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 8; n++){
	for (m = 0; m < 8; m++){
	  fprintf(fp, "0x%02x, ", cptr[m]);
	}
	cptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  cptr = mpeg2dec->decoder.picture_dest[1];
  cptr += 8;
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      fprintf(fp, "mb_x:%d mb_y:%d\n", i, j);
      for (n = 0; n < 8; n++){
	for (m = 0; m < 8; m++){
	  fprintf(fp, "0x%02x, ", cptr[m]);
	}
	cptr+=16;
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
  }

  fclose(fp);
  return;
}

void mpeg2_crc(mpeg2dec_t * mpeg2dec){
#ifdef MPEG2_CRC_CODE
  unsigned char *y_ptr = NULL;
  unsigned char *c_ptr = NULL;
  int mb_width = mpeg2dec->decoder.stride_frame>>4;
  int mb_height= mpeg2dec->decoder.height>>4;
  int j = 0;

  //crc_code = 0;

  y_ptr = mpeg2dec->decoder.picture_dest[0];
  c_ptr = mpeg2dec->decoder.picture_dest[1];
  for (j = 0; j < mb_height; j++){
      crc_code = crc(y_ptr, mb_width * 256, crc_code);
      crc_code = crc(c_ptr, mb_width * 128, crc_code);
      y_ptr += (mb_width * 256);
      c_ptr += (mb_width * 128);
  }

  mp_msg(NULL, NULL, "%d  crc_code:0x%04x %d\n", frame_num, crc_code, mpeg2dec->decoder.coding_type);
#endif
}

#if 0
#undef sprintf
#undef fprintf
static void dump_frame_data(mpeg2dec_t * mpeg2dec)
{
    int i, j, k;
    int mb_width = mpeg2dec->decoder.width / 16;
    int mb_height = mpeg2dec->decoder.height / 16;
    unsigned char *yptr = mpeg2dec->decoder.picture_dest[0];
    unsigned char *cptr = mpeg2dec->decoder.picture_dest[1];
    char filename[100];
    sprintf(filename, "frame_data%d.c", frame_num);
    FILE * fp = fopen(filename, "w+");
    for (i = 0; i < mb_height; i++)
	for (j = 0; j < mb_width; j++) {
	    fprintf(fp, "mb_pos : %d %d\nY:\n", i, j);
	    for (k = 0; k < 16; k++) {
		fprintf(fp, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x"
			" 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			yptr[0], yptr[1], yptr[2], yptr[3], yptr[4], yptr[5], yptr[6], yptr[7],
			yptr[8], yptr[9], yptr[10], yptr[11], yptr[12], yptr[13], yptr[14], yptr[15]);
		yptr += 16;
	    }
	    for (k = 0; k < 8; k++) {
		fprintf(fp, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x **"
			" 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			cptr[0], cptr[1], cptr[2], cptr[3], cptr[4], cptr[5], cptr[6], cptr[7],
			cptr[8], cptr[9], cptr[10], cptr[11], cptr[12], cptr[13], cptr[14], cptr[15]);
		cptr += 16;
	    }
	}
    fclose(fp);
}
#endif

mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec)
{
    int size_buffer, size_chunk, copied;
    int frame_flag = 0;
    if (mpeg2dec->action) {
	mpeg2_state_t state;

	state = mpeg2dec->action (mpeg2dec);
	if ((int)state > (int)STATE_INTERNAL_NORETURN)
	    return state;
    }

    RST_VPU();
    while (1) {
	while ((unsigned) (mpeg2dec->code - mpeg2dec->first_decode_slice) <
	       mpeg2dec->nb_decode_slices) {
	    size_buffer = mpeg2dec->buf_end - mpeg2dec->buf_start;
	    size_chunk = (mpeg2dec->chunk_buffer + BUFFER_SIZE -
			  mpeg2dec->chunk_ptr);
	    if (size_buffer <= size_chunk) {
		copied = copy_chunk (mpeg2dec, size_buffer);
		if (!copied) {
		    mpeg2dec->bytes_since_tag += size_buffer;
		    mpeg2dec->chunk_ptr += size_buffer;
		    return STATE_BUFFER;
		}
	    } else {
		copied = copy_chunk (mpeg2dec, size_chunk);
		if (!copied) {
		    /* filled the chunk buffer without finding a start code */
		    mpeg2dec->bytes_since_tag += size_chunk;
		    mpeg2dec->action = seek_chunk;
		    return STATE_INVALID;
		}
	    }
            mpeg2dec->decoder.tbslen = copied;
	    mpeg2dec->bytes_since_tag += copied;

            /* decode a slice */
	    mpeg2_slice (&(mpeg2dec->decoder), mpeg2dec->code,
			 mpeg2dec->chunk_start);
	    frame_flag = 1;
	    mpeg2dec->code = mpeg2dec->buf_start[-1];
	    mpeg2dec->chunk_ptr = mpeg2dec->chunk_start;
	}
	if ((unsigned) (mpeg2dec->code - 1) >= 0xb0 - 1)
	    break;
	if (seek_chunk (mpeg2dec) == STATE_BUFFER)
	    return STATE_BUFFER;
    }

    if (frame_flag == 1){
	frame_num++;
#ifdef MPEG2_CRC_CODE
	if (frame_num == 2){
	    fprint_refcall(mpeg2dec);
	    fprint_call(mpeg2dec);
	}
	mpeg2_crc(mpeg2dec);
#endif
    }

    mpeg2dec->action = mpeg2_seek_header;
    switch (mpeg2dec->code) {
    case 0x00:
	return mpeg2dec->state;
    case 0xb3:
    case 0xb7:
    case 0xb8:
	return (mpeg2dec->state == STATE_SLICE) ? STATE_SLICE : STATE_INVALID;
    default:
	mpeg2dec->action = seek_chunk;
	return STATE_INVALID;
    }
}

mpeg2_state_t mpeg2_parse_header (mpeg2dec_t * mpeg2dec)
{
    static int (* process_header[]) (mpeg2dec_t * mpeg2dec) = {
	mpeg2_header_picture, mpeg2_header_extension, mpeg2_header_user_data,
	mpeg2_header_sequence, NULL, NULL, NULL, NULL, mpeg2_header_gop
    };
    int size_buffer, size_chunk, copied;

    mpeg2dec->action = mpeg2_parse_header;
    mpeg2dec->info.user_data = NULL;	mpeg2dec->info.user_data_len = 0;
    while (1) {
	size_buffer = mpeg2dec->buf_end - mpeg2dec->buf_start;
	size_chunk = (mpeg2dec->chunk_buffer + BUFFER_SIZE -
		      mpeg2dec->chunk_ptr);
	if (size_buffer <= size_chunk) {
	    copied = copy_chunk (mpeg2dec, size_buffer);
	    if (!copied) {
		mpeg2dec->bytes_since_tag += size_buffer;
		mpeg2dec->chunk_ptr += size_buffer;
		return STATE_BUFFER;
	    }
	} else {
	    copied = copy_chunk (mpeg2dec, size_chunk);
	    if (!copied) {
		/* filled the chunk buffer without finding a start code */
		mpeg2dec->bytes_since_tag += size_chunk;
		mpeg2dec->code = 0xb4;
		mpeg2dec->action = mpeg2_seek_header;
		return STATE_INVALID;
	    }
	}
	mpeg2dec->bytes_since_tag += copied;

	if (process_header[mpeg2dec->code & 0x0b] (mpeg2dec)) {
	    mpeg2dec->code = mpeg2dec->buf_start[-1];
	    mpeg2dec->action = mpeg2_seek_header;
	    return STATE_INVALID;
	}

	mpeg2dec->code = mpeg2dec->buf_start[-1];
	switch (RECEIVED (mpeg2dec->code, mpeg2dec->state)) {

	/* state transition after a sequence header */
	case RECEIVED (0x00, STATE_SEQUENCE):
	case RECEIVED (0xb8, STATE_SEQUENCE):
	    mpeg2_header_sequence_finalize (mpeg2dec);
	    break;

	/* other legal state transitions */
	case RECEIVED (0x00, STATE_GOP):
	    mpeg2_header_gop_finalize (mpeg2dec);
	    break;
	case RECEIVED (0x01, STATE_PICTURE):
	case RECEIVED (0x01, STATE_PICTURE_2ND):
	    mpeg2_header_picture_finalize (mpeg2dec, mpeg2_accels);
	    mpeg2dec->action = mpeg2_header_slice_start;
	    break;

	/* legal headers within a given state */
	case RECEIVED (0xb2, STATE_SEQUENCE):
	case RECEIVED (0xb2, STATE_GOP):
	case RECEIVED (0xb2, STATE_PICTURE):
	case RECEIVED (0xb2, STATE_PICTURE_2ND):
	case RECEIVED (0xb5, STATE_SEQUENCE):
	case RECEIVED (0xb5, STATE_PICTURE):
	case RECEIVED (0xb5, STATE_PICTURE_2ND):
	    mpeg2dec->chunk_ptr = mpeg2dec->chunk_start;
	    continue;

	default:
	    mpeg2dec->action = mpeg2_seek_header;
	    return STATE_INVALID;
	}

	mpeg2dec->chunk_start = mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
	mpeg2dec->user_data_len = 0;
	return mpeg2dec->state;
    }
}

int mpeg2_convert (mpeg2dec_t * mpeg2dec, mpeg2_convert_t convert, void * arg)
{
    mpeg2_convert_init_t convert_init;
    int error;

    error = convert (MPEG2_CONVERT_SET, NULL, &(mpeg2dec->sequence), 0,
		     mpeg2_accels, arg, &convert_init);
    if (!error) {
	mpeg2dec->convert = convert;
	mpeg2dec->convert_arg = arg;
	mpeg2dec->convert_id_size = convert_init.id_size;
	mpeg2dec->convert_stride = 0;
    }
    return error;
}

int mpeg2_stride (mpeg2dec_t * mpeg2dec, int stride)
{
    if (!mpeg2dec->convert) {
	if (stride < (int) mpeg2dec->sequence.width)
	    stride = mpeg2dec->sequence.width;
	mpeg2dec->decoder.stride_frame = stride;
    } else {
	mpeg2_convert_init_t convert_init;

	stride = mpeg2dec->convert (MPEG2_CONVERT_STRIDE, NULL,
				    &(mpeg2dec->sequence), stride,
				    mpeg2_accels, mpeg2dec->convert_arg,
				    &convert_init);
	mpeg2dec->convert_id_size = convert_init.id_size;
	mpeg2dec->convert_stride = stride;
    }
    return stride;
}

void mpeg2_set_buf (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id)
{
    mpeg2_fbuf_t * fbuf;
    
    if (mpeg2dec->custom_fbuf) {
	if (mpeg2dec->state == STATE_SEQUENCE) {
	    mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
	    mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
	}
	mpeg2_set_fbuf (mpeg2dec, (mpeg2dec->decoder.coding_type ==
				   PIC_FLAG_CODING_TYPE_B));
	fbuf = mpeg2dec->fbuf[0];
    } else {
	fbuf = &(mpeg2dec->fbuf_alloc[mpeg2dec->alloc_index].fbuf);
	mpeg2dec->alloc_index_user = ++mpeg2dec->alloc_index;
    }
    fbuf->buf[0] = buf[0];
    fbuf->buf[1] = buf[1];
    fbuf->buf[2] = buf[2];
    fbuf->id = id;

    // HACK! FIXME! At first I frame, copy pointers to prediction frame too!
    if (mpeg2dec->custom_fbuf && !mpeg2dec->fbuf[1]->buf[0]) {
	mpeg2dec->fbuf[1]->buf[0] = buf[0];
	mpeg2dec->fbuf[1]->buf[1] = buf[1];
	mpeg2dec->fbuf[1]->buf[2] = buf[2];
	mpeg2dec->fbuf[1]->id     = NULL;
    }
}

void mpeg2_custom_fbuf (mpeg2dec_t * mpeg2dec, int custom_fbuf)
{
    mpeg2dec->custom_fbuf = custom_fbuf;
}

void mpeg2_skip (mpeg2dec_t * mpeg2dec, int skip)
{
    mpeg2dec->first_decode_slice = 1;
    mpeg2dec->nb_decode_slices = skip ? 0 : (0xb0 - 1);
}

void mpeg2_slice_region (mpeg2dec_t * mpeg2dec, int start, int end)
{
    start = (start < 1) ? 1 : (start > 0xb0) ? 0xb0 : start;
    end = (end < start) ? start : (end > 0xb0) ? 0xb0 : end;
    mpeg2dec->first_decode_slice = start;
    mpeg2dec->nb_decode_slices = end - start;
}

void mpeg2_tag_picture (mpeg2dec_t * mpeg2dec, uint32_t tag, uint32_t tag2)
{
    mpeg2dec->tag_previous = mpeg2dec->tag_current;
    mpeg2dec->tag2_previous = mpeg2dec->tag2_current;
    mpeg2dec->tag_current = tag;
    mpeg2dec->tag2_current = tag2;
    mpeg2dec->num_tags++;
    mpeg2dec->bytes_since_tag = 0;
}

uint32_t mpeg2_accel (uint32_t accel)
{
    if (!mpeg2_accels) {
	mpeg2_accels = mpeg2_detect_accel (accel) | MPEG2_ACCEL_DETECT;
	mpeg2_cpu_state_init (mpeg2_accels);
	mpeg2_idct_init (mpeg2_accels);
	mpeg2_mc_init (mpeg2_accels);
    }
    return mpeg2_accels & ~MPEG2_ACCEL_DETECT;
}

void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset)
{
    mpeg2dec->buf_start = mpeg2dec->buf_end = NULL;
    mpeg2dec->num_tags = 0;
    mpeg2dec->shift = 0xffffff00;
    mpeg2dec->code = 0xb4;
    mpeg2dec->action = mpeg2_seek_header;
    mpeg2dec->state = STATE_INVALID;
    mpeg2dec->first = 1;

    mpeg2_reset_info(&(mpeg2dec->info));
    mpeg2dec->info.gop = NULL;
    mpeg2dec->info.user_data = NULL;
    mpeg2dec->info.user_data_len = 0;
    if (full_reset) {
	mpeg2dec->info.sequence = NULL;
	mpeg2_header_state_init (mpeg2dec);
    }

}


extern int frame_num;
mpeg2dec_t * mpeg2_init (void)
{
    mpeg2dec_t * mpeg2dec;

    mpeg2_accel (MPEG2_ACCEL_DETECT);

    mpeg2dec = (mpeg2dec_t *) mpeg2_malloc (sizeof (mpeg2dec_t),
					    MPEG2_ALLOC_MPEG2DEC);
    if (mpeg2dec == NULL)
	return NULL;

    memset (mpeg2dec, 0, sizeof (mpeg2dec_t));
    memset (mpeg2dec->decoder.DCTblock, 0, 64 * sizeof (int16_t));
    memset (mpeg2dec->quantizer_matrix, 0, 4 * 64 * sizeof (uint8_t));

#ifdef MPEG2_SCH_CONTROL_n
    mpeg2dec->chunk_buffer = (uint8_t *) jz4740_alloc_frame( 4, BUFFER_SIZE + 4);
    printf("chunk_buffer = %x\n", mpeg2dec->chunk_buffer);
#else
    mpeg2dec->chunk_buffer = (uint8_t *) mpeg2_malloc (BUFFER_SIZE + 4,
						       MPEG2_ALLOC_CHUNK);
#endif
    mpeg2dec->sequence.width = (unsigned)-1;
    mpeg2_reset (mpeg2dec, 1);

#ifdef JZC_MXU_OPT
    S32I2M(xr16, 0x3);
#endif

    frame_num = 0;
#ifdef MPEG2_SCH_CONTROL
    printf("---------------- mpeg2_ACFG_config malloc ---------------\n");
    mpeg2dec->decoder.vdma_base = (uint8_t *) jz4740_alloc_frame(128, 0x2000);
    memset(mpeg2dec->decoder.vdma_base, 0, 0x2000);
    mpeg2dec->decoder.tbsbuf = (uint8_t *) jz4740_alloc_frame(4, BUFFER_SIZE + 4);
#endif

#ifdef MPEG2_CRC_CODE
    crc_code = 0;
#endif
    use_jz_buf = 1;

    return mpeg2dec;
}


void mpeg2_close (mpeg2dec_t * mpeg2dec)
{
    mpeg2_header_state_init (mpeg2dec);
    mpeg2_free (mpeg2dec->chunk_buffer);
    mpeg2_free (mpeg2dec);
}
