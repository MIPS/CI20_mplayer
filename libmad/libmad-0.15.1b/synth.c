/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: synth.c,v 1.1.1.1 2012/03/27 04:02:57 dqliu Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include "fixed.h"
# include "frame.h"
# include "synth.h"
#include "../../libjzcommon/jzmedia.h"
#include "../../libjzcommon/jzasm.h"

/*
 * NAME:	synth->init()
 * DESCRIPTION:	initialize synth struct
 */
void mad_synth_init(struct mad_synth *synth)
{
  mad_synth_mute(synth);

  synth->phase = 0;

  synth->pcm.samplerate = 0;
  synth->pcm.channels   = 0;
  synth->pcm.length     = 0;
}

/*
 * NAME:	synth->mute()
 * DESCRIPTION:	zero all polyphase filterbank values, resetting synthesis
 */
void mad_synth_mute(struct mad_synth *synth)
{
  unsigned int ch, s, v;

  for (ch = 0; ch < 2; ++ch) {
    for (s = 0; s < 16; ++s) {
      for (v = 0; v < 8; ++v) {
	synth->filter[ch][0][0][s][v] = synth->filter[ch][0][1][s][v] =
	synth->filter[ch][1][0][s][v] = synth->filter[ch][1][1][s][v] = 0;
      }
    }
  }
}

/*
 * An optional optimization called here the Subband Synthesis Optimization
 * (SSO) improves the performance of subband synthesis at the expense of
 * accuracy.
 *
 * The idea is to simplify 32x32->64-bit multiplication to 32x32->32 such
 * that extra scaling and rounding are not necessary. This often allows the
 * compiler to use faster 32-bit multiply-accumulate instructions instead of
 * explicit 64-bit multiply, shift, and add instructions.
 *
 * SSO works like this: a full 32x32->64-bit multiply of two mad_fixed_t
 * values requires the result to be right-shifted 28 bits to be properly
 * scaled to the same fixed-point format. Right shifts can be applied at any
 * time to either operand or to the result, so the optimization involves
 * careful placement of these shifts to minimize the loss of accuracy.
 *
 * First, a 14-bit shift is applied with rounding at compile-time to the D[]
 * table of coefficients for the subband synthesis window. This only loses 2
 * bits of accuracy because the lower 12 bits are always zero. A second
 * 12-bit shift occurs after the DCT calculation. This loses 12 bits of
 * accuracy. Finally, a third 2-bit shift occurs just before the sample is
 * saved in the PCM buffer. 14 + 12 + 2 == 28 bits.
 */

/* FPM_DEFAULT without OPT_SSO will actually lose accuracy and performance */

# if defined(FPM_DEFAULT) && !defined(OPT_SSO)
#  define OPT_SSO
# endif

/* second SSO shift, with rounding */

# if defined(OPT_SSO)
#  define SHIFT(x)  (((x) + (1L << 11)) >> 12)
# else
#  define SHIFT(x)  (x)
# endif

/* possible DCT speed optimization */

# if defined(OPT_SPEED) && defined(MAD_F_MLX)
#  define OPT_DCTO
#  define MUL(x, y)  \
    ({ mad_fixed64hi_t hi;  \
       mad_fixed64lo_t lo;  \
       MAD_F_MLX(hi, lo, (x), (y));  \
       hi << (32 - MAD_F_SCALEBITS - 3);  \
    })
# else
#  undef OPT_DCTO
#  define MUL(x, y)  mad_f_mul((x), (y))
# endif

/*
 * NAME:	dct32()
 * DESCRIPTION:	perform fast in[32]->out[32] DCT
 */
#if defined(JZ4750_OPT)
static
void dct32(mad_fixed_t const in[32], unsigned int slot,
           mad_fixed_t lo[16][8], mad_fixed_t hi[16][8])
{
  mad_fixed_t t0,   t1,   t2,   t3,   t4,   t5,   t6,   t7;
  mad_fixed_t t8,   t9,   t10,  t11,  t12,  t13,  t14,  t15;
  mad_fixed_t t16,  t17,  t18,  t19,  t20,  t21,  t22,  t23;
  mad_fixed_t t24,  t25,  t26,  t27,  t28,  t29,  t30,  t31;
  mad_fixed_t t32,  t33,  t34,  t35,  t36,  t37,  t38,  t39;
  mad_fixed_t t40,  t41,  t42,  t43,  t44,  t45,  t46,  t47;
  mad_fixed_t t48,  t49,  t50,  t51,  t52,  t53,  t54,  t55;
  mad_fixed_t t56,  t57,  t58,  t59,  t60,  t61,  t62,  t63;
  mad_fixed_t t64,  t65,  t66,  t67,  t68,  t69,  t70,  t71;
  mad_fixed_t t72,  t73,  t74,  t75,  t76,  t77,  t78,  t79;
  mad_fixed_t t80,  t81,  t82,  t83,  t84,  t85,  t86,  t87;
  mad_fixed_t t88,  t89,  t90,  t91,  t92,  t93,  t94,  t95;
  mad_fixed_t t96,  t97,  t98,  t99,  t100, t101, t102, t103;
  mad_fixed_t t104, t105, t106, t107, t108, t109, t110, t111;
  mad_fixed_t t112, t113, t114, t115, t116, t117, t118, t119;
  mad_fixed_t t120, t121, t122, t123, t124, t125, t126, t127;
  mad_fixed_t t128, t129, t130, t131, t132, t133, t134, t135;
  mad_fixed_t t136, t137, t138, t139, t140, t141, t142, t143;
  mad_fixed_t t144, t145, t146, t147, t148, t149, t150, t151;
  mad_fixed_t t152, t153, t154, t155, t156, t157, t158, t159;
  mad_fixed_t t160, t161, t162, t163, t164, t165, t166, t167;
  mad_fixed_t t168, t169, t170, t171, t172, t173, t174, t175;
  mad_fixed_t t176;
  int node;
  /* costab[i] = cos(PI / (2 * 32) * i) */

# if defined(OPT_DCTO)
#  define costab1       MAD_F(0x7fd8878e)
#  define costab2       MAD_F(0x7f62368f)
#  define costab3       MAD_F(0x7e9d55fc)
#  define costab4       MAD_F(0x7d8a5f40)
#  define costab5       MAD_F(0x7c29fbee)
#  define costab6       MAD_F(0x7a7d055b)
#  define costab7       MAD_F(0x78848414)
#  define costab8       MAD_F(0x7641af3d)
#  define costab9       MAD_F(0x73b5ebd1)
#  define costab10      MAD_F(0x70e2cbc6)
#  define costab11      MAD_F(0x6dca0d14)
#  define costab12      MAD_F(0x6a6d98a4)
#  define costab13      MAD_F(0x66cf8120)
#  define costab14      MAD_F(0x62f201ac)
#  define costab15      MAD_F(0x5ed77c8a)
#  define costab16      MAD_F(0x5a82799a)
#  define costab17      MAD_F(0x55f5a4d2)
#  define costab18      MAD_F(0x5133cc94)
#  define costab19      MAD_F(0x4c3fdff4)
#  define costab20      MAD_F(0x471cece7)
#  define costab21      MAD_F(0x41ce1e65)
#  define costab22      MAD_F(0x3c56ba70)
#  define costab23      MAD_F(0x36ba2014)
#  define costab24      MAD_F(0x30fbc54d)
#  define costab25      MAD_F(0x2b1f34eb)
#  define costab26      MAD_F(0x25280c5e)
#  define costab27      MAD_F(0x1f19f97b)
#  define costab28      MAD_F(0x18f8b83c)
#  define costab29      MAD_F(0x12c8106f)
#  define costab30      MAD_F(0x0c8bd35e)
#  define costab31      MAD_F(0x0647d97c)
# else
#  define costab1       MAD_F(0x0ffb10f2)  /* 0.998795456 */
#  define costab2       MAD_F(0x0fec46d2)  /* 0.995184727 */
#  define costab3       MAD_F(0x0fd3aac0)  /* 0.989176510 */
#  define costab4       MAD_F(0x0fb14be8)  /* 0.980785280 */
#  define costab5       MAD_F(0x0f853f7e)  /* 0.970031253 */
#  define costab6       MAD_F(0x0f4fa0ab)  /* 0.956940336 */
#  define costab7       MAD_F(0x0f109082)  /* 0.941544065 */
#  define costab8       MAD_F(0x0ec835e8)  /* 0.923879533 */
#  define costab9       MAD_F(0x0e76bd7a)  /* 0.903989293 */
#  define costab10      MAD_F(0x0e1c5979)  /* 0.881921264 */
#  define costab11      MAD_F(0x0db941a3)  /* 0.857728610 */
#  define costab12      MAD_F(0x0d4db315)  /* 0.831469612 */
#  define costab13      MAD_F(0x0cd9f024)  /* 0.803207531 */
#  define costab14      MAD_F(0x0c5e4036)  /* 0.773010453 */
#  define costab15      MAD_F(0x0bdaef91)  /* 0.740951125 */
#  define costab16      MAD_F(0x0b504f33)  /* 0.707106781 */
#  define costab17      MAD_F(0x0abeb49a)  /* 0.671558955 */
#  define costab18      MAD_F(0x0a267993)  /* 0.634393284 */
#  define costab19      MAD_F(0x0987fbfe)  /* 0.595699304 */
#  define costab20      MAD_F(0x08e39d9d)  /* 0.555570233 */
#  define costab21      MAD_F(0x0839c3cd)  /* 0.514102744 */
#  define costab22      MAD_F(0x078ad74e)  /* 0.471396737 */
#  define costab23      MAD_F(0x06d74402)  /* 0.427555093 */
#  define costab24      MAD_F(0x061f78aa)  /* 0.382683432 */
#  define costab25      MAD_F(0x0563e69d)  /* 0.336889853 */
#  define costab26      MAD_F(0x04a5018c)  /* 0.290284677 */
#  define costab27      MAD_F(0x03e33f2f)  /* 0.242980180 */
#  define costab28      MAD_F(0x031f1708)  /* 0.195090322 */
#  define costab29      MAD_F(0x0259020e)  /* 0.146730474 */
#  define costab30      MAD_F(0x01917a6c)  /* 0.098017140 */
#  define costab31      MAD_F(0x00c8fb30)  /* 0.049067674 */
# endif
  /* case 0
  t0   = in[0]  + in[31];  t16  = MUL(in[0]  - in[31], costab1);
  t1   = in[15] + in[16];  t17  = MUL(in[15] - in[16], costab31);

  t41  = t16 + t17;
  t59  = MUL(t16 - t17, costab2);
  t33  = t0  + t1;
  t50  = MUL(t0  - t1,  costab2);
  case 1
  t2   = in[7]  + in[24];  t18  = MUL(in[7]  - in[24], costab15);
  t3   = in[8]  + in[23];  t19  = MUL(in[8]  - in[23], costab17);

  t42  = t18 + t19;
  t60  = MUL(t18 - t19, costab30);
  t34  = t2  + t3;
  t51  = MUL(t2  - t3,  costab30);
  case 2
  t4   = in[3]  + in[28];  t20  = MUL(in[3]  - in[28], costab7);
  t5   = in[12] + in[19];  t21  = MUL(in[12] - in[19], costab25);

  t43  = t20 + t21;
  t61  = MUL(t20 - t21, costab14);
  t35  = t4  + t5;
  t52  = MUL(t4  - t5,  costab14);
  case 3
  t6   = in[4]  + in[27];  t22  = MUL(in[4]  - in[27], costab9);
  t7   = in[11] + in[20];  t23  = MUL(in[11] - in[20], costab23);

  t44  = t22 + t23;
  t62  = MUL(t22 - t23, costab18);
  t36  = t6  + t7;
  t53  = MUL(t6  - t7,  costab18);
  case 4
  t8   = in[1]  + in[30];  t24  = MUL(in[1]  - in[30], costab3);
  t9   = in[14] + in[17];  t25  = MUL(in[14] - in[17], costab29);

  t45  = t24 + t25;
  t63  = MUL(t24 - t25, costab6);
  t37  = t8  + t9;
  t54  = MUL(t8  - t9,  costab6);
  case 5
  t10  = in[6]  + in[25];  t26  = MUL(in[6]  - in[25], costab13);
  t11  = in[9]  + in[22];  t27  = MUL(in[9]  - in[22], costab19);

  t46  = t26 + t27;
  t64  = MUL(t26 - t27, costab26);
  t38  = t10 + t11;
  t55  = MUL(t10 - t11, costab26);
  case 6
  t12  = in[2]  + in[29];  t28  = MUL(in[2]  - in[29], costab5);
  t13  = in[13] + in[18];  t29  = MUL(in[13] - in[18], costab27);

  t47  = t28 + t29;
  t65  = MUL(t28 - t29, costab10);
  t39  = t12 + t13;
  t56  = MUL(t12 - t13, costab10);
  case 7
  t14  = in[5]  + in[26];  t30  = MUL(in[5]  - in[26], costab11);
  t15  = in[10] + in[21];  t31  = MUL(in[10] - in[21], costab21);

  t48  = t30 + t31;
  t66  = MUL(t30 - t31, costab22);
  t40  = t14 + t15;
  t57  = MUL(t14 - t15, costab22);
  */

  /* case 0 : */
  S32LDD(xr1,in,0);
  S32LDD(xr2,in,124);
  S32LDD(xr3,in,60);
  S32LDD(xr4,in,64);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t16 = S32M2I(xr2);
  t17 = S32M2I(xr4);
  S32MUL(xr2,xr5,t16,costab1);
  S32MUL(xr4,xr6,t17,costab31);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5); //xr2 = t16;
  S32OR(xr4,xr4,xr6); //xr4 = t17;

  D32ADD_AS(xr1,xr1,xr3,xr3);//xr1 = t33, xr3 = t0  - t1;
  D32ADD_AS(xr2,xr2,xr4,xr4);//xr2 = t41, xr4 = t16 - t17;
  t50 = S32M2I(xr3);
  t59 = S32M2I(xr4);
  S32MUL(xr3,xr5,t50,costab2);
  S32MUL(xr4,xr6,t59,costab2);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5); //xr3 = t50;
  S32OR(xr4,xr4,xr6); //xr4 = t59;
  t33 = S32M2I(xr1);
  t41 = S32M2I(xr2);
  t50 = S32M2I(xr3);
  t59 = S32M2I(xr4);

  /* case 1: */
  S32LDD(xr1,in,28);
  S32LDD(xr2,in,96);
  S32LDD(xr3,in,32);
  S32LDD(xr4,in,92);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t18 = S32M2I(xr2);
  t19 = S32M2I(xr4);
  S32MUL(xr2,xr5,t18,costab15);
  S32MUL(xr4,xr6,t19,costab17);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);

  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
  t51 = S32M2I(xr3);
  t60 = S32M2I(xr4);
  S32MUL(xr3,xr5,t51,costab30);
  S32MUL(xr4,xr6,t60,costab30);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t34 = S32M2I(xr1);
  t42 = S32M2I(xr2);
  t51 = S32M2I(xr3);
  t60 = S32M2I(xr4);
  /* case 2: */
  S32LDD(xr1,in,12);
  S32LDD(xr2,in,112);
  S32LDD(xr3,in,48);
  S32LDD(xr4,in,76);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t20 = S32M2I(xr2);
  t21 = S32M2I(xr4);
  S32MUL(xr2,xr5,t20,costab7);
  S32MUL(xr4,xr6,t21,costab25);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);

  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
  t52 = S32M2I(xr3);
  t61 = S32M2I(xr4);
  S32MUL(xr3,xr5,t52,costab14);
  S32MUL(xr4,xr6,t61,costab14);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t35 = S32M2I(xr1);
  t43 = S32M2I(xr2);
  t52 = S32M2I(xr3);
  t61 = S32M2I(xr4);
  /* case 3: */
  S32LDD(xr1,in,16);
  S32LDD(xr2,in,108);
  S32LDD(xr3,in,44);
  S32LDD(xr4,in,80);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t22 = S32M2I(xr2);
  t23 = S32M2I(xr4);
  S32MUL(xr2,xr5,t22,costab9);
  S32MUL(xr4,xr6,t23,costab23);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);

  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
  t53 = S32M2I(xr3);
  t62 = S32M2I(xr4);
  S32MUL(xr3,xr5,t53,costab18);
  S32MUL(xr4,xr6,t62,costab18);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t36 = S32M2I(xr1);
  t44 = S32M2I(xr2);
  t53 = S32M2I(xr3);
  t62 = S32M2I(xr4);
  /* case 4: */
  S32LDD(xr1,in,4);
  S32LDD(xr2,in,120);
  S32LDD(xr3,in,56);
  S32LDD(xr4,in,68);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t24 = S32M2I(xr2);
  t25 = S32M2I(xr4);
  S32MUL(xr2,xr5,t24,costab3);
  S32MUL(xr4,xr6,t25,costab29);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
 D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);

  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
  t54 = S32M2I(xr3);
  t63 = S32M2I(xr4);
  S32MUL(xr3,xr5,t54,costab6);
  S32MUL(xr4,xr6,t63,costab6);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t37 = S32M2I(xr1);
  t45 = S32M2I(xr2);
  t54 = S32M2I(xr3);
  t63 = S32M2I(xr4);
  /* case 5: */
  S32LDD(xr1,in,24);
  S32LDD(xr2,in,100);
  S32LDD(xr3,in,36);
  S32LDD(xr4,in,88);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t26 = S32M2I(xr2);
  t27 = S32M2I(xr4);
  S32MUL(xr2,xr5,t26,costab13);
  S32MUL(xr4,xr6,t27,costab19);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);

  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
  t55 = S32M2I(xr3);
  t64 = S32M2I(xr4);
  S32MUL(xr3,xr5,t55,costab26);
  S32MUL(xr4,xr6,t64,costab26);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t38 = S32M2I(xr1);
  t46 = S32M2I(xr2);
  t55 = S32M2I(xr3);
  t64 = S32M2I(xr4);
  /* case 6: */
  S32LDD(xr1,in,8);
  S32LDD(xr2,in,116);
  S32LDD(xr3,in,52);
  S32LDD(xr4,in,72);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t28 = S32M2I(xr2);
  t29 = S32M2I(xr4);
  S32MUL(xr2,xr5,t28,costab5);
  S32MUL(xr4,xr6,t29,costab27);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);
  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
  t56 = S32M2I(xr3);
  t65 = S32M2I(xr4);
  S32MUL(xr3,xr5,t56,costab10);
  S32MUL(xr4,xr6,t65,costab10);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t39 = S32M2I(xr1);
  t47 = S32M2I(xr2);
  t56 = S32M2I(xr3);
  t65 = S32M2I(xr4);
  /* case 7: */
  S32LDD(xr1,in,20);
  S32LDD(xr2,in,104);
  S32LDD(xr3,in,40);
  S32LDD(xr4,in,84);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  t30 = S32M2I(xr2);
  t31 = S32M2I(xr4);
  S32MUL(xr2,xr5,t30,costab11);
  S32MUL(xr4,xr6,t31,costab21);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);

  D32ADD_AS(xr1,xr1,xr3,xr3);
  D32ADD_AS(xr2,xr2,xr4,xr4);
 t57 = S32M2I(xr3);
  t66 = S32M2I(xr4);
  S32MUL(xr3,xr5,t57,costab22);
  S32MUL(xr4,xr6,t66,costab22);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr3,xr3,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr3,xr3,xr5);
  S32OR(xr4,xr4,xr6);
  t40 = S32M2I(xr1);
  t48 = S32M2I(xr2);
  t57 = S32M2I(xr3);
  t66 = S32M2I(xr4);

  /*
     step2-1
  t69  = t33 + t34;  t89  = MUL(t33 - t34, costab4);
  t70  = t35 + t36;  t90  = MUL(t35 - t36, costab28);
  t71  = t37 + t38;  t91  = MUL(t37 - t38, costab12);
  t72  = t39 + t40;  t92  = MUL(t39 - t40, costab20);
    step3-1
  t113 = t69  + t70;
  t114 = t71  + t72;

    0 : hi[15][slot] = SHIFT(t113 + t114);


  t125 = t89  + t90;
  t126 = t91  + t92;

  t93  = t125 + t126;

  4 :  hi[11][slot] = SHIFT(t93);
  */
  S32I2M(xr1,t33);
  S32I2M(xr2,t34);
  S32I2M(xr3,t35);
  S32I2M(xr4,t36);
  S32I2M(xr5,t37);
  S32I2M(xr6,t38);
  S32I2M(xr7,t39);
  S32I2M(xr8,t40);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  D32ADD_AS(xr5,xr5,xr6,xr6);
  D32ADD_AS(xr7,xr7,xr8,xr8);
  t89  = S32M2I(xr2);
  t90  = S32M2I(xr4);
  t91  = S32M2I(xr6);
  t92  = S32M2I(xr8);
  S32MUL(xr2,xr9,t89,costab4);
  S32MUL(xr4,xr10,t90,costab28);
  D32ADD_AS(xr1,xr1,xr3,xr3); //xr1 = t69 + t70, xr3 = t69 - t70;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr2,xr2,xr9);
  S32OR(xr4,xr4,xr10);
  S32MUL(xr6,xr9,t91,costab12);
  S32MUL(xr8,xr10,t92,costab20);
  D32ADD_AS(xr5,xr5,xr7,xr7); //xr5 = t71 + t72, xr7 = t71 - t72;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr6,xr6,xr8,xr8,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr6,xr6,xr9);
  S32OR(xr8,xr8,xr10);
  D32ADD_AS(xr2,xr2,xr4,xr4); //xr2 = t89 + t90, xr4 = t89 - t90;
  D32ADD_AS(xr6,xr6,xr8,xr8); //xr6 = t91 + t92, xr8 = t91 - t92;
  D32ADD_AS(xr1,xr1,xr5,xr5); //xr1 = t113 + t114, xr5 = t113 - t114;
  D32ADD_AS(xr15,xr2,xr6,xr6); //xr2 = t125 + t126, xr6 = t125 - t126;

  t141 = S32M2I(xr3);
  t142 = S32M2I(xr7);
  t157 = S32M2I(xr4);
  t158 = S32M2I(xr8);
  /* 0  hi[15][slot] = S32M2I(xr1); */
  S32STDV(xr1, hi[15], slot, 2);
  t113 = S32M2I(xr5);
  /* 4  hi[11][slot] = S32M2I(xr15); */
  S32STDV(xr15, hi[11], slot, 2);
  t160 = S32M2I(xr6);

  /*
    step2-2
  t73  = t41 + t42;  t94  = MUL(t41 - t42, costab4);
  t74  = t43 + t44;  t95  = MUL(t43 - t44, costab28);
  t75  = t45 + t46;  t96  = MUL(t45 - t46, costab12);
  t76  = t47 + t48;  t97  = MUL(t47 - t48, costab20);
  step3-2
  t115 = t73  + t74;
  t116 = t75  + t76;

  t32  = t115 + t116;

  1 : hi[14][slot] = SHIFT(t32);

  t128 = t94  + t95;
  t129 = t96  + t97;

  t98  = t128 + t129;
  */

  S32I2M(xr1,t41);
  S32I2M(xr2,t42);
  S32I2M(xr3,t43);
  S32I2M(xr4,t44);
  S32I2M(xr5,t45);
  S32I2M(xr6,t46);
  S32I2M(xr7,t47);
  S32I2M(xr8,t48);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  D32ADD_AS(xr5,xr5,xr6,xr6);
  D32ADD_AS(xr7,xr7,xr8,xr8);
  t94  = S32M2I(xr2);
  t95  = S32M2I(xr4);
  t96  = S32M2I(xr6);
  t97  = S32M2I(xr8);
  S32MUL(xr2,xr9,t94,costab4);
  S32MUL(xr4,xr10,t95,costab28);
  D32ADD_AS(xr1,xr1,xr3,xr3); //xr1 = t73 + t74, xr3 = t73 - t74;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr2,xr2,xr9);
  S32OR(xr4,xr4,xr10);
  S32MUL(xr6,xr9,t96,costab12);
  S32MUL(xr8,xr10,t97,costab20);
  D32ADD_AS(xr5,xr5,xr7,xr7); //xr5 = t75 + t76, xr7 = t75 - t76;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr6,xr6,xr8,xr8,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr6,xr6,xr9);
  S32OR(xr8,xr8,xr10);
  D32ADD_AS(xr2,xr2,xr4,xr4); //xr2 = t94 + t95, xr4 = t94 - t95;
  D32ADD_AS(xr6,xr6,xr8,xr8); //xr6 = t96 + t97, xr8 = t96 - t97;
  D32ADD_AS(xr13,xr1,xr5,xr5); //xr1 = t115 + t116, xr5 = t115 - t116;
  D32ADD_AS(xr14,xr2,xr6,xr6); //xr14 = t128 + t129, xr6 = t128 - t129;

  t144 = S32M2I(xr3);
  t145 = S32M2I(xr7);
  t161 = S32M2I(xr4);
  t162 = S32M2I(xr8);
  /* 1 hi[14][slot] = S32M2I(xr13); */
  S32STDV(xr13, hi[14], slot, 2);
  t124 = S32M2I(xr5);
  t164 = S32M2I(xr6);

  /*
  step2-3
  t78  = t50 + t51;  t100 = MUL(t50 - t51, costab4);
  t79  = t52 + t53;  t101 = MUL(t52 - t53, costab28);
  t80  = t54 + t55;  t102 = MUL(t54 - t55, costab12);
  t81  = t56 + t57;  t103 = MUL(t56 - t57, costab20);
  step3-3
  t118 = t78  + t79;
  t119 = t80  + t81;

  t58  = t118 + t119;

  2 :  hi[13][slot] = SHIFT(t58);

  t132 = t100 + t101;
  t133 = t102 + t103;

  t104 = t132 + t133;

  t82  = (t104 * 2) - t58;

  6 : hi[ 9][slot] = SHIFT(t82);
  */

  S32I2M(xr1,t50);
  S32I2M(xr2,t51);
  S32I2M(xr3,t52);
  S32I2M(xr4,t53);
  S32I2M(xr5,t54);
  S32I2M(xr6,t55);
  S32I2M(xr7,t56);
  S32I2M(xr8,t57);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  D32ADD_AS(xr5,xr5,xr6,xr6);
  D32ADD_AS(xr7,xr7,xr8,xr8);
  t100 = S32M2I(xr2);
  t101 = S32M2I(xr4);
  t102 = S32M2I(xr6);
  t103 = S32M2I(xr8);
  S32MUL(xr2,xr9,t100,costab4);
  S32MUL(xr4,xr10,t101,costab28);
  D32ADD_AS(xr1,xr1,xr3,xr3); //xr1 = t78 + t79, xr3 = t78 - t79;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr2,xr2,xr9);
  S32OR(xr4,xr4,xr10);
  S32MUL(xr6,xr9,t102,costab12);
  S32MUL(xr8,xr10,t103,costab20);
  D32ADD_AS(xr5,xr5,xr7,xr7); //xr5 = t80 + t81, xr7 = t80 - t81;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr6,xr6,xr8,xr8,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr6,xr6,xr9);
  S32OR(xr8,xr8,xr10);
 D32ADD_AS(xr2,xr2,xr4,xr4); //xr2 = t100 + t101, xr4 = t100 - t101;
  D32ADD_AS(xr6,xr6,xr8,xr8); //xr6 = t102 + t103, xr8 = t102 - t103;
  D32ADD_AS(xr11,xr1,xr5,xr5); //xr11 = t118 + t119, xr5 = t118 - t119;
  D32ADD_AS(xr12,xr2,xr6,xr6); //xr12 = t132 + t133, xr6 = t132 - t133;

  t148 = S32M2I(xr3);
  t149 = S32M2I(xr7);
  t166 = S32M2I(xr4);
  t167 = S32M2I(xr8);
  /* 2  hi[13][slot] = S32M2I(xr11); */
  S32STDV(xr11, hi[13], slot, 2);
  t135 = S32M2I(xr5);
  t169 = S32M2I(xr6);

  /*
  step2-4
  t83  = t59 + t60;  t106 = MUL(t59 - t60, costab4);
  t84  = t61 + t62;  t107 = MUL(t61 - t62, costab28);
  t85  = t63 + t64;  t108 = MUL(t63 - t64, costab12);
  t86  = t65 + t66;  t109 = MUL(t65 - t66, costab20);
  step3-4
  t121 = t83  + t84;
  t122 = t85  + t86;

  t67  = t121 + t122;

  t49  = (t67 * 2) - t32;

  3 : hi[12][slot] = SHIFT(t49);

   step 4-1
  t68  = (t98 * 2) - t49;

   5 : hi[10][slot] = SHIFT(t68);


  t136 = t106 + t107;
  t137 = t108 + t109;

  t110 = t136 + t137;

  t87  = (t110 * 2) - t67;

  t77  = (t87 * 2) - t68;

  7 : hi[ 8][slot] = SHIFT(t77);
  */

  S32I2M(xr1,t59);
  S32I2M(xr2,t60);
  S32I2M(xr3,t61);
  S32I2M(xr4,t62);
  S32I2M(xr5,t63);
  S32I2M(xr6,t64);
  S32I2M(xr7,t65);
  S32I2M(xr8,t66);
  D32ADD_AS(xr1,xr1,xr2,xr2);
  D32ADD_AS(xr3,xr3,xr4,xr4);
  D32ADD_AS(xr5,xr5,xr6,xr6);
  D32ADD_AS(xr7,xr7,xr8,xr8);
  t106 = S32M2I(xr2);
  t107 = S32M2I(xr4);
  t108 = S32M2I(xr6);
  t109 = S32M2I(xr8);
  S32MUL(xr2,xr9, t106,costab4);
  S32MUL(xr4,xr10,t107,costab28);
  D32ADD_AS(xr1,xr1,xr3,xr3); //xr1 = t83  + t84, xr3 = t83  - t84;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr2,xr2,xr9);
  S32OR(xr4,xr4,xr10);
  S32MUL(xr6,xr9,t108,costab12);
  S32MUL(xr8,xr10,t109,costab20);
  D32ADD_AS(xr5,xr5,xr7,xr7); //xr5 = t85  + t86, xr7 = t85  - t86;
  D32SLR(xr9,xr9,xr10,xr10,14);
  D32SLL(xr6,xr6,xr8,xr8,4);
  D32SLR(xr9,xr9,xr10,xr10,14);
  S32OR(xr6,xr6,xr9);
  S32OR(xr8,xr8,xr10);
  D32ADD_AS(xr2,xr2,xr4,xr4); //xr2 = t106 + t107, xr4 = t106 - t107;
  D32ADD_AS(xr6,xr6,xr8,xr8); //xr6 = t108 + t109, xr8 = t108 - t109;
  D32ADD_AS(xr1,xr1,xr5,xr5); //xr1 = t121 + t122, xr5 = t121 - t122;
  D32ADD_AS(xr2,xr2,xr6,xr6); //xr2 = t136 + t137, xr6 = t136 - t137;

  D32SLL(xr10,xr12,xr1,xr9,1);
  t152 = S32M2I(xr3);
  t153 = S32M2I(xr7);
  D32ASUM_SS(xr10,xr11,xr13,xr9);
  t171 = S32M2I(xr4);
  t172 = S32M2I(xr8);
  t67 = S32M2I(xr1);
  D32SLL(xr8,xr14,xr2,xr7,1);
  t139 = S32M2I(xr5);
  D32ASUM_SS(xr8,xr9,xr1,xr7); //xr8 = t68, xr7 = t87;


  t110 = S32M2I(xr2);
  t174 = S32M2I(xr6);

  /*
  16 : lo[ 0][slot] = SHIFT(MUL(t113 , costab16));

  t77  = (t87 * 2) - t68;
  7 : hi[ 8][slot] = SHIFT(t77);

  t141 = MUL(t141, costab8);
  t142 = MUL(t142, costab24);
  t143 = t141 + t142;

  8 : hi[ 7][slot] = SHIFT(t143);
  24 : lo[ 8][slot] =
             SHIFT((MUL(t141 - t142, costab16) * 2) - t143);
  */

  S32MUL(xr1,xr2, t141, costab8);
  S32MUL(xr3,xr4, t142, costab24);
  /*  6  hi[ 9][slot] = S32M2I(xr10); */
  S32STDV(xr10, hi[9], slot, 2);
  D32SLR(xr2,xr2,xr4,xr4,14);
  D32SLL(xr1,xr1,xr3,xr3,4);
  D32SLR(xr2,xr2,xr4,xr4,14);
  S32OR(xr1,xr1,xr2);
  S32OR(xr3,xr3,xr4);
  D32ADD_AS(xr1,xr1,xr3,xr3); //xr1 = t141 + t142, xr3 = t141 - t142;
  /*  5 hi[10][slot] = S32M2I(xr8); */
  S32STDV(xr8, hi[10], slot, 2);
  t141 = S32M2I(xr3);
  S32MUL(xr2,xr5, t113, costab16);
  S32MUL(xr4,xr6, t141, costab16);
  /*  3  hi[12][slot] = S32M2I(xr9);*/
  S32STDV(xr9, hi[12], slot, 2);
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);
  D32SLL(xr9,xr7,xr4,xr4,1); /* xr9 = t87*2 , xr4 = (MUL(t141 - t142, costab16) * 2); */
  /* 16  lo[ 0][slot] = S32M2I(xr2); */
  S32STDV(xr2, lo[0], slot, 2);
  D32ASUM_SS(xr9,xr8,xr1,xr4); /* xr9 = t77, xr4 = lo[ 8][slot]*/

  /*  8  hi[ 7][slot] = S32M2I(xr1); */
  S32STDV(xr1, hi[7], slot, 2);
  /* 24  lo[ 8][slot] = S32M2I(xr4); */
  S32STDV(xr4, lo[8], slot, 2);

  /*
  t144 = MUL(t144, costab8);
  t145 = MUL(t145, costab24);
  t146 = t144 + t145;

  t88  = (t146 * 2) - t77;

  9 : hi[ 6][slot] = SHIFT(t88);

  t148 = MUL(t148, costab8);
  t149 = MUL(t149, costab24);
  t150 = t148 + t149;

  t105 = (t150 * 2) - t82;

  10 : hi[ 5][slot] = SHIFT(t105);
  */
  S32MUL(xr1,xr2, t144, costab8);
  S32MUL(xr3,xr4, t145, costab24);
  /*  7  hi[ 8][slot] = S32M2I(xr9); */
  S32STDV(xr9, hi[8], slot, 2);
  D32SLR(xr2,xr2,xr4,xr4,14);
  D32SLL(xr1,xr1,xr3,xr3,4);
  D32SLR(xr2,xr2,xr4,xr4,14);
  S32OR(xr1,xr1,xr2);
  S32OR(xr3,xr3,xr4);

  S32MUL(xr2,xr5, t148, costab8);
  S32MUL(xr4,xr6, t149, costab24);
  D32ADD_AS(xr1,xr1,xr3,xr3); //xr1 = t144 + t145, xr3 = t144 - t145;
  D32SLR(xr5,xr5,xr6,xr6,14);
  D32SLL(xr2,xr2,xr4,xr4,4);
  D32SLR(xr5,xr5,xr6,xr6,14);
  S32OR(xr2,xr2,xr5);
  S32OR(xr4,xr4,xr6);
  D32ADD_AS(xr2,xr2,xr4,xr4); //xr2 = t148 + t149, xr4 = t148 - t149;
  t144 = S32M2I(xr3);
  t148 = S32M2I(xr4);
  D32SLL(xr3,xr1,xr2,xr4,1); /* xr3 = t146*2 , xr4 = t150 * 2; */
  D32ASUM_SS(xr3,xr9,xr10,xr4); /* xr3 = t88, xr4 = t105*/

  /*
  t152 = MUL(t152, costab8);
  t153 = MUL(t153, costab24);
  t154 = t152 + t153;

  t111 = (t154 * 2) - t87;

  t99  = (t111 * 2) - t88;

  11 : hi[ 4][slot] = SHIFT(t99);

  t157 = MUL(t157, costab8);
  t158 = MUL(t158, costab24);
  t159 = t157 + t158;

  t127 = (t159 * 2) - t93;

  12 : hi[ 3][slot] = SHIFT(t127);

    t160 = (MUL(t125 - t126, costab16) * 2) - t127;
  t160 = (MUL(t160, costab16) * 2) - t127;

  20  : lo[ 4][slot] = SHIFT(t160);
   28 : lo[12][slot] =
             SHIFT((((MUL(t157 - t158, costab16) * 2) - t159) * 2) - t160);
  */

  S32MUL(xr5,xr6, t152, costab8);
  S32MUL(xr8,xr9, t153, costab24);
  /*  9  hi[ 6][slot] = S32M2I(xr3); */
  S32STDV(xr3, hi[6], slot, 2);
  D32SLR(xr6,xr6,xr9,xr9,14);
  D32SLL(xr5,xr5,xr8,xr8,4);
  D32SLR(xr6,xr6,xr9,xr9,14);
  S32OR(xr5,xr5,xr6);
  S32OR(xr8,xr8,xr9);

  S32MUL(xr6,xr10, t157, costab8);
  S32MUL(xr9,xr11, t158, costab24);
  D32ADD_AS(xr5,xr5,xr8,xr8); //xr5 = t152 + t153, xr8 = t152 - t153;
  /* 10  hi[ 5][slot] = S32M2I(xr4); */
  S32STDV(xr4, hi[5], slot, 2);
  t152 = S32M2I(xr8);
  D32SLR(xr10,xr10,xr11,xr11,14);
  D32SLL(xr6,xr6,xr9,xr9,4);
  D32SLR(xr10,xr10,xr11,xr11,14);
  S32OR(xr6,xr6,xr10);
  S32OR(xr9,xr9,xr11);
  D32ADD_AS(xr6,xr6,xr9,xr9); //xr6 = t157 + t158, xr9 = t157 - t158;
  t157 = S32M2I(xr9);
  D32SLL(xr8,xr5,xr6,xr9,1); /* xr8 = t154 * 2 , xr9 = t159 * 2; */
  D32ASUM_SS(xr8,xr7,xr15,xr9); /* xr8 = t111, xr9 = t127*/

  S32MUL(xr13,xr10, t160, costab16);
  S32MUL(xr15,xr11, t157, costab16);
  /* 12  hi[ 3][slot] = S32M2I(xr9);  */
  S32STDV(xr9, hi[3], slot, 2);
  D32SLR(xr10,xr10,xr11,xr11,14);
  D32SLL(xr13,xr13,xr15,xr15,4);
  D32SLR(xr10,xr10,xr11,xr11,14);
  S32OR(xr13,xr13,xr10);
  S32OR(xr15,xr15,xr11);
  D32SLL(xr13,xr13,xr15,xr15,1); /* xr13 = (MUL(t160, costab16) * 2) , xr15 = (MUL(t157, costab16) * 2); */
  D32ASUM_SS(xr13,xr9,xr6,xr15); /* xr13 = t160, xr15 = ((MUL(t157, costab16) * 2) - t159)*/

  D32SLL(xr11,xr8,xr15,xr15,1); /* xr11 = t111 * 2 , xr15 = ((MUL(t157, costab16) * 2) - t159) * 2; */
  D32ASUM_SS(xr11,xr3,xr13,xr15); /* xr11 = t99, xr15 = lo[12][slot] */

  /*
  t161 = MUL(t161, costab8);
  t162 = MUL(t162, costab24);
  t163 = t161 + t162;

  t130 = (t163 * 2) - t98;

  t112 = (t130 * 2) - t99;

   13  : hi[ 2][slot] = SHIFT(t112);

  t164 = (MUL(t164, costab16) * 2) - t130;

  t166 = MUL(t166, costab8);
  t167 = MUL(t167, costab24);
 t168 = t166 + t167;

  t134 = (t168 * 2) - t104;

  t120 = (t134 * 2) - t105;

   14  : hi[ 1][slot] = SHIFT(t120);

  t135 = (MUL(t135, costab16) * 2) - t120;

   18 : lo[ 2][slot] = SHIFT(t135);
  */

  S32MUL(xr3,xr6, t161, costab8);
  S32MUL(xr7,xr9, t162, costab24);
  /* 20  lo[ 4][slot] = S32M2I(xr13); */
  S32STDV(xr13, lo[4], slot, 2);
  /* 28  lo[12][slot] = S32M2I(xr15); */
  S32STDV(xr15, lo[12], slot, 2);
  D32SLR(xr6,xr6,xr9,xr9,14);
  D32SLL(xr3,xr3,xr7,xr7,4);
  D32SLR(xr6,xr6,xr9,xr9,14);
  S32OR(xr3,xr3,xr6);
  S32OR(xr7,xr7,xr9);

  S32MUL(xr6,xr10, t166, costab8);
  S32MUL(xr9,xr13, t167, costab24);
  D32ADD_AS(xr3,xr3,xr7,xr7); //xr3 = t161 + t162, xr7 = t161 - t162;
  D32SLR(xr10,xr10,xr13,xr13,14);
  D32SLL(xr6,xr6,xr9,xr9,4);
  D32SLR(xr10,xr10,xr13,xr13,14);
  S32OR(xr6,xr6,xr10);
  S32OR(xr9,xr9,xr13);
  D32ADD_AS(xr6,xr6,xr9,xr9); //xr6 = t166 + t167, xr9 = t166 - t167;
  /* 11  hi[ 4][slot] = S32M2I(xr11); */
  S32STDV(xr11, hi[4], slot, 2);
  t161 = S32M2I(xr7);

  D32SLL(xr10,xr3,xr6,xr13,1); /* xr10 = t163 * 2 , xr13 = t168 * 2; */
  D32ASUM_SS(xr10,xr14,xr12,xr13); /* xr10 = t130, xr13 = t134; */
  t166 = S32M2I(xr9);
  D32SLL(xr12,xr10,xr13,xr14,1); /* xr12 = t130 * 2 , xr14 = t134 * 2; */
  D32ASUM_SS(xr12,xr11,xr4,xr14); /* xr12 = t112, xr14 = t120; */

  S32MUL(xr4,xr7, t164, costab16);
  S32MUL(xr9,xr11, t135, costab16);
  /* 13  hi[ 2][slot] = S32M2I(xr12); */
  S32STDV(xr12, hi[2], slot, 2);
  /* 14  hi[ 1][slot] = S32M2I(xr14); */
  S32STDV(xr14, hi[1], slot, 2);
  D32SLR(xr7,xr7,xr11,xr11,14);
  D32SLL(xr4,xr4,xr9,xr9,4);
  D32SLR(xr7,xr7,xr11,xr11,14);
  S32OR(xr4,xr4,xr7);
  S32OR(xr9,xr9,xr11);
  D32SLL(xr4,xr4,xr9,xr9,1); /* xr4 = (MUL(t164, costab16) * 2) , xr9 = (MUL(t135, costab16) * 2); */
  D32ASUM_SS(xr4,xr10,xr14,xr9); /* xr4 = t164, xr9 = t135; */

  /*
  t169 = (MUL(t169, costab16) * 2) - t134;

  t151 = (t169 * 2) - t135;

  22 : lo[ 6][slot] = SHIFT(t151);

  t170 = (((MUL(t148, costab16) * 2) - t150) * 2) - t151;

  26 : lo[10][slot] = SHIFT(t170);

  t171 = MUL(t171, costab8);
  t172 = MUL(t172, costab24);
  t173 = t171 + t172;

  t138 = (t173 * 2) - t110;

  t123 = (t138 * 2) - t111;
  */

  S32MUL(xr7,xr10, t169, costab16);
  S32MUL(xr11,xr14, t148, costab16);
  /* 18 lo[ 2][slot] = S32M2I(xr9); */
  S32STDV(xr9, lo[2], slot, 2);
  D32SLR(xr10,xr10,xr14,xr14,14);
  D32SLL(xr7,xr7,xr11,xr11,4);
  D32SLR(xr10,xr10,xr14,xr14,14);
  S32OR(xr7,xr7,xr10);
  S32OR(xr11,xr11,xr14);
  D32SLL(xr7,xr7,xr11,xr11,1); /* xr7 = (MUL(t169, costab16) * 2) , xr11 = (MUL(t148, costab16) * 2); */
  D32ASUM_SS(xr7,xr13,xr2,xr11); /* xr7 = t169, xr11 = ((MUL(t148, costab16) * 2) - t150); */

  S32MUL(xr9,xr10, t171, costab8);
  S32MUL(xr13,xr14, t172, costab24);
  t135 =  lo[ 2][slot];
  D32SLR(xr10,xr10,xr14,xr14,14);
  D32SLL(xr9,xr9,xr13,xr13,4);
  D32SLR(xr10,xr10,xr14,xr14,14);
  S32OR(xr9,xr9,xr10);
  S32OR(xr13,xr13,xr14);
  S32I2M(xr14,t110);
  D32ADD_AS(xr9,xr9,xr13,xr13); //xr9 = t171 + t172, xr13 = t171 - t172;
  t171 = S32M2I(xr13);
  D32SLL(xr2,xr7,xr9,xr15,1); /* xr2 = t169 * 2 , xr15 = t173 * 2; */
  S32I2M(xr10,t135);
  D32ASUM_SS(xr2,xr10,xr14,xr15); /* xr2 = t151, xr15 = t138; */
  D32SLL(xr11,xr11,xr15,xr13,1); /* xr11 = (((MUL(t148, costab16) * 2) - t150) * 2) , xr13 = t138 * 2; */
  D32ASUM_SS(xr11,xr2,xr8,xr13); /* xr11 = t170, xr13 = t123; */
  /* 22  lo[ 6][slot] = S32M2I(xr2); */
  S32STDV(xr2, lo[6], slot, 2);
  /*
  30 : lo[14][slot] =
             SHIFT((((((MUL(t166, costab16) * 2) -
                       t168) * 2) - t169) * 2) - t170);

  t139 = (MUL(t139, costab16) * 2) - t123;

  t117 = (t123 * 2) - t112;

  15 : hi[ 0][slot] = SHIFT(t117);

  t124 = (MUL(t124, costab16) * 2) - t117;

  17 : lo[ 1][slot] = SHIFT(t124);

  t131 = (t139 * 2) - t124;

  19 : lo[ 3][slot] = SHIFT(t131);

  t174 = (MUL(t174, costab16) * 2) - t138;
  */
  S32MUL(xr2,xr10, t166, costab16);
  S32MUL(xr8,xr14, t139, costab16);
  /* 26  lo[10][slot] = S32M2I(xr11); */
  S32STDV(xr11, lo[10], slot, 2);
  D32SLR(xr10,xr10,xr14,xr14,14);
  D32SLL(xr2,xr2,xr8,xr8,4);
  D32SLR(xr10,xr10,xr14,xr14,14);
  S32OR(xr2,xr2,xr10);
  S32OR(xr8,xr8,xr14);
  D32SLL(xr2,xr2,xr8,xr8,1); /* xr2 = (MUL(t166, costab16) * 2) , xr8 = (MUL(t139, costab16) * 2); */
  D32ASUM_SS(xr2,xr6,xr13,xr8); /* xr2 = ((MUL(t166, costab16) * 2) - t168), xr8 = t139; */

  D32SLL(xr2,xr2,xr13,xr13,1); /* xr2 = ((MUL(t166, costab16) * 2) - t168) * 2, xr13 = t123 * 2; */
  D32ASUM_SS(xr2,xr7,xr12,xr13); /* xr2 = ((((MUL(t166, costab16) * 2) -
                                             t168) * 2) - t169), xr13 = t117; */
  S32MUL(xr6,xr10, t124, costab16);
  S32MUL(xr7,xr14, t174, costab16);
  D32SLR(xr10,xr10,xr14,xr14,14);
  D32SLL(xr6,xr6,xr7,xr7,4);
  D32SLR(xr10,xr10,xr14,xr14,14);
  S32OR(xr6,xr6,xr10);
  S32OR(xr7,xr7,xr14);
  D32SLL(xr2,xr2,xr6,xr6,1); /* xr2 = ((((MUL(t166, costab16) * 2) -
                                             t168) * 2) - t169) * 2, xr6 = (MUL(t124, costab16) * 2); */
  D32ASUM_SS(xr2,xr11,xr13,xr6);  /* ((((((MUL(t166, costab16) * 2) -
                                           t168) * 2) - t169) * 2) - t170), xr6 = t124; */
  /* 15  hi[ 0][slot] = S32M2I(xr13); */
  S32STDV(xr13, hi[0], slot, 2);
  /* 17  lo[ 1][slot] = S32M2I(xr6); */
  S32STDV(xr6, lo[1], slot, 2);
  /* 30  lo[14][slot] = S32M2I(xr2); */
  S32STDV(xr2, lo[14], slot, 2);

  D32SLL(xr7,xr7,xr8,xr12,1); /* xr7 = (MUL(t174, costab16) * 2), xr12 = t139 * 2; */
  D32ASUM_SS(xr7,xr15,xr6,xr12); /* xr7 = (MUL(t174, costab16) * 2) - t138, xr12 = t131; */

  /*
  t140 = (t164 * 2) - t131;

  21 : lo[ 5][slot] = SHIFT(t140);


  t155 = (t174 * 2) - t139;

  t147 = (t155 * 2) - t140;

  23 : lo[ 7][slot] = SHIFT(t147);

  t156 = (((MUL(t144, costab16) * 2) - t146) * 2) - t147;

  25 : lo[ 9][slot] = SHIFT(t156);

  t175 = (((MUL(t152, costab16) * 2) - t154) * 2) - t155;

  t165 = (t175 * 2) - t156;

  27 : lo[11][slot] = SHIFT(t165);
  t176 = (((((MUL(t161, costab16) * 2) -
             t163) * 2) - t164) * 2) - t165;

  29 : lo[13][slot] = SHIFT(t176);
   31 : lo[15][slot] =
             SHIFT((((((((MUL(t171, costab16) * 2) -
                         t173) * 2) - t174) * 2) - t175) * 2) - t176);
  */

  S32MUL(xr2,xr10, t144, costab16);
  S32MUL(xr6,xr11, t152, costab16);
  /* 19  lo[ 3][slot] = S32M2I(xr12); */
  S32STDV(xr12, lo[3], slot, 2);
  D32SLR(xr10,xr10,xr11,xr11,14);
  D32SLL(xr2,xr2,xr6,xr6,4);
  D32SLR(xr10,xr10,xr11,xr11,14);
  S32OR(xr2,xr2,xr10);
  S32OR(xr6,xr6,xr11);
  D32SLL(xr2,xr2,xr6,xr6,1); /* xr2 = (MUL(t144, costab16) * 2) , xr6 = (MUL(t152, costab16) * 2); */
  D32ASUM_SS(xr2,xr1,xr5,xr6); /* xr2 = ((MUL(t144, costab16) * 2) - t146),
                                  xr6 = ((MUL(t152, costab16) * 2) - t154); */

  D32SLL(xr13,xr4,xr7,xr14,1); /* xr13 = t164 * 2 , xr14 = t174 * 2; */
  D32ASUM_SS(xr13,xr12,xr8,xr14); /* xr13 = t140 , xr14 = t155; */

  D32SLL(xr6,xr6,xr14,xr15,1); /* xr6 = (((MUL(t152, costab16) * 2) - t154) * 2) , xr15 = t155 * 2; */
  D32ASUM_SS(xr6,xr14,xr13,xr15); /* xr6 = t175 , xr15 = t147; */
  /* 21  lo[ 5][slot] = S32M2I(xr13); */
  S32STDV(xr13, lo[5], slot, 2);

  D32SLL(xr2,xr2,xr6,xr5,1); /* xr2 = (((MUL(t144, costab16) * 2) - t146) * 2) , xr5 = t175 * 2; */

  S32MUL(xr8,xr10, t161, costab16);
  S32MUL(xr11,xr12, t171, costab16);
  /* 23  lo[ 7][slot] = S32M2I(xr15); */
  S32STDV(xr15, lo[7], slot, 2);
  D32SLR(xr10,xr10,xr12,xr12,14);
  D32SLL(xr8,xr8,xr11,xr11,4);
  D32SLR(xr10,xr10,xr12,xr12,14);
  S32OR(xr8,xr8,xr10);
  S32OR(xr11,xr11,xr12);
  D32SLL(xr8,xr8,xr11,xr11,1); /* xr8 = (MUL(t161, costab16) * 2) , xr11 = (MUL(t171, costab16) * 2); */
  D32ASUM_SS(xr8,xr3,xr15,xr2); /* xr8 = ((MUL(t161, costab16) * 2) - t163), xr2 = t156*/

  D32ASUM_SS(xr11,xr9,xr2,xr5); /* xr11 = ((MUL(t171, costab16) * 2) - t173) , xr5 = t165; */
  D32SLL(xr8,xr8,xr11,xr11,1); /* xr8 = ((MUL(t161, costab16) * 2) - t163) * 2 ,
                                  xr11 = (((MUL(t171, costab16) * 2) - t173) * 2) ; */
  D32ASUM_SS(xr8,xr4,xr7,xr11); /* xr8 = ((((MUL(t161, costab16) * 2) - t163) * 2) - t164),
                                    xr11 = (((MUL(t171, costab16) * 2) - t173) * 2) - t174 */
  /* 25  lo[ 9][slot] = S32M2I(xr2); */
  S32STDV(xr2, lo[9], slot, 2);
  D32SLL(xr8,xr8,xr11,xr11,1); /* xr8 = (((((MUL(t161, costab16) * 2) - t163) * 2) - t164) * 2),
                                    xr11 = (((((MUL(t171, costab16) * 2) - t173) * 2) - t174 ) * 2) */
  D32ASUM_SS(xr8,xr5,xr6,xr11); /* xr8 = t176,
                                    xr11 = (((((MUL(t171, costab16) * 2) - t173) * 2) - t174 ) * 2) - t175*/
  /* 27  lo[11][slot] = S32M2I(xr5); */
  S32STDV(xr5, lo[11], slot, 2);
  D32SLL(xr0,xr0,xr11,xr11,1);
  D32ASUM_SS(xr0,xr0,xr8,xr11); /* xr11 = lo[15][slot] */


  /* 29  lo[13][slot] = S32M2I(xr8); */
  S32STDV(xr8, lo[13], slot, 2);
  /* 31  lo[15][slot] = S32M2I(xr11); */
  S32STDV(xr11, lo[15], slot, 2);
  /*
   * Totals:
   *  80 multiplies
   *  80 additions
   * 119 subtractions
   *  49 shifts (not counting SSO)
   */

}

#else //JZ4750_OPT

static
void dct32(mad_fixed_t const in[32], unsigned int slot,
	   mad_fixed_t lo[16][8], mad_fixed_t hi[16][8])
{
  mad_fixed_t t0,   t1,   t2,   t3,   t4,   t5,   t6,   t7;
  mad_fixed_t t8,   t9,   t10,  t11,  t12,  t13,  t14,  t15;
  mad_fixed_t t16,  t17,  t18,  t19,  t20,  t21,  t22,  t23;
  mad_fixed_t t24,  t25,  t26,  t27,  t28,  t29,  t30,  t31;
  mad_fixed_t t32,  t33,  t34,  t35,  t36,  t37,  t38,  t39;
  mad_fixed_t t40,  t41,  t42,  t43,  t44,  t45,  t46,  t47;
  mad_fixed_t t48,  t49,  t50,  t51,  t52,  t53,  t54,  t55;
  mad_fixed_t t56,  t57,  t58,  t59,  t60,  t61,  t62,  t63;
  mad_fixed_t t64,  t65,  t66,  t67,  t68,  t69,  t70,  t71;
  mad_fixed_t t72,  t73,  t74,  t75,  t76,  t77,  t78,  t79;
  mad_fixed_t t80,  t81,  t82,  t83,  t84,  t85,  t86,  t87;
  mad_fixed_t t88,  t89,  t90,  t91,  t92,  t93,  t94,  t95;
  mad_fixed_t t96,  t97,  t98,  t99,  t100, t101, t102, t103;
  mad_fixed_t t104, t105, t106, t107, t108, t109, t110, t111;
  mad_fixed_t t112, t113, t114, t115, t116, t117, t118, t119;
  mad_fixed_t t120, t121, t122, t123, t124, t125, t126, t127;
  mad_fixed_t t128, t129, t130, t131, t132, t133, t134, t135;
  mad_fixed_t t136, t137, t138, t139, t140, t141, t142, t143;
  mad_fixed_t t144, t145, t146, t147, t148, t149, t150, t151;
  mad_fixed_t t152, t153, t154, t155, t156, t157, t158, t159;
  mad_fixed_t t160, t161, t162, t163, t164, t165, t166, t167;
  mad_fixed_t t168, t169, t170, t171, t172, t173, t174, t175;
  mad_fixed_t t176;

  /* costab[i] = cos(PI / (2 * 32) * i) */

# if defined(OPT_DCTO)
#  define costab1	MAD_F(0x7fd8878e)
#  define costab2	MAD_F(0x7f62368f)
#  define costab3	MAD_F(0x7e9d55fc)
#  define costab4	MAD_F(0x7d8a5f40)
#  define costab5	MAD_F(0x7c29fbee)
#  define costab6	MAD_F(0x7a7d055b)
#  define costab7	MAD_F(0x78848414)
#  define costab8	MAD_F(0x7641af3d)
#  define costab9	MAD_F(0x73b5ebd1)
#  define costab10	MAD_F(0x70e2cbc6)
#  define costab11	MAD_F(0x6dca0d14)
#  define costab12	MAD_F(0x6a6d98a4)
#  define costab13	MAD_F(0x66cf8120)
#  define costab14	MAD_F(0x62f201ac)
#  define costab15	MAD_F(0x5ed77c8a)
#  define costab16	MAD_F(0x5a82799a)
#  define costab17	MAD_F(0x55f5a4d2)
#  define costab18	MAD_F(0x5133cc94)
#  define costab19	MAD_F(0x4c3fdff4)
#  define costab20	MAD_F(0x471cece7)
#  define costab21	MAD_F(0x41ce1e65)
#  define costab22	MAD_F(0x3c56ba70)
#  define costab23	MAD_F(0x36ba2014)
#  define costab24	MAD_F(0x30fbc54d)
#  define costab25	MAD_F(0x2b1f34eb)
#  define costab26	MAD_F(0x25280c5e)
#  define costab27	MAD_F(0x1f19f97b)
#  define costab28	MAD_F(0x18f8b83c)
#  define costab29	MAD_F(0x12c8106f)
#  define costab30	MAD_F(0x0c8bd35e)
#  define costab31	MAD_F(0x0647d97c)
# else
#  define costab1	MAD_F(0x0ffb10f2)  /* 0.998795456 */
#  define costab2	MAD_F(0x0fec46d2)  /* 0.995184727 */
#  define costab3	MAD_F(0x0fd3aac0)  /* 0.989176510 */
#  define costab4	MAD_F(0x0fb14be8)  /* 0.980785280 */
#  define costab5	MAD_F(0x0f853f7e)  /* 0.970031253 */
#  define costab6	MAD_F(0x0f4fa0ab)  /* 0.956940336 */
#  define costab7	MAD_F(0x0f109082)  /* 0.941544065 */
#  define costab8	MAD_F(0x0ec835e8)  /* 0.923879533 */
#  define costab9	MAD_F(0x0e76bd7a)  /* 0.903989293 */
#  define costab10	MAD_F(0x0e1c5979)  /* 0.881921264 */
#  define costab11	MAD_F(0x0db941a3)  /* 0.857728610 */
#  define costab12	MAD_F(0x0d4db315)  /* 0.831469612 */
#  define costab13	MAD_F(0x0cd9f024)  /* 0.803207531 */
#  define costab14	MAD_F(0x0c5e4036)  /* 0.773010453 */
#  define costab15	MAD_F(0x0bdaef91)  /* 0.740951125 */
#  define costab16	MAD_F(0x0b504f33)  /* 0.707106781 */
#  define costab17	MAD_F(0x0abeb49a)  /* 0.671558955 */
#  define costab18	MAD_F(0x0a267993)  /* 0.634393284 */
#  define costab19	MAD_F(0x0987fbfe)  /* 0.595699304 */
#  define costab20	MAD_F(0x08e39d9d)  /* 0.555570233 */
#  define costab21	MAD_F(0x0839c3cd)  /* 0.514102744 */
#  define costab22	MAD_F(0x078ad74e)  /* 0.471396737 */
#  define costab23	MAD_F(0x06d74402)  /* 0.427555093 */
#  define costab24	MAD_F(0x061f78aa)  /* 0.382683432 */
#  define costab25	MAD_F(0x0563e69d)  /* 0.336889853 */
#  define costab26	MAD_F(0x04a5018c)  /* 0.290284677 */
#  define costab27	MAD_F(0x03e33f2f)  /* 0.242980180 */
#  define costab28	MAD_F(0x031f1708)  /* 0.195090322 */
#  define costab29	MAD_F(0x0259020e)  /* 0.146730474 */
#  define costab30	MAD_F(0x01917a6c)  /* 0.098017140 */
#  define costab31	MAD_F(0x00c8fb30)  /* 0.049067674 */
# endif

  t0   = in[0]  + in[31];  t16  = MUL(in[0]  - in[31], costab1);
  t1   = in[15] + in[16];  t17  = MUL(in[15] - in[16], costab31);

  t41  = t16 + t17;
  t59  = MUL(t16 - t17, costab2);
  t33  = t0  + t1;
  t50  = MUL(t0  - t1,  costab2);

  t2   = in[7]  + in[24];  t18  = MUL(in[7]  - in[24], costab15);
  t3   = in[8]  + in[23];  t19  = MUL(in[8]  - in[23], costab17);

  t42  = t18 + t19;
  t60  = MUL(t18 - t19, costab30);
  t34  = t2  + t3;
  t51  = MUL(t2  - t3,  costab30);

  t4   = in[3]  + in[28];  t20  = MUL(in[3]  - in[28], costab7);
  t5   = in[12] + in[19];  t21  = MUL(in[12] - in[19], costab25);

  t43  = t20 + t21;
  t61  = MUL(t20 - t21, costab14);
  t35  = t4  + t5;
  t52  = MUL(t4  - t5,  costab14);

  t6   = in[4]  + in[27];  t22  = MUL(in[4]  - in[27], costab9);
  t7   = in[11] + in[20];  t23  = MUL(in[11] - in[20], costab23);

  t44  = t22 + t23;
  t62  = MUL(t22 - t23, costab18);
  t36  = t6  + t7;
  t53  = MUL(t6  - t7,  costab18);

  t8   = in[1]  + in[30];  t24  = MUL(in[1]  - in[30], costab3);
  t9   = in[14] + in[17];  t25  = MUL(in[14] - in[17], costab29);

  t45  = t24 + t25;
  t63  = MUL(t24 - t25, costab6);
  t37  = t8  + t9;
  t54  = MUL(t8  - t9,  costab6);

  t10  = in[6]  + in[25];  t26  = MUL(in[6]  - in[25], costab13);
  t11  = in[9]  + in[22];  t27  = MUL(in[9]  - in[22], costab19);

  t46  = t26 + t27;
  t64  = MUL(t26 - t27, costab26);
  t38  = t10 + t11;
  t55  = MUL(t10 - t11, costab26);

  t12  = in[2]  + in[29];  t28  = MUL(in[2]  - in[29], costab5);
  t13  = in[13] + in[18];  t29  = MUL(in[13] - in[18], costab27);

  t47  = t28 + t29;
  t65  = MUL(t28 - t29, costab10);
  t39  = t12 + t13;
  t56  = MUL(t12 - t13, costab10);

  t14  = in[5]  + in[26];  t30  = MUL(in[5]  - in[26], costab11);
  t15  = in[10] + in[21];  t31  = MUL(in[10] - in[21], costab21);

  t48  = t30 + t31;
  t66  = MUL(t30 - t31, costab22);
  t40  = t14 + t15;
  t57  = MUL(t14 - t15, costab22);

  t69  = t33 + t34;  t89  = MUL(t33 - t34, costab4);
  t70  = t35 + t36;  t90  = MUL(t35 - t36, costab28);
  t71  = t37 + t38;  t91  = MUL(t37 - t38, costab12);
  t72  = t39 + t40;  t92  = MUL(t39 - t40, costab20);
  t73  = t41 + t42;  t94  = MUL(t41 - t42, costab4);
  t74  = t43 + t44;  t95  = MUL(t43 - t44, costab28);
  t75  = t45 + t46;  t96  = MUL(t45 - t46, costab12);
  t76  = t47 + t48;  t97  = MUL(t47 - t48, costab20);

  t78  = t50 + t51;  t100 = MUL(t50 - t51, costab4);
  t79  = t52 + t53;  t101 = MUL(t52 - t53, costab28);
  t80  = t54 + t55;  t102 = MUL(t54 - t55, costab12);
  t81  = t56 + t57;  t103 = MUL(t56 - t57, costab20);

  t83  = t59 + t60;  t106 = MUL(t59 - t60, costab4);
  t84  = t61 + t62;  t107 = MUL(t61 - t62, costab28);
  t85  = t63 + t64;  t108 = MUL(t63 - t64, costab12);
  t86  = t65 + t66;  t109 = MUL(t65 - t66, costab20);

  t113 = t69  + t70;
  t114 = t71  + t72;

  /*  0 */ hi[15][slot] = SHIFT(t113 + t114);
  /* 16 */ lo[ 0][slot] = SHIFT(MUL(t113 - t114, costab16));

  t115 = t73  + t74;
  t116 = t75  + t76;

  t32  = t115 + t116;

  /*  1 */ hi[14][slot] = SHIFT(t32);

  t118 = t78  + t79;
  t119 = t80  + t81;

  t58  = t118 + t119;

  /*  2 */ hi[13][slot] = SHIFT(t58);

  t121 = t83  + t84;
  t122 = t85  + t86;

  t67  = t121 + t122;

  t49  = (t67 * 2) - t32;

  /*  3 */ hi[12][slot] = SHIFT(t49);

  t125 = t89  + t90;
  t126 = t91  + t92;

  t93  = t125 + t126;

  /*  4 */ hi[11][slot] = SHIFT(t93);

  t128 = t94  + t95;
  t129 = t96  + t97;

  t98  = t128 + t129;

  t68  = (t98 * 2) - t49;

  /*  5 */ hi[10][slot] = SHIFT(t68);

  t132 = t100 + t101;
  t133 = t102 + t103;

  t104 = t132 + t133;

  t82  = (t104 * 2) - t58;

  /*  6 */ hi[ 9][slot] = SHIFT(t82);

  t136 = t106 + t107;
  t137 = t108 + t109;

  t110 = t136 + t137;

  t87  = (t110 * 2) - t67;

  t77  = (t87 * 2) - t68;

  /*  7 */ hi[ 8][slot] = SHIFT(t77);

  t141 = MUL(t69 - t70, costab8);
  t142 = MUL(t71 - t72, costab24);
  t143 = t141 + t142;

  /*  8 */ hi[ 7][slot] = SHIFT(t143);
  /* 24 */ lo[ 8][slot] =
	     SHIFT((MUL(t141 - t142, costab16) * 2) - t143);

  t144 = MUL(t73 - t74, costab8);
  t145 = MUL(t75 - t76, costab24);
  t146 = t144 + t145;

  t88  = (t146 * 2) - t77;

  /*  9 */ hi[ 6][slot] = SHIFT(t88);

  t148 = MUL(t78 - t79, costab8);
  t149 = MUL(t80 - t81, costab24);
  t150 = t148 + t149;

  t105 = (t150 * 2) - t82;

  /* 10 */ hi[ 5][slot] = SHIFT(t105);

  t152 = MUL(t83 - t84, costab8);
  t153 = MUL(t85 - t86, costab24);
  t154 = t152 + t153;

  t111 = (t154 * 2) - t87;

  t99  = (t111 * 2) - t88;

  /* 11 */ hi[ 4][slot] = SHIFT(t99);

  t157 = MUL(t89 - t90, costab8);
  t158 = MUL(t91 - t92, costab24);
  t159 = t157 + t158;

  t127 = (t159 * 2) - t93;

  /* 12 */ hi[ 3][slot] = SHIFT(t127);

  t160 = (MUL(t125 - t126, costab16) * 2) - t127;

  /* 20 */ lo[ 4][slot] = SHIFT(t160);
  /* 28 */ lo[12][slot] =
	     SHIFT((((MUL(t157 - t158, costab16) * 2) - t159) * 2) - t160);

  t161 = MUL(t94 - t95, costab8);
  t162 = MUL(t96 - t97, costab24);
  t163 = t161 + t162;

  t130 = (t163 * 2) - t98;

  t112 = (t130 * 2) - t99;

  /* 13 */ hi[ 2][slot] = SHIFT(t112);

  t164 = (MUL(t128 - t129, costab16) * 2) - t130;

  t166 = MUL(t100 - t101, costab8);
  t167 = MUL(t102 - t103, costab24);
  t168 = t166 + t167;

  t134 = (t168 * 2) - t104;

  t120 = (t134 * 2) - t105;

  /* 14 */ hi[ 1][slot] = SHIFT(t120);

  t135 = (MUL(t118 - t119, costab16) * 2) - t120;

  /* 18 */ lo[ 2][slot] = SHIFT(t135);

  t169 = (MUL(t132 - t133, costab16) * 2) - t134;

  t151 = (t169 * 2) - t135;

  /* 22 */ lo[ 6][slot] = SHIFT(t151);

  t170 = (((MUL(t148 - t149, costab16) * 2) - t150) * 2) - t151;

  /* 26 */ lo[10][slot] = SHIFT(t170);
  /* 30 */ lo[14][slot] =
	     SHIFT((((((MUL(t166 - t167, costab16) * 2) -
		       t168) * 2) - t169) * 2) - t170);

  t171 = MUL(t106 - t107, costab8);
  t172 = MUL(t108 - t109, costab24);
  t173 = t171 + t172;

  t138 = (t173 * 2) - t110;

  t123 = (t138 * 2) - t111;

  t139 = (MUL(t121 - t122, costab16) * 2) - t123;

  t117 = (t123 * 2) - t112;

  /* 15 */ hi[ 0][slot] = SHIFT(t117);

  t124 = (MUL(t115 - t116, costab16) * 2) - t117;

  /* 17 */ lo[ 1][slot] = SHIFT(t124);

  t131 = (t139 * 2) - t124;

  /* 19 */ lo[ 3][slot] = SHIFT(t131);

  t140 = (t164 * 2) - t131;

  /* 21 */ lo[ 5][slot] = SHIFT(t140);

  t174 = (MUL(t136 - t137, costab16) * 2) - t138;

  t155 = (t174 * 2) - t139;

  t147 = (t155 * 2) - t140;

  /* 23 */ lo[ 7][slot] = SHIFT(t147);

  t156 = (((MUL(t144 - t145, costab16) * 2) - t146) * 2) - t147;

  /* 25 */ lo[ 9][slot] = SHIFT(t156);

  t175 = (((MUL(t152 - t153, costab16) * 2) - t154) * 2) - t155;

  t165 = (t175 * 2) - t156;

  /* 27 */ lo[11][slot] = SHIFT(t165);

  t176 = (((((MUL(t161 - t162, costab16) * 2) -
	     t163) * 2) - t164) * 2) - t165;

  /* 29 */ lo[13][slot] = SHIFT(t176);
  /* 31 */ lo[15][slot] =
	     SHIFT((((((((MUL(t171 - t172, costab16) * 2) -
			 t173) * 2) - t174) * 2) - t175) * 2) - t176);

  /*
   * Totals:
   *  80 multiplies
   *  80 additions
   * 119 subtractions
   *  49 shifts (not counting SSO)
   */
}
#endif //JZ4750_OPT

# undef MUL
# undef SHIFT

/* third SSO shift and/or D[] optimization preshift */

# if defined(OPT_SSO)
#  if MAD_F_FRACBITS != 28
#   error "MAD_F_FRACBITS must be 28 to use OPT_SSO"
#  endif
#  define ML0(hi, lo, x, y)	((lo)  = (x) * (y))
#  define MLA(hi, lo, x, y)	((lo) += (x) * (y))
#  define MLS(hi, lo, x, y)     ((lo) -= (x) * (y))
#  define MLN(hi, lo)		((lo)  = -(lo))
#  define MLZ(hi, lo)		((void) (hi), (mad_fixed_t) (lo))
#  define SHIFT(x)		((x) >> 2)
#  define PRESHIFT(x)		((MAD_F(x) + (1L << 13)) >> 14)
# else
#  define ML0(hi, lo, x, y)	MAD_F_ML0((hi), (lo), (x), (y))
#  define MLA(hi, lo, x, y)	MAD_F_MLA((hi), (lo), (x), (y))
#  define MLS(hi, lo, x, y)     MAD_F_MLS((hi), (lo), (x), (y))
#  define MLN(hi, lo)		MAD_F_MLN((hi), (lo))
#  define MLZ(hi, lo)		MAD_F_MLZ((hi), (lo))
#  define SHIFT(x)		(x)
#  if defined(MAD_F_SCALEBITS)
#   undef  MAD_F_SCALEBITS
#   define MAD_F_SCALEBITS	(MAD_F_FRACBITS - 12)
#   define PRESHIFT(x)		(MAD_F(x) >> 12)
#  else
#   define PRESHIFT(x)		MAD_F(x)
#  endif
# endif

static
mad_fixed_t const D[17][32] = {
# include "D.dat"
};

# if defined(ASO_SYNTH)
void synth_full(struct mad_synth *, struct mad_frame const *,
		unsigned int, unsigned int);
# else
/*
 * NAME:	synth->full()
 * DESCRIPTION:	perform full frequency PCM synthesis
 */
#if defined(JZ4750_OPT)
static
void synth_full(struct mad_synth *synth, struct mad_frame const *frame,
                unsigned int nch, unsigned int ns)
{
  unsigned int phase, ch, s, sb, pe, po;
  mad_fixed_t *pcm1, *pcm2, (*filter)[2][2][16][8];
  mad_fixed_t const (*sbsample)[36][32];
  register mad_fixed_t (*fe)[8], (*fx)[8], (*fo)[8];
  register mad_fixed_t *pfe, *pfx, *pfo;
  register mad_fixed_t const (*Dptr)[32], *ptr;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  for (ch = 0; ch < nch; ++ch) {
    sbsample = &frame->sbsample[ch];
    filter   = &synth->filter[ch];
    phase    = synth->phase;
    pcm1     = synth->pcm.samples[ch] - 1;

    for (s = 0; s < ns; ++s) {
      dct32((*sbsample)[s], phase >> 1,
	    (*filter)[0][phase & 1], (*filter)[1][phase & 1]);

      pe = phase & ~1;
      po = ((phase - 1) & 0xf) | 1;

      /* calculate 32 samples */

      pfe = (*filter)[0][ phase & 1][0];
      pfx = (*filter)[0][~phase & 1][0];
      pfo = (*filter)[1][~phase & 1][0];

      Dptr = &D[0];

      ptr = *Dptr + pe;
      S32MUL(xr1,xr2, pfe[0], ptr[ 0]);
      S32MADD(xr1,xr2, pfe[1], ptr[14]);
      S32MADD(xr1,xr2, pfe[2], ptr[12]);
      S32MADD(xr1,xr2, pfe[3], ptr[10]);
      S32MADD(xr1,xr2, pfe[4], ptr[ 8]);
      S32MADD(xr1,xr2, pfe[5], ptr[ 6]);
      S32MADD(xr1,xr2, pfe[6], ptr[ 4]);
      S32MADD(xr1,xr2, pfe[7], ptr[ 2]);

      ptr = *Dptr + po;
      S32MSUB(xr1,xr2, pfx[0], ptr[ 0]);
      S32MSUB(xr1,xr2, pfx[1], ptr[14]);
      S32MSUB(xr1,xr2, pfx[2], ptr[12]);
      S32MSUB(xr1,xr2, pfx[3], ptr[10]);
      S32MSUB(xr1,xr2, pfx[4], ptr[ 8]);
      S32MSUB(xr1,xr2, pfx[5], ptr[ 6]);
      S32MSUB(xr1,xr2, pfx[6], ptr[ 4]);
      S32MSUB(xr1,xr2, pfx[7], ptr[ 2]);
      S32ALN(xr1, xr1, xr2, 2);
      S32SDI(xr1, pcm1, 0x04);

      pcm2 = pcm1 + 32;

      for (sb = 1; sb < 16; ++sb) {
        pfe += 8;
	++Dptr;

	/* D[32 - sb][i] == -D[sb][31 - i] */
	ptr = *Dptr + pe;
	S32MUL(xr1,xr2, pfe[7], ptr[ 2]);
	S32MADD(xr1,xr2, pfe[6], ptr[ 4]);
	S32MADD(xr1,xr2, pfe[5], ptr[ 6]);
	S32MADD(xr1,xr2, pfe[4], ptr[ 8]);
	S32MADD(xr1,xr2, pfe[3], ptr[10]);
	S32MADD(xr1,xr2, pfe[2], ptr[12]);
	S32MADD(xr1,xr2, pfe[1], ptr[14]);
	S32MADD(xr1,xr2, pfe[0], ptr[ 0]);

	ptr = *Dptr + po;
	S32MSUB(xr1,xr2, pfo[0], ptr[ 0]);
	S32MSUB(xr1,xr2, pfo[1], ptr[14]);
	S32MSUB(xr1,xr2, pfo[2], ptr[12]);
	S32MSUB(xr1,xr2, pfo[3], ptr[10]);
	S32MSUB(xr1,xr2, pfo[4], ptr[ 8]);
	S32MSUB(xr1,xr2, pfo[5], ptr[ 6]);
	S32MSUB(xr1,xr2, pfo[6], ptr[ 4]);
	S32MSUB(xr1,xr2, pfo[7], ptr[ 2]);

        S32ALN(xr1,xr1,xr2,2);
        S32SDI(xr1,pcm1,0x04);

	ptr = *Dptr - pe;
	S32MUL(xr1,xr2, pfe[0], ptr[15]);
	S32MADD(xr1,xr2, pfe[1], ptr[17]);
	S32MADD(xr1,xr2, pfe[2], ptr[19]);
	S32MADD(xr1,xr2, pfe[3], ptr[21]);
	S32MADD(xr1,xr2, pfe[4], ptr[23]);
	S32MADD(xr1,xr2, pfe[5], ptr[25]);
	S32MADD(xr1,xr2, pfe[6], ptr[27]);
	S32MADD(xr1,xr2, pfe[7], ptr[29]);

	ptr = *Dptr - po;
	S32MADD(xr1,xr2, pfo[7], ptr[29]);
	S32MADD(xr1,xr2, pfo[6], ptr[27]);
	S32MADD(xr1,xr2, pfo[5], ptr[25]);
	S32MADD(xr1,xr2, pfo[4], ptr[23]);
	S32MADD(xr1,xr2, pfo[3], ptr[21]);
	S32MADD(xr1,xr2, pfo[2], ptr[19]);
	S32MADD(xr1,xr2, pfo[1], ptr[17]);
	S32MADD(xr1,xr2, pfo[0], ptr[15]);

        S32ALN(xr1,xr1,xr2,2);
        S32SDI(xr1, pcm2, -0x04);
        pfo += 8;
      }

      ++Dptr;

      ptr = *Dptr + po;

      S32MUL(xr1,xr2, pfo[0], ptr[ 0]);
      S32MADD(xr1,xr2, pfo[1], ptr[14]);
      S32MADD(xr1,xr2, pfo[2], ptr[12]);
      S32MADD(xr1,xr2, pfo[3], ptr[10]);
      S32MADD(xr1,xr2, pfo[4], ptr[ 8]);
      S32MADD(xr1,xr2, pfo[5], ptr[ 6]);
      S32MADD(xr1,xr2, pfo[6], ptr[ 4]);
      S32MADD(xr1,xr2, pfo[7], ptr[ 2]);
      S32ALN(xr1,xr1,xr2,2);
      D32ADD_SS(xr1,xr0,xr1,xr0);
      S32SDI(xr1, pcm1, 0x04);
      pcm1 += 15;

      phase = (phase + 1) % 16;
    }
  }
}
#  else   //JZ4750_OPT

static
void synth_full(struct mad_synth *synth, struct mad_frame const *frame,
		unsigned int nch, unsigned int ns)
{
  unsigned int phase, ch, s, sb, pe, po;
  mad_fixed_t *pcm1, *pcm2, (*filter)[2][2][16][8];
  mad_fixed_t const (*sbsample)[36][32];
  register mad_fixed_t (*fe)[8], (*fx)[8], (*fo)[8];
  register mad_fixed_t const (*Dptr)[32], *ptr;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  for (ch = 0; ch < nch; ++ch) {
    sbsample = &frame->sbsample[ch];
    filter   = &synth->filter[ch];
    phase    = synth->phase;
    pcm1     = synth->pcm.samples[ch];

    for (s = 0; s < ns; ++s) {
      dct32((*sbsample)[s], phase >> 1,
	    (*filter)[0][phase & 1], (*filter)[1][phase & 1]);

      pe = phase & ~1;
      po = ((phase - 1) & 0xf) | 1;

      /* calculate 32 samples */

      fe = &(*filter)[0][ phase & 1][0];
      fx = &(*filter)[0][~phase & 1][0];
      fo = &(*filter)[1][~phase & 1][0];

      Dptr = &D[0];

      ptr = *Dptr + pe;
      ML0(hi, lo, (*fe)[0], ptr[ 0]);
      MLA(hi, lo, (*fe)[1], ptr[14]);
      MLA(hi, lo, (*fe)[2], ptr[12]);
      MLA(hi, lo, (*fe)[3], ptr[10]);
      MLA(hi, lo, (*fe)[4], ptr[ 8]);
      MLA(hi, lo, (*fe)[5], ptr[ 6]);
      MLA(hi, lo, (*fe)[6], ptr[ 4]);
      MLA(hi, lo, (*fe)[7], ptr[ 2]);

      ptr = *Dptr + po;
      MLS(hi, lo, (*fx)[0], ptr[ 0]);
      MLS(hi, lo, (*fx)[1], ptr[14]);
      MLS(hi, lo, (*fx)[2], ptr[12]);
      MLS(hi, lo, (*fx)[3], ptr[10]);
      MLS(hi, lo, (*fx)[4], ptr[ 8]);
      MLS(hi, lo, (*fx)[5], ptr[ 6]);
      MLS(hi, lo, (*fx)[6], ptr[ 4]);
      MLS(hi, lo, (*fx)[7], ptr[ 2]);

      *pcm1++ = SHIFT(MLZ(hi, lo));

      pcm2 = pcm1 + 30;

      for (sb = 1; sb < 16; ++sb) {
	++fe;
	++Dptr;

	/* D[32 - sb][i] == -D[sb][31 - i] */

        ptr = *Dptr + pe;
        ML0(hi, lo, (*fe)[7], ptr[ 2]);
        MLA(hi, lo, (*fe)[6], ptr[ 4]);
        MLA(hi, lo, (*fe)[5], ptr[ 6]);
        MLA(hi, lo, (*fe)[4], ptr[ 8]);
        MLA(hi, lo, (*fe)[3], ptr[10]);
        MLA(hi, lo, (*fe)[2], ptr[12]);
        MLA(hi, lo, (*fe)[1], ptr[14]);
        MLA(hi, lo, (*fe)[0], ptr[ 0]);

        ptr = *Dptr + po;
        MLS(hi, lo, (*fo)[0], ptr[ 0]);
        MLS(hi, lo, (*fo)[1], ptr[14]);
        MLS(hi, lo, (*fo)[2], ptr[12]);
        MLS(hi, lo, (*fo)[3], ptr[10]);
        MLS(hi, lo, (*fo)[4], ptr[ 8]);
        MLS(hi, lo, (*fo)[5], ptr[ 6]);
        MLS(hi, lo, (*fo)[6], ptr[ 4]);
        MLS(hi, lo, (*fo)[7], ptr[ 2]);

	*pcm1++ = SHIFT(MLZ(hi, lo));

	ptr = *Dptr - pe;
	ML0(hi, lo, (*fe)[0], ptr[31 - 16]);
	MLA(hi, lo, (*fe)[1], ptr[31 - 14]);
	MLA(hi, lo, (*fe)[2], ptr[31 - 12]);
	MLA(hi, lo, (*fe)[3], ptr[31 - 10]);
	MLA(hi, lo, (*fe)[4], ptr[31 -  8]);
	MLA(hi, lo, (*fe)[5], ptr[31 -  6]);
	MLA(hi, lo, (*fe)[6], ptr[31 -  4]);
	MLA(hi, lo, (*fe)[7], ptr[31 -  2]);

	ptr = *Dptr - po;
	MLA(hi, lo, (*fo)[7], ptr[31 -  2]);
	MLA(hi, lo, (*fo)[6], ptr[31 -  4]);
	MLA(hi, lo, (*fo)[5], ptr[31 -  6]);
	MLA(hi, lo, (*fo)[4], ptr[31 -  8]);
	MLA(hi, lo, (*fo)[3], ptr[31 - 10]);
	MLA(hi, lo, (*fo)[2], ptr[31 - 12]);
	MLA(hi, lo, (*fo)[1], ptr[31 - 14]);
	MLA(hi, lo, (*fo)[0], ptr[31 - 16]);

	*pcm2-- = SHIFT(MLZ(hi, lo));

	++fo;
      }

      ++Dptr;

      ptr = *Dptr + po;
      ML0(hi, lo, (*fo)[0], ptr[ 0]);
      MLA(hi, lo, (*fo)[1], ptr[14]);
      MLA(hi, lo, (*fo)[2], ptr[12]);
      MLA(hi, lo, (*fo)[3], ptr[10]);
      MLA(hi, lo, (*fo)[4], ptr[ 8]);
      MLA(hi, lo, (*fo)[5], ptr[ 6]);
      MLA(hi, lo, (*fo)[6], ptr[ 4]);
      MLA(hi, lo, (*fo)[7], ptr[ 2]);

      *pcm1 = SHIFT(-MLZ(hi, lo));
      pcm1 += 16;

      phase = (phase + 1) % 16;
    }
  }
}
#endif //JZ4750_OPT
# endif

/*
 * NAME:	synth->half()
 * DESCRIPTION:	perform half frequency PCM synthesis
 */
#if defined(JZ4750_OPT)
static
void synth_half(struct mad_synth *synth, struct mad_frame const *frame,
                unsigned int nch, unsigned int ns)
{
  unsigned int phase, ch, s, sb, pe, po;
  mad_fixed_t *pcm1, *pcm2, (*filter)[2][2][16][8];
  mad_fixed_t const (*sbsample)[36][32];
  register mad_fixed_t (*fe)[8], (*fx)[8], (*fo)[8];
  register mad_fixed_t const (*Dptr)[32], *ptr;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;
  mad_fixed_t tmp0, tmp1, tmp2, tmp3;

  for (ch = 0; ch < nch; ++ch) {
    sbsample = &frame->sbsample[ch];
    filter   = &synth->filter[ch];
    phase    = synth->phase;
    pcm1     = synth->pcm.samples[ch];

    for (s = 0; s < ns; ++s) {
      dct32((*sbsample)[s], phase >> 1,
            (*filter)[0][phase & 1], (*filter)[1][phase & 1]);

      pe = phase & ~1;
      po = ((phase - 1) & 0xf) | 1;

      /* calculate 16 samples */

      fe = &(*filter)[0][ phase & 1][0];
      fx = &(*filter)[0][~phase & 1][0];
      fo = &(*filter)[1][~phase & 1][0];

      Dptr = &D[0];

      ptr = *Dptr + po;
      /*
      ML0(hi, lo, (*fx)[0], ptr[ 0]);
      MLA(hi, lo, (*fx)[1], ptr[14]);
      MLA(hi, lo, (*fx)[2], ptr[12]);
      MLA(hi, lo, (*fx)[3], ptr[10]);
      MLA(hi, lo, (*fx)[4], ptr[ 8]);
      MLA(hi, lo, (*fx)[5], ptr[ 6]);
      MLA(hi, lo, (*fx)[6], ptr[ 4]);
      MLA(hi, lo, (*fx)[7], ptr[ 2]);
      MLN(hi, lo);
      */
      tmp0 = 0; tmp1 = 0;
      tmp2 = (*fx)[0]; tmp3 = ptr[ 0];
      D32SLL(xr1,xr0,xr0,xr2,1);
      tmp0 = (*fx)[1]; tmp1 = ptr[14];
      S32MSUB(xr1,xr2,tmp2, tmp3);
      tmp2 = (*fx)[2]; tmp3 = ptr[12];
      S32MSUB(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fx)[3]; tmp1 = ptr[10];
      S32MSUB(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fx)[4]; tmp3 = ptr[ 8];
      S32MSUB(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fx)[5]; tmp1 = ptr[ 6];
      S32MSUB(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fx)[6]; tmp3 = ptr[ 4];
      S32MSUB(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fx)[7]; tmp1 = ptr[ 2];
      S32MSUB(xr1,xr2, tmp2, tmp3);
      S32MSUB(xr1,xr2, tmp0, tmp1);

      ptr = *Dptr + pe;
      /*
      MLA(hi, lo, (*fe)[0], ptr[ 0]);
      MLA(hi, lo, (*fe)[1], ptr[14]);
      MLA(hi, lo, (*fe)[2], ptr[12]);
      MLA(hi, lo, (*fe)[3], ptr[10]);
      MLA(hi, lo, (*fe)[4], ptr[ 8]);
      MLA(hi, lo, (*fe)[5], ptr[ 6]);
      MLA(hi, lo, (*fe)[6], ptr[ 4]);
      MLA(hi, lo, (*fe)[7], ptr[ 2]);

      *pcm1++ = SHIFT(MLZ(hi, lo));
      */
      tmp0 = (*fe)[0]; tmp1 = ptr[ 0];
      tmp2 = (*fe)[1]; tmp3 = ptr[14];
      S32MADD(xr1,xr2,tmp0, tmp1);
      tmp0 = (*fe)[2]; tmp1 = ptr[12];
      S32MADD(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fe)[3]; tmp3 = ptr[10];
      S32MADD(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fe)[4]; tmp1 = ptr[ 8];
      S32MADD(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fe)[5]; tmp3 = ptr[ 6];
      S32MADD(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fe)[6]; tmp1 = ptr[ 4];
      S32MADD(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fe)[7]; tmp3 = ptr[ 2];
      S32MADD(xr1,xr2, tmp0, tmp1);
      S32MADD(xr1,xr2, tmp2, tmp3);

      S32ALN(xr1, xr1, xr2,2);
      tmp0 = S32M2I(xr1);
      *pcm1++ = tmp0;

      pcm2 = pcm1 + 14;

      for (sb = 1; sb < 16; ++sb) {
        ++fe;
        ++Dptr;

        /* D[32 - sb][i] == -D[sb][31 - i] */

        if (!(sb & 1)) {
          ptr = *Dptr + po;
          /*
          ML0(hi, lo, (*fo)[0], ptr[ 0]);
          MLA(hi, lo, (*fo)[1], ptr[14]);
          MLA(hi, lo, (*fo)[2], ptr[12]);
          MLA(hi, lo, (*fo)[3], ptr[10]);
          MLA(hi, lo, (*fo)[4], ptr[ 8]);
          MLA(hi, lo, (*fo)[5], ptr[ 6]);
          MLA(hi, lo, (*fo)[6], ptr[ 4]);
          MLA(hi, lo, (*fo)[7], ptr[ 2]);
          MLN(hi, lo);
          */
        tmp0 = 0; tmp1 = 0;
        tmp2 = (*fo)[0]; tmp3 = ptr[ 0];
        D32SLL(xr1,xr0,xr0,xr2,1);
        tmp0 = (*fo)[1]; tmp1 = ptr[14];
        S32MSUB(xr1,xr2,tmp2, tmp3);
        tmp2 = (*fo)[2]; tmp3 = ptr[12];
        S32MSUB(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fo)[3]; tmp1 = ptr[10];
        S32MSUB(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fo)[4]; tmp3 = ptr[ 8];
        S32MSUB(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fo)[5]; tmp1 = ptr[ 6];
        S32MSUB(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fo)[6]; tmp3 = ptr[ 4];
        S32MSUB(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fo)[7]; tmp1 = ptr[ 2];
        S32MSUB(xr1,xr2, tmp2, tmp3);
        S32MSUB(xr1,xr2, tmp0, tmp1);

          ptr = *Dptr + pe;
          /*
          MLA(hi, lo, (*fe)[7], ptr[ 2]);
          MLA(hi, lo, (*fe)[6], ptr[ 4]);
          MLA(hi, lo, (*fe)[5], ptr[ 6]);
          MLA(hi, lo, (*fe)[4], ptr[ 8]);
          MLA(hi, lo, (*fe)[3], ptr[10]);
          MLA(hi, lo, (*fe)[2], ptr[12]);
          MLA(hi, lo, (*fe)[1], ptr[14]);
          MLA(hi, lo, (*fe)[0], ptr[ 0]);

          *pcm1++ = SHIFT(MLZ(hi, lo));
          */
        tmp0 = (*fe)[7]; tmp1 = ptr[ 2];
        tmp2 = (*fe)[6]; tmp3 = ptr[ 4];
        S32MADD(xr1,xr2,tmp0, tmp1);
        tmp0 = (*fe)[5]; tmp1 = ptr[ 6];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fe)[4]; tmp3 = ptr[ 8];
        S32MADD(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fe)[3]; tmp1 = ptr[10];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fe)[2]; tmp3 = ptr[12];
        S32MADD(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fe)[1]; tmp1 = ptr[14];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fe)[0]; tmp3 = ptr[ 0];
        S32MADD(xr1,xr2, tmp0, tmp1);
        S32MADD(xr1,xr2, tmp2, tmp3);

      S32ALN(xr1, xr1, xr2,2);
        tmp0 = S32M2I(xr1);
        *pcm1++ = tmp0;

          ptr = *Dptr - po;
          /*
          ML0(hi, lo, (*fo)[7], ptr[31 -  2]);
          MLA(hi, lo, (*fo)[6], ptr[31 -  4]);
          MLA(hi, lo, (*fo)[5], ptr[31 -  6]);
          MLA(hi, lo, (*fo)[4], ptr[31 -  8]);
          MLA(hi, lo, (*fo)[3], ptr[31 - 10]);
          MLA(hi, lo, (*fo)[2], ptr[31 - 12]);
          MLA(hi, lo, (*fo)[1], ptr[31 - 14]);
          MLA(hi, lo, (*fo)[0], ptr[31 - 16]);
          */
        tmp0 = (*fo)[7]; tmp1 = ptr[31 -  2];
        tmp2 = (*fo)[6]; tmp3 = ptr[31 -  4];
        S32MUL(xr1,xr2,tmp0, tmp1);
        tmp0 = (*fo)[5]; tmp1 = ptr[31 -  6];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fo)[4]; tmp3 = ptr[31 -  8];
        S32MADD(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fo)[3]; tmp1 = ptr[31 - 10];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fo)[2]; tmp3 = ptr[31 - 12];
        S32MADD(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fo)[1]; tmp1 = ptr[31 - 14];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fo)[0]; tmp3 = ptr[31 - 16];
        S32MADD(xr1,xr2, tmp0, tmp1);
        S32MADD(xr1,xr2, tmp2, tmp3);

          ptr = *Dptr - pe;
          /*
          MLA(hi, lo, (*fe)[0], ptr[31 - 16]);
          MLA(hi, lo, (*fe)[1], ptr[31 - 14]);
          MLA(hi, lo, (*fe)[2], ptr[31 - 12]);
          MLA(hi, lo, (*fe)[3], ptr[31 - 10]);
          MLA(hi, lo, (*fe)[4], ptr[31 -  8]);
          MLA(hi, lo, (*fe)[5], ptr[31 -  6]);
          MLA(hi, lo, (*fe)[6], ptr[31 -  4]);
          MLA(hi, lo, (*fe)[7], ptr[31 -  2]);

          *pcm2-- = SHIFT(MLZ(hi, lo));
          */
        tmp0 = (*fe)[0]; tmp1 = ptr[31 - 16];
        tmp2 = (*fe)[1]; tmp3 = ptr[31 - 14];
        S32MADD(xr1,xr2,tmp0, tmp1);
        tmp0 = (*fe)[2]; tmp1 = ptr[31 - 12];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fe)[3]; tmp3 = ptr[31 - 10];
        S32MADD(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fe)[4]; tmp1 = ptr[31 -  8];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fe)[5]; tmp3 = ptr[31 -  6];
        S32MADD(xr1,xr2, tmp0, tmp1);
        tmp0 = (*fe)[6]; tmp1 = ptr[31 -  4];
        S32MADD(xr1,xr2, tmp2, tmp3);
        tmp2 = (*fe)[7]; tmp3 = ptr[31 -  2];
        S32MADD(xr1,xr2, tmp0, tmp1);
        S32MADD(xr1,xr2, tmp2, tmp3);
      S32ALN(xr1, xr1, xr2,2);
        tmp0 = S32M2I(xr1);
        *pcm2-- = tmp0;

        }

        ++fo;
      }

      ++Dptr;

      ptr = *Dptr + po;
      /*
      ML0(hi, lo, (*fo)[0], ptr[ 0]);
      MLA(hi, lo, (*fo)[1], ptr[14]);
      MLA(hi, lo, (*fo)[2], ptr[12]);
      MLA(hi, lo, (*fo)[3], ptr[10]);
      MLA(hi, lo, (*fo)[4], ptr[ 8]);
      MLA(hi, lo, (*fo)[5], ptr[ 6]);
      MLA(hi, lo, (*fo)[6], ptr[ 4]);
      MLA(hi, lo, (*fo)[7], ptr[ 2]);

      *pcm1 = SHIFT(-MLZ(hi, lo));
      */

      tmp0 = (*fo)[0]; tmp1 = ptr[ 0];
      tmp2 = (*fo)[1]; tmp3 = ptr[14];
      S32MUL(xr1,xr2,tmp0, tmp1);
      tmp0 = (*fo)[2]; tmp1 = ptr[12];
      S32MADD(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fo)[3]; tmp3 = ptr[10];
      S32MADD(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fo)[4]; tmp1 = ptr[ 8];
      S32MADD(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fo)[5]; tmp3 = ptr[ 6];
      S32MADD(xr1,xr2, tmp0, tmp1);
      tmp0 = (*fo)[6]; tmp1 = ptr[ 4];
      S32MADD(xr1,xr2, tmp2, tmp3);
      tmp2 = (*fo)[7]; tmp3 = ptr[ 2];
      S32MADD(xr1,xr2, tmp0, tmp1);
      S32MADD(xr1,xr2, tmp2, tmp3);

      S32ALN(xr1, xr1, xr2,2);
      tmp0 = S32M2I(xr1);

      *pcm1 = -tmp0;
      pcm1 += 8;

      phase = (phase + 1) % 16;
    }
  }
}

#else//JZ4750_OPT

static
void synth_half(struct mad_synth *synth, struct mad_frame const *frame,
		unsigned int nch, unsigned int ns)
{
  unsigned int phase, ch, s, sb, pe, po;
  mad_fixed_t *pcm1, *pcm2, (*filter)[2][2][16][8];
  mad_fixed_t const (*sbsample)[36][32];
  register mad_fixed_t (*fe)[8], (*fx)[8], (*fo)[8];
  register mad_fixed_t const (*Dptr)[32], *ptr;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  for (ch = 0; ch < nch; ++ch) {
    sbsample = &frame->sbsample[ch];
    filter   = &synth->filter[ch];
    phase    = synth->phase;
    pcm1     = synth->pcm.samples[ch];

    for (s = 0; s < ns; ++s) {
      dct32((*sbsample)[s], phase >> 1,
	    (*filter)[0][phase & 1], (*filter)[1][phase & 1]);

      pe = phase & ~1;
      po = ((phase - 1) & 0xf) | 1;

      /* calculate 16 samples */

      fe = &(*filter)[0][ phase & 1][0];
      fx = &(*filter)[0][~phase & 1][0];
      fo = &(*filter)[1][~phase & 1][0];

      Dptr = &D[0];

      ptr = *Dptr + pe;
      ML0(hi, lo, (*fe)[0], ptr[ 0]);
      MLA(hi, lo, (*fe)[1], ptr[14]);
      MLA(hi, lo, (*fe)[2], ptr[12]);
      MLA(hi, lo, (*fe)[3], ptr[10]);
      MLA(hi, lo, (*fe)[4], ptr[ 8]);
      MLA(hi, lo, (*fe)[5], ptr[ 6]);
      MLA(hi, lo, (*fe)[6], ptr[ 4]);
      MLA(hi, lo, (*fe)[7], ptr[ 2]);

      ptr = *Dptr + po;
      MLS(hi, lo, (*fx)[0], ptr[ 0]);
      MLS(hi, lo, (*fx)[1], ptr[14]);
      MLS(hi, lo, (*fx)[2], ptr[12]);
      MLS(hi, lo, (*fx)[3], ptr[10]);
      MLS(hi, lo, (*fx)[4], ptr[ 8]);
      MLS(hi, lo, (*fx)[5], ptr[ 6]);
      MLS(hi, lo, (*fx)[6], ptr[ 4]);
      MLS(hi, lo, (*fx)[7], ptr[ 2]);

      *pcm1++ = SHIFT(MLZ(hi, lo));

      pcm2 = pcm1 + 14;

      for (sb = 1; sb < 16; ++sb) {
	++fe;
	++Dptr;

	/* D[32 - sb][i] == -D[sb][31 - i] */

	if (!(sb & 1)) {
          ptr = *Dptr + pe;
          ML0(hi, lo, (*fe)[7], ptr[ 2]);
          MLA(hi, lo, (*fe)[6], ptr[ 4]);
          MLA(hi, lo, (*fe)[5], ptr[ 6]);
          MLA(hi, lo, (*fe)[4], ptr[ 8]);
          MLA(hi, lo, (*fe)[3], ptr[10]);
          MLA(hi, lo, (*fe)[2], ptr[12]);
          MLA(hi, lo, (*fe)[1], ptr[14]);
          MLA(hi, lo, (*fe)[0], ptr[ 0]);

          ptr = *Dptr + po;
          MLS(hi, lo, (*fo)[0], ptr[ 0]);
          MLS(hi, lo, (*fo)[1], ptr[14]);
          MLS(hi, lo, (*fo)[2], ptr[12]);
          MLS(hi, lo, (*fo)[3], ptr[10]);
          MLS(hi, lo, (*fo)[4], ptr[ 8]);
          MLS(hi, lo, (*fo)[5], ptr[ 6]);
          MLS(hi, lo, (*fo)[6], ptr[ 4]);
          MLS(hi, lo, (*fo)[7], ptr[ 2]);

	  *pcm1++ = SHIFT(MLZ(hi, lo));

	  ptr = *Dptr - po;
	  ML0(hi, lo, (*fo)[7], ptr[31 -  2]);
	  MLA(hi, lo, (*fo)[6], ptr[31 -  4]);
	  MLA(hi, lo, (*fo)[5], ptr[31 -  6]);
	  MLA(hi, lo, (*fo)[4], ptr[31 -  8]);
	  MLA(hi, lo, (*fo)[3], ptr[31 - 10]);
	  MLA(hi, lo, (*fo)[2], ptr[31 - 12]);
	  MLA(hi, lo, (*fo)[1], ptr[31 - 14]);
	  MLA(hi, lo, (*fo)[0], ptr[31 - 16]);

	  ptr = *Dptr - pe;
	  MLA(hi, lo, (*fe)[0], ptr[31 - 16]);
	  MLA(hi, lo, (*fe)[1], ptr[31 - 14]);
	  MLA(hi, lo, (*fe)[2], ptr[31 - 12]);
	  MLA(hi, lo, (*fe)[3], ptr[31 - 10]);
	  MLA(hi, lo, (*fe)[4], ptr[31 -  8]);
	  MLA(hi, lo, (*fe)[5], ptr[31 -  6]);
	  MLA(hi, lo, (*fe)[6], ptr[31 -  4]);
	  MLA(hi, lo, (*fe)[7], ptr[31 -  2]);

	  *pcm2-- = SHIFT(MLZ(hi, lo));
	}

	++fo;
      }

      ++Dptr;

      ptr = *Dptr + po;
      ML0(hi, lo, (*fo)[0], ptr[ 0]);
      MLA(hi, lo, (*fo)[1], ptr[14]);
      MLA(hi, lo, (*fo)[2], ptr[12]);
      MLA(hi, lo, (*fo)[3], ptr[10]);
      MLA(hi, lo, (*fo)[4], ptr[ 8]);
      MLA(hi, lo, (*fo)[5], ptr[ 6]);
      MLA(hi, lo, (*fo)[6], ptr[ 4]);
      MLA(hi, lo, (*fo)[7], ptr[ 2]);

      *pcm1 = SHIFT(-MLZ(hi, lo));
      pcm1 += 8;

      phase = (phase + 1) % 16;
    }
  }
}
#endif//JZ4750_OPT

/*
 * NAME:	synth->frame()
 * DESCRIPTION:	perform PCM synthesis of frame subband samples
 */
void mad_synth_frame(struct mad_synth *synth, struct mad_frame const *frame)
{
  unsigned int nch, ns;
  void (*synth_frame)(struct mad_synth *, struct mad_frame const *,
		      unsigned int, unsigned int);

  nch = MAD_NCHANNELS(&frame->header);
  ns  = MAD_NSBSAMPLES(&frame->header);

  synth->pcm.samplerate = frame->header.samplerate;
  synth->pcm.channels   = nch;
  synth->pcm.length     = 32 * ns;

  synth_frame = synth_full;

  if (frame->options & MAD_OPTION_HALFSAMPLERATE) {
    synth->pcm.samplerate /= 2;
    synth->pcm.length     /= 2;

    synth_frame = synth_half;
  }

  synth_frame(synth, frame, nch, ns);

  synth->phase = (synth->phase + ns) % 16;
}
