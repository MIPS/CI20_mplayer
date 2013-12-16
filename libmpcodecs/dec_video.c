/*
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mp_msg.h"
#include "help_mp.h"

#include "osdep/timer.h"
#include "osdep/shmem.h"

#include "stream/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/parse_es.h"

#include "codec-cfg.h"

#include "libvo/video_out.h"

#include "libmpdemux/stheader.h"
#include "vd.h"
#include "vf.h"
#include "eosd.h"

#include "dec_video.h"

#ifdef CONFIG_DYNAMIC_PLUGINS
#include <dlfcn.h>
#endif

// ===================================================================

extern double video_time_usage;
extern double vout_time_usage;

#include "cpudetect.h"

int field_dominance = -1;

int divx_quality = 0;

const vd_functions_t *mpvdec = NULL;

int get_video_quality_max(sh_video_t *sh_video)
{
    vf_instance_t *vf = sh_video->vfilter;
    if (vf) {
        int ret = vf->control(vf, VFCTRL_QUERY_MAX_PP_LEVEL, NULL);
        if (ret > 0) {
            mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_UsingExternalPP, ret);
            return ret;
        }
    }
    if (mpvdec) {
        int ret = mpvdec->control(sh_video, VDCTRL_QUERY_MAX_PP_LEVEL, NULL);
        if (ret > 0) {
            mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_UsingCodecPP, ret);
            return ret;
        }
    }
//  mp_msg(MSGT_DECVIDEO,MSGL_INFO,"[PP] Sorry, postprocessing is not available\n");
    return 0;
}

void set_video_quality(sh_video_t *sh_video, int quality)
{
    vf_instance_t *vf = sh_video->vfilter;
    if (vf) {
        int ret = vf->control(vf, VFCTRL_SET_PP_LEVEL, (void *) (&quality));
        if (ret == CONTROL_TRUE)
            return;             // success
    }
    if (mpvdec)
        mpvdec->control(sh_video, VDCTRL_SET_PP_LEVEL, (void *) (&quality));
}

int set_video_colors(sh_video_t *sh_video, const char *item, int value)
{
    vf_instance_t *vf = sh_video->vfilter;
    vf_equalizer_t data;

    data.item  = item;
    data.value = value;

    mp_dbg(MSGT_DECVIDEO, MSGL_V, "set video colors %s=%d \n", item, value);
    if (vf) {
        int ret = vf->control(vf, VFCTRL_SET_EQUALIZER, &data);
        if (ret == CONTROL_TRUE)
            return 1;
    }
    /* try software control */
    if (mpvdec)
        if (mpvdec->control(sh_video, VDCTRL_SET_EQUALIZER, item,
                            (int *) value) == CONTROL_OK)
            return 1;
    mp_msg(MSGT_DECVIDEO, MSGL_V, MSGTR_VideoAttributeNotSupportedByVO_VD,
           item);
    return 0;
}

int get_video_colors(sh_video_t *sh_video, const char *item, int *value)
{
    vf_instance_t *vf = sh_video->vfilter;
    vf_equalizer_t data;

    data.item = item;

    mp_dbg(MSGT_DECVIDEO, MSGL_V, "get video colors %s \n", item);
    if (vf) {
        int ret = vf->control(vf, VFCTRL_GET_EQUALIZER, &data);
        if (ret == CONTROL_TRUE) {
            *value = data.value;
            return 1;
        }
    }
    /* try software control */
    if (mpvdec)
        return mpvdec->control(sh_video, VDCTRL_GET_EQUALIZER, item, value);
    return 0;
}

int set_rectangle(sh_video_t *sh_video, int param, int value)
{
    vf_instance_t *vf = sh_video->vfilter;
    int data[] = { param, value };

    mp_dbg(MSGT_DECVIDEO, MSGL_V, "set rectangle \n");
    if (vf) {
        int ret = vf->control(vf, VFCTRL_CHANGE_RECTANGLE, data);
        if (ret)
            return 1;
    }
    return 0;
}

void resync_video_stream(sh_video_t *sh_video)
{
    sh_video->timer            = 0;
    sh_video->next_frame_time  = 0;
    sh_video->num_buffered_pts = 0;
    sh_video->last_pts         = MP_NOPTS_VALUE;
    if (mpvdec)
        mpvdec->control(sh_video, VDCTRL_RESYNC_STREAM, NULL);
}

int get_current_video_decoder_lag(sh_video_t *sh_video)
{
    int ret;

    if (!mpvdec)
        return -1;
    ret = mpvdec->control(sh_video, VDCTRL_QUERY_UNSEEN_FRAMES, NULL);
    if (ret >= 10)
        return ret - 10;
    return -1;
}

void uninit_video(sh_video_t *sh_video)
{
    if (!sh_video->initialized)
        return;
    mp_msg(MSGT_DECVIDEO, MSGL_V, MSGTR_UninitVideoStr, sh_video->codec->drv);
    mpvdec->uninit(sh_video);
#ifdef CONFIG_DYNAMIC_PLUGINS
    if (sh_video->dec_handle)
        dlclose(sh_video->dec_handle);
#endif
    vf_uninit_filter_chain(sh_video->vfilter);
    eosd_uninit();
    sh_video->initialized = 0;
}

void vfm_help(void)
{
    int i;
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_AvailableVideoFm);
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_DRIVERS\n");
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "   vfm:    info:  (comment)\n");
    for (i = 0; mpcodecs_vd_drivers[i] != NULL; i++)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "%8s  %s (%s)\n",
               mpcodecs_vd_drivers[i]->info->short_name,
               mpcodecs_vd_drivers[i]->info->name,
               mpcodecs_vd_drivers[i]->info->comment);
}

static int init_video(sh_video_t *sh_video, char *codecname, char *vfm,
                      int status, stringset_t *selected)
{
    int force = 0;
    unsigned int orig_fourcc = sh_video->bih ? sh_video->bih->biCompression : 0;
    sh_video->codec = NULL;
    sh_video->vf_initialized = 0;
    if (codecname && codecname[0] == '+') {
        codecname = &codecname[1];
        force = 1;
    }

    while (1) {
        int i;
        int orig_w, orig_h;
        // restore original fourcc:
        if (sh_video->bih)
            sh_video->bih->biCompression = orig_fourcc;
        if (!(sh_video->codec =
              find_video_codec(sh_video->format,
                               sh_video->bih ? ((unsigned int *) &sh_video->bih->biCompression) : NULL,
                               sh_video->codec, force)))
            break;
        // ok we found one codec
        if (stringset_test(selected, sh_video->codec->name))
            continue;           // already tried & failed
        if (codecname && strcmp(sh_video->codec->name, codecname))
            continue;           // -vc
        if (vfm && strcmp(sh_video->codec->drv, vfm))
            continue;           // vfm doesn't match
        if (!force && sh_video->codec->status < status)
            continue;           // too unstable
        stringset_add(selected, sh_video->codec->name); // tagging it
        // ok, it matches all rules, let's find the driver!
        for (i = 0; mpcodecs_vd_drivers[i] != NULL; i++)
//          if(mpcodecs_vd_drivers[i]->info->id==sh_video->codec->driver) break;
            if (!strcmp
                (mpcodecs_vd_drivers[i]->info->short_name,
                 sh_video->codec->drv))
                break;
        mpvdec = mpcodecs_vd_drivers[i];
#ifdef CONFIG_DYNAMIC_PLUGINS
        if (!mpvdec) {
            /* try to open shared decoder plugin */
            int buf_len;
            char *buf;
            vd_functions_t *funcs_sym;
            vd_info_t *info_sym;

            buf_len = strlen(MPLAYER_LIBDIR) +
                      strlen(sh_video->codec->drv) + 16;
            buf = malloc(buf_len);
            if (!buf)
                break;
            snprintf(buf, buf_len, "%s/mplayer/vd_%s.so", MPLAYER_LIBDIR,
                     sh_video->codec->drv);
            mp_msg(MSGT_DECVIDEO, MSGL_DBG2,
                   "Trying to open external plugin: %s\n", buf);
            sh_video->dec_handle = dlopen(buf, RTLD_LAZY);
            if (!sh_video->dec_handle)
                break;
            snprintf(buf, buf_len, "mpcodecs_vd_%s", sh_video->codec->drv);
            funcs_sym = dlsym(sh_video->dec_handle, buf);
            if (!funcs_sym || !funcs_sym->info || !funcs_sym->init
                || !funcs_sym->uninit || !funcs_sym->control
                || !funcs_sym->decode)
                break;
            info_sym = funcs_sym->info;
            if (strcmp(info_sym->short_name, sh_video->codec->drv))
                break;
            free(buf);
            mpvdec = funcs_sym;
            mp_msg(MSGT_DECVIDEO, MSGL_V,
                   "Using external decoder plugin (%s/mplayer/vd_%s.so)!\n",
                   MPLAYER_LIBDIR, sh_video->codec->drv);
        }
#endif
        if (!mpvdec) {          // driver not available (==compiled in)
            mp_msg(MSGT_DECVIDEO, MSGL_WARN,
                   MSGTR_VideoCodecFamilyNotAvailableStr,
                   sh_video->codec->name, sh_video->codec->drv);
            continue;
        }
        orig_w = sh_video->bih ? sh_video->bih->biWidth  : sh_video->disp_w;
        orig_h = sh_video->bih ? sh_video->bih->biHeight : sh_video->disp_h;
        sh_video->disp_w = orig_w;
        sh_video->disp_h = orig_h;
        // it's available, let's try to init!
        if (sh_video->codec->flags & CODECS_FLAG_ALIGN16) {
            // align width/height to n*16
            sh_video->disp_w = (sh_video->disp_w + 15) & (~15);
            sh_video->disp_h = (sh_video->disp_h + 15) & (~15);
        }
        if (sh_video->bih) {
            sh_video->bih->biWidth  = sh_video->disp_w;
            sh_video->bih->biHeight = sh_video->disp_h;
        }
        // init()
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_OpeningVideoDecoder,
               mpvdec->info->short_name, mpvdec->info->name);
        // clear vf init error, it is no longer relevant
        if (sh_video->vf_initialized < 0)
            sh_video->vf_initialized = 0;
        if (!mpvdec->init(sh_video)) {
	    mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_VDecoderInitFailed);
            sh_video->disp_w = orig_w;
            sh_video->disp_h = orig_h;
            if (sh_video->bih) {
                sh_video->bih->biWidth  = sh_video->disp_w;
                sh_video->bih->biHeight = sh_video->disp_h;
            }
            continue;           // try next...
        }
        // Yeah! We got it!
        sh_video->initialized = 1;
        return 1;
    }
    return 0;
}

int init_best_video_codec(sh_video_t *sh_video, char **video_codec_list,
                          char **video_fm_list)
{
    char *vc_l_default[2] = { "", (char *) NULL };
    stringset_t selected;
    // hack:
    if (!video_codec_list)
        video_codec_list = vc_l_default;
    // Go through the codec.conf and find the best codec...
    sh_video->initialized = 0;
    stringset_init(&selected);
    while (!sh_video->initialized && *video_codec_list) {
        char *video_codec = *(video_codec_list++);
        if (video_codec[0]) {
            if (video_codec[0] == '-') {
                // disable this codec:
                stringset_add(&selected, video_codec + 1);
            } else {
                // forced codec by name:
                mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_ForcedVideoCodec,
                       video_codec);
                init_video(sh_video, video_codec, NULL, -1, &selected);
            }
        } else {
            int status;
            // try in stability order: UNTESTED, WORKING, BUGGY. never try CRASHING.
            if (video_fm_list) {
                char **fmlist = video_fm_list;
                // try first the preferred codec families:
                while (!sh_video->initialized && *fmlist) {
                    char *video_fm = *(fmlist++);
                    mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_TryForceVideoFmtStr,
                           video_fm);
                    for (status = CODECS_STATUS__MAX;
                         status >= CODECS_STATUS__MIN; --status)
                        if (init_video
                            (sh_video, NULL, video_fm, status, &selected))
                            break;
                }
            }
            if (!sh_video->initialized)
                for (status = CODECS_STATUS__MAX; status >= CODECS_STATUS__MIN;
                     --status)
                    if (init_video(sh_video, NULL, NULL, status, &selected))
                        break;
        }
    }
    stringset_free(&selected);

    if (!sh_video->initialized) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, MSGTR_CantFindVideoCodec,
               sh_video->format);
        return 0;               // failed
    }

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, MSGTR_SelectedVideoCodec,
           sh_video->codec->name, sh_video->codec->drv, sh_video->codec->info);
    return 1;                   // success
}

void *decode_video(sh_video_t *sh_video, unsigned char *start, int in_size,
                   int drop_frame, double pts, int *full_frame)
{
    mp_image_t *mpi = NULL;
    unsigned int t = GetTimer();
    unsigned int t2;
    double tt;
    int delay;
    int got_picture = 1;

    mpi = mpvdec->decode(sh_video, start, in_size, drop_frame);

    //------------------------ frame decoded. --------------------

    if (mpi && mpi->type == MP_IMGTYPE_INCOMPLETE) {
	got_picture = 0;
	mpi = NULL;
    }

    if (full_frame)
	*full_frame = got_picture;

    delay = get_current_video_decoder_lag(sh_video);
    if (correct_pts && pts != MP_NOPTS_VALUE
        && (got_picture || sh_video->num_buffered_pts < delay)) {
        if (sh_video->num_buffered_pts ==
            sizeof(sh_video->buffered_pts) / sizeof(double))
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Too many buffered pts\n");
        else {
            int i, j;
            for (i = 0; i < sh_video->num_buffered_pts; i++)
                if (sh_video->buffered_pts[i] < pts)
                    break;
            for (j = sh_video->num_buffered_pts; j > i; j--)
                sh_video->buffered_pts[j] = sh_video->buffered_pts[j - 1];
            sh_video->buffered_pts[i] = pts;
            sh_video->num_buffered_pts++;
        }
    }

#if HAVE_MMX
    // some codecs are broken, and doesn't restore MMX state :(
    // it happens usually with broken/damaged files.
    if (gCpuCaps.has3DNow) {
        __asm__ volatile ("femms\n\t":::"memory");
    } else if (gCpuCaps.hasMMX) {
        __asm__ volatile ("emms\n\t":::"memory");
    }
#endif

    t2 = GetTimer();
    t = t2 - t;
    tt = t * 0.000001f;
    video_time_usage += tt;

    if (!mpi || drop_frame)
        return NULL;            // error / skipped frame

    if (field_dominance == 0)
        mpi->fields |= MP_IMGFIELD_TOP_FIRST;
    else if (field_dominance == 1)
        mpi->fields &= ~MP_IMGFIELD_TOP_FIRST;

    if (correct_pts) {
        if (sh_video->num_buffered_pts) {
            sh_video->num_buffered_pts--;
            sh_video->pts = sh_video->buffered_pts[sh_video->num_buffered_pts];
        } else {
            mp_msg(MSGT_CPLAYER, MSGL_ERR,
                   "No pts value from demuxer to " "use for frame!\n");
            sh_video->pts = MP_NOPTS_VALUE;
        }
        if (delay >= 0) {
            // limit buffered pts only afterwards so we do not get confused
            // by packets that produce no output (e.g. a single field of a
            // H.264 frame).
            if (delay > sh_video->num_buffered_pts)
#if 0
                // this is disabled because vd_ffmpeg reports the same lag
                // after seek even when there are no buffered frames,
                // leading to incorrect error messages
                mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Not enough buffered pts\n");
#else
                ;
#endif
            else
                sh_video->num_buffered_pts = delay;
        }
    }
    return mpi;
}

extern volatile unsigned char *jpeg_base;
extern volatile unsigned char *gp0_base;
extern volatile unsigned char *vpu_base;
extern volatile unsigned char *cpm_base;
#include "../libjzcommon/jzasm.h"
#include "jzm_jpeg_enc.h"
#include "jzm_jpeg_enc.c"
#include "genhead.c"
static int frameno = 0;
int jpegenc_flags = 0;

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
static int fexists(char *fname)
{
    struct stat dummy;
    if (stat(fname, &dummy) == 0) return 1;
    else return 0;
}

#define CPM_VPU_SWRST    (cpm_base + 0xC4)
#define CPM_VPU_SR     	 (0x1<<31)
#define CPM_VPU_STP    	 (0x1<<30)
#define CPM_VPU_ACK    	 (0x1<<29)

#define write_cpm_reg(a)    (*(volatile unsigned int *)(CPM_VPU_SWRST) = a)
#define read_cpm_reg()      (*(volatile unsigned int *)(CPM_VPU_SWRST))

#define RST_VPU()							\
    {									\
	write_cpm_reg(read_cpm_reg() | CPM_VPU_STP);			\
	while( !(read_cpm_reg() & CPM_VPU_ACK) );			\
	write_cpm_reg( (read_cpm_reg() | CPM_VPU_SR) & (~CPM_VPU_STP) ); \
	write_cpm_reg( read_cpm_reg() & (~CPM_VPU_SR) & (~CPM_VPU_STP) ); \
    }

int genyuv(mp_image_t *mpi, int width, int height, unsigned char* ref_y,unsigned char* ref_c)
{
    unsigned char *pxly, *pxlu, *pxlv;
    int mbx, mby, i, j;
  
    int align_w = (width + 15) / 16;
    int align_h = (height + 15) / 16;

    unsigned char* y_ptr = ref_y;
    unsigned char* c_ptr = ref_c;

    for(mby=0; mby<align_h; mby++){
	for(mbx=0; mbx<align_w; mbx++){
	    pxly = mpi->planes[0] + mby*16*mpi->stride[0] + mbx*16;
	    for(j=0; j<16; j++){
		for(i=0; i<16; i++){
		    *y_ptr = pxly[i];
		    y_ptr++;
		}
		pxly += mpi->stride[0];
	    }
	    pxlu = mpi->planes[1] + mby*8*mpi->stride[1] + mbx*8;
	    pxlv = mpi->planes[2] + mby*8*mpi->stride[2] + mbx*8;
	    for(j=0; j<8; j++){
		for(i=0; i<8; i++){
		    *c_ptr = pxlu[i];
		    c_ptr++;
		}
		for(i=0; i<8; i++){
		    *c_ptr = pxlv[i];
		    c_ptr++;
		}
		pxlu += mpi->stride[1];
		pxlv += mpi->stride[2];
	    }
	}
    }
    jz_dcache_wb();

    return 0;
}

void write_jpeg(mp_image_t *mpi, int width, int height)
{
    FILE * output_file;
    char filename[50];
    int bslen;
    int quality=2;        

    do {
        snprintf (filename, 50, "shot%04d.jpg", ++frameno);
    } while (fexists(filename) && frameno < 100000);
    if (fexists(filename)) {
        printf("screenshot touch the top level 100000!");
        return;
    }
    output_file = fopen(filename, "w+");

    int align_w = (width + 15) & ~0xF;
    int align_h = (height + 15) & ~0xF;
    if (!mpi->ipu_line) { // SW
	if (NULL == mpi->ref_y) {
	    mpi->ref_y = (unsigned char *)jz4740_alloc_frame(256, align_w * align_h);
	    mpi->ref_y = (unsigned char *)((int)mpi->ref_y & ~0xFF);
	}
	if (NULL == mpi->ref_c) {
	    mpi->ref_c = (unsigned char *)jz4740_alloc_frame(256, (align_w * align_h) / 2);
	    mpi->ref_c = (unsigned char *)((int)mpi->ref_c & ~0xFF);
	}
	if (0 != genyuv(mpi, width, height, mpi->ref_y, mpi->ref_c) ) { 
	    printf("genyuv error\n");
	    exit(1);
	}   
#if 0
	{
	    int i, j;
	    unsigned char * data = NULL;
	    FILE * fp = fopen("frame_yuv.c", "w+");
	    printf("w: %d h: %d, stride : %d %d %d\n",
		   width, height, mpi->stride[0], mpi->stride[1], mpi->stride[2]);
	    fprintf(fp, "unsigned char frame_y[] = {\n");
	    for (i = 0; i < height; i++) {
		data = mpi->planes[0] + mpi->stride[0] * height;
		for (j = 0; j < width; j++)
		    fprintf(fp, "0x%02x, ", data[j]);
	    }
	    fprintf(fp, "}\n");
	    fprintf(fp, "unsigned char frame_u[] = {\n");
	    for (i = 0; i < height / 2; i++) {
		data = mpi->planes[1] + mpi->stride[1] * height / 2;
		for (j = 0; j < width / 2; j++)
		    fprintf(fp, "0x%02x, ", data[j]);
	    }
	    fprintf(fp, "}\n");
	    fprintf(fp, "unsigned char frame_v[] = {\n");
	    for (i = 0; i < height / 2; i++) {
		data = mpi->planes[0] + mpi->stride[1] * height / 2;
		for (j = 0; j < width / 2; j++)
		    fprintf(fp, "0x%02x, ", data[j]);
	    }
	    fprintf(fp, "}\n");
	    fclose(fp);
	}
#endif
    } else { // HW
		mpi->ref_y = mpi->planes[0];
		mpi->ref_c = mpi->planes[1];
#if 0
		{
			int i, j, k, h;
			unsigned char * data = NULL;
			FILE * fp = fopen("frame.c", "w+");	
			fprintf(fp, "unsigned char frame_y[] = {\n");
			for (i = 0; i < align_h / 16; i++)	
				for (j = 0; j < align_w / 16; j++)
				{
					data = ref_y + i * mpi->stride[0] + j * 256;	
					fprintf(fp, "// mb %d %d\n", i, j);
					for (k = 0; k < 16; k++) {
						for(h = 0; h < 16; h++)
						{
							fprintf(fp, "0x%02x, ", *data);	
							data++;
						}
						fprintf(fp, "\n");
					}
				}
			fprintf(fp, "}\n");
			fprintf(fp, "unsigned char frame_c[] = {\n");
			for (i = 0; i < align_h / 16; i++)	
				for (j = 0; j < align_w / 16; j++)
				{
					data = ref_c + i * mpi->stride[0] / 2 + j * 128;	
					fprintf(fp, "// mb %d %d\n", i, j);
					for (k = 0; k < 8; k++) {
						for(h = 0; h < 16; h++)
						{
							fprintf(fp, "0x%02x, ", *data);	
							data++;
						}
						fprintf(fp, "\n");
					}
				}
			fprintf(fp, "}\n");
			fclose(fp);
		}
#endif
    }
    if (NULL == mpi->bts) {
	mpi->bts = (unsigned char *)jz4740_alloc_frame(256, 2000000);
	mpi->bts = (unsigned char *)((int)mpi->bts & ~0x7F) + 256;
	printf("Alloc bts = 0x%08x, tile y = 0x%08x, tile c = 0x%08x\n", mpi->bts, mpi->ref_y, mpi->ref_c);
    }
    if ((NULL == mpi->ref_y) || (NULL == mpi->ref_c) || (NULL == mpi->bts)) {
	printf("jz4770_alloc_frame error \n");
	exit(1);
    }

    genhead( output_file, width, height, quality );
    _JPEGE_SliceInfo *s;
    s = malloc(sizeof(_JPEGE_SliceInfo));

    s->des_va = (uint32_t *)jz4740_alloc_frame(256, 100000);  
    s->des_pa = (uint32_t *)get_phy_addr((int)s->des_va);
    s->bsa  = (uint8_t *)get_phy_addr ((int)mpi->bts);
    s->p0a  = (uint8_t *)get_phy_addr ((int)mpi->ref_y);
    s->p1a  = (uint8_t *)get_phy_addr((int)mpi->ref_c);
    s->nmcu = align_w * align_h / 256 - 1;
    s->nrsm = 0x0;
    s->ncol = 0x2;
    s->ql_sel = quality;
    s->huffenc_sel = 0;

    /* VPU reset */    
    RST_VPU();
    JPEGE_SliceInit(s);
    jz_dcache_wb();

    *(volatile unsigned int *)(gp0_base + 0x8) = (VDMA_ACFG_DHA(s->des_pa) | VDMA_ACFG_RUN);
    volatile int *aa = (volatile int *)(vpu_base + REG_SCH_STAT);
    while(!(*(volatile unsigned int *)(jpeg_base + 0x8) & 0x80000000)){
	//usleep(1000);
	//printf("NMCU: %08x\n",read_reg(jpeg_base, 0x8) & 0xFFFFFF);
    } 
    printf("vpu work ending\n");
    bslen = *(volatile unsigned int *)(jpeg_base + 0x8) & 0xFFFFFF;

    fwrite(mpi->bts, 1, bslen,output_file);
    fputc(255,output_file);
    fputc(217,output_file);
    fclose(output_file);
}

int filter_video(sh_video_t *sh_video, void *frame, double pts)
{
    mp_image_t *mpi = frame;
    unsigned int t2 = GetTimer();
    vf_instance_t *vf = sh_video->vfilter;
    // apply video filters and call the leaf vo/ve
    if (jpegenc_flags){
	mp_msg(NULL,NULL,"jpegenc!!,srcw=%d,srch=%d\n",sh_video->disp_w,sh_video->disp_h);
	jpegenc_flags=0;
	write_jpeg(mpi,sh_video->disp_w,sh_video->disp_h);
    }
    int ret = vf->put_image(vf, mpi, pts);
    if (ret > 0) {
        // draw EOSD first so it ends up below the OSD.
        // Note that changing this is will not work right with vf_ass and the
        // vos currently always draw the EOSD first in paused mode.
#ifdef CONFIG_ASS
        vf->control(vf, VFCTRL_DRAW_EOSD, NULL);
#endif
        vf->control(vf, VFCTRL_DRAW_OSD, NULL);
    }

    t2 = GetTimer() - t2;
    vout_time_usage += t2 * 0.000001;

    return ret;
}
