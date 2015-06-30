/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*
*  This program is free software ; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation ; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY ; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program ; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
****************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "T264.h"
#include "ratecontrol.h"
#include "math.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif

typedef struct
{
    // frame numbers per gop
    int32_t gop;
    int32_t np;
    int32_t nb;
    int32_t qp_sum;
    // the number in current gop, based on 0
    int32_t p_no;
    int32_t b_no;
    // prev qp1, qp2
    int32_t qp_p1;
    int32_t qp_p2;
    double deltap;
    // fluid Flow Traffic Model
    int32_t bc;
    // linear Model
    double a1;
    double a2;
    // model
    double x1;
    double x2;
    double qp[20];
    double rp[20];
    double mad[20];
    int32_t window_p;
    int32_t mad_window_p;
    // remaining bits in gop
    int32_t gop_bits;
    double tbl;
    double gamma;
    double beta;
    double theta;
    double wp;
    double wb;
    double AWp;
    double AWb;
    int32_t ideal_bits;
    double mad_p;
} T264_rc_t;

void rc_init_seq(T264_t* t, T264_rc_t* rc);
void rc_init_gop(T264_t* t, T264_rc_t* rc);
void rc_init_pic(T264_t* t, T264_rc_t* rc);
void rc_update_pic(T264_t* t, T264_rc_t* rc);
// frame level
void rc_update_qp(T264_t* t, T264_rc_t* rc);
// quadratic model
void rc_update_quad_model(T264_t* t, T264_rc_t* rc);

void 
rc_init_seq(T264_t* t, T264_rc_t* rc)
{
    double bpp, L1, L2, L3;

    rc->gop = T264_MIN(t->param.idrframe, t->param.iframe);
    rc->np = rc->gop / (1 + t->param.b_num) - 1;
    rc->nb = rc->gop - rc->np - 1;
    rc->bc = 0;
    rc->a1 = 1.0;
    rc->a2 = 0.0;
    rc->x1 = t->param.bitrate;

    if (rc->nb > 0)
    {
        rc->gamma = 0.25;
        rc->beta = 0.9;
        rc->theta = 1.3636;
    }
    else
    {
        rc->gamma = 0.5;
        rc->beta = 0.75;
    }

    if (t->param.qp == 0)
    {
        bpp = ((double)t->param.bitrate) / (t->param.framerate * t->param.width * t->param.height);
        if (t->param.width == 176)
        {
            L1 = 0.1;
            L2 = 0.3;
            L3 = 0.6;
        }
        else if (t->param.width == 352)
        {
            L1 = 0.2;
            L2 = 0.6;
            L3 = 1.2;
        }
        else
        {
            L1 = 0.6;
            L2 = 1.4;
            L3 = 2.4;
        }

        // first gop first i, p
        if (bpp <= L1)
        {
            t->qp_y = 30;
        }
        else if (bpp <= L2)
        {
            t->qp_y = 25;
        }
        else if (bpp <= L3)
        {
            t->qp_y = 20;
        }
        else
        {
            t->qp_y = 10;
        }
    }
}

void 
rc_init_gop(T264_t* t, T264_rc_t* rc)
{
    rc->gop_bits = (int32_t)(rc->gop * t->param.bitrate / (t->param.framerate) - rc->bc);
    if (t->frame_id != 0)
    {
        // JVTH0014 say to do so
        // t->qp_y = rc->qp_sum / rc->np + 8 * rc->bc / rc->gop_bits - T264_MIN(2, rc->gop / 15);
        // JM does this way
        t->qp_y = rc->qp_sum / rc->np - T264_MIN(2, rc->gop / 15);
        if (rc->gop != 1)
        {
            if (t->qp_y > rc->qp_p2 - 2)
                t->qp_y --;
            t->qp_y = clip3(t->qp_y, rc->qp_p2 - 2, rc->qp_p2 + 2);
        }
        t->qp_y = clip3(t->qp_y, t->param.min_qp, t->param.max_qp);
    }

    rc->qp_sum = 0;
    rc->p_no = 0;
    rc->b_no = 0;
    rc->qp_p2 = t->qp_y;
}

void 
rc_init_pic(T264_t* t, T264_rc_t* rc)
{
    int32_t f1, f2, f;

    if (t->slice_type == SLICE_P)
    {
        if (rc->p_no > 0)
        {
            // Step 1.1 Determination of target buffer occupancy
            if (rc->p_no == 1)
            {
                rc->deltap = ((double)rc->bc) / (double)(rc->np - 1);
                rc->tbl = rc->bc;
                rc->AWp = rc->wp;
                rc->AWb = rc->wb;
            }
            else if (rc->p_no < 8)
            {
                rc->AWp = rc->wp * (rc->p_no - 1) / (rc->p_no) + rc->AWp/ (rc->p_no);
            }
            else
            {
                rc->AWp = rc->wp / 8 + 7 * rc->AWp / 8;
            }
            
            rc->tbl -= rc->deltap;
            if (t->param.b_num > 0)
            {
                rc->tbl += rc->AWp * (t->param.b_num + 1) * t->param.bitrate / (t->param.framerate * (rc->AWp + rc->AWb * t->param.b_num))
                    - t->param.bitrate / t->param.framerate;
            }

            // Step 1.2 Microscopic control (target bit rate computation).
            f1 = (int32_t)(t->param.bitrate / t->param.framerate + rc->gamma * (rc->tbl - rc->bc));
            f1 = T264_MAX(0, f1);
            f2 = (int32_t)(rc->wp * rc->gop_bits / (rc->wp * (rc->np - rc->p_no) + rc->wb * (rc->nb - rc->b_no)));
            //f2 = rc->gop_bits / (rc->np - rc->p_no);
            f = (int32_t)(rc->beta * f1 + (1 - rc->beta) * f2);

            rc->ideal_bits = (int32_t)(f * (1 - t->param.b_num * 0.05));
            // HRD consideration ??
        }
    }
    else if (t->slice_type == SLICE_B)
    {
        if (rc->b_no > 0)
        {
            if (rc->b_no == 1)
            {
                rc->AWb = rc->wb;
            }
            else if (rc->b_no < 8)
            {
                rc->AWb = rc->wb * (rc->b_no - 1) / rc->b_no + rc->AWb/ rc->b_no;
            }
            else
            {
                rc->AWb = rc->wb / 8 + 7 * rc->AWb / 8;
            }
        }
    }
    rc_update_qp(t, rc);
}

static void 
rc_update_pic(T264_t* t, T264_rc_t* rc)
{
    int32_t X;

    rc_update_quad_model(t, rc);
    rc->gop_bits -= t->frame_bits;
    rc->bc += (int32_t)(t->frame_bits - t->param.bitrate / t->param.framerate);

    X = (int32_t)t->qp_y * t->frame_bits;
    
    if (t->slice_type == SLICE_P)
    {
        rc->qp_sum += t->qp_y;
        rc->p_no ++;
        rc->wp = X;
        // compute mad
    }
    else if (t->slice_type == SLICE_B)
    {
        rc->b_no ++;
        rc->wb = X / rc->theta;
    }
}

double
qp2qstep( int32_t qp)
{
    int32_t i; 
    double qstep;
    static const double QP2QSTEP[6] = 
    {0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125};

    qstep = QP2QSTEP[qp % 6];
    for(i = 0; i< (qp/6) ; i ++)
        qstep *= 2;

    return qstep;
}

int32_t
qstep2qp(double qstep)
{
    int32_t q_per = 0, q_rem = 0;

    if( qstep < qp2qstep(0))
        return 0;
    else if (qstep > qp2qstep(51))
        return 51;

    while( qstep > qp2qstep(5))
    {
        qstep /= 2;
        q_per += 1;
    }

    if (qstep <= (0.625+0.6875)/2) 
    {
        qstep = 0.625;
        q_rem = 0;
    }
    else if (qstep <= (0.6875+0.8125)/2)
    {
        qstep = 0.6875;
        q_rem = 1;
    }
    else if (qstep <= (0.8125+0.875)/2)
    {
        qstep = 0.8125;
        q_rem = 2;
    }
    else if (qstep <= (0.875+1.0)/2)
    {
        qstep = 0.875;
        q_rem = 3;
    }
    else if (qstep <= (1.0+1.125)/2)
    {
        qstep = 1.0;  
        q_rem = 4;
    }
    else 
    {
        qstep = 1.125;
        q_rem = 5;
    }

    return (q_per * 6 + q_rem);
}

void 
rc_update_qp(T264_t* t, T264_rc_t* rc)
{
    if (t->slice_type == SLICE_P)
    {
        if (rc->p_no != 0)
        {
            if (rc->ideal_bits < 0)
            {
                t->qp_y += 2;
                t->qp_y = T264_MIN(t->qp_y, t->param.max_qp);
            }
            else
            {
                double tmp1, tmp2;
                double step;
                int32_t bits = rc->ideal_bits - t->header_bits;
                double mad = rc->a1 * rc->mad_p + rc->a2;
                int32_t qp;
                bits = T264_MAX(bits, (int32_t)(t->param.bitrate / t->param.framerate / 4));
                tmp1 = mad * mad * rc->x1 * rc->x1 + 4 * rc->x2 * mad * bits;
                tmp2 = sqrt(tmp1) - rc->x1 * mad;
                if (rc->x2 < 0.000001 || tmp1 < 0.000001 ||  tmp2 <= 0.000001)
                    step = rc->x1 * mad / bits;
                else
                    step = rc->x2 * mad * 2 / tmp2;

                qp = qstep2qp(step);
                qp = T264_MIN(rc->qp_p2 + 2, qp);
                qp = T264_MIN(t->param.max_qp, qp);
                qp = T264_MAX(rc->qp_p2 - 2, qp);
                t->qp_y = T264_MAX(t->param.min_qp, qp);
            }
        }
        rc->qp_p1 = rc->qp_p2;
        rc->qp_p2 = t->qp_y;
    }
    else if (t->slice_type == SLICE_B)
    {
        if (t->param.b_num == 1)
        {
            if (rc->qp_p1 == rc->qp_p2)
                t->qp_y += 2;
            else
                t->qp_y = (rc->qp_p1 + rc->qp_p2) / 2 + 1;
            
            t->qp_y = clip3(t->qp_y, t->param.min_qp, t->param.max_qp);
        }
        else
        {
            int32_t step_size;
            if (rc->qp_p2 - rc->qp_p1 <= -2 * t->param.b_num - 3)
                step_size = -3;
            else if (rc->qp_p2 - rc->qp_p1 == -2 * t->param.b_num - 2)
                step_size = -2;
            else if (rc->qp_p2 - rc->qp_p1 == -2 * t->param.b_num - 1)
                step_size = -1;
            else if (rc->qp_p2 - rc->qp_p1 == -2 * t->param.b_num - 0)
                step_size = 0;
            else if (rc->qp_p2 - rc->qp_p1 == -2 * t->param.b_num + 1)
                step_size = 1;
            else
                step_size = 2;

            t->qp_y = rc->qp_p1 + step_size;
            //t->qp_y += T264_MIN(2 * rc->b_no, 
            //    T264_MAX(-2 * rc->b_no, rc->b_no * (rc->qp_p2 - rc->qp_p1) / (t->param.b_num - 1)));
            t->qp_y = clip3(t->qp_y, t->param.min_qp, t->param.max_qp);
        }
    }
}

static void __inline
remove_outlier(T264_t* t, int8_t valid[20], T264_rc_t* rc)
{
    double error[20];
    int32_t i;
    double std = 0.0;
    double threshold;

    for(i = 0 ; i < rc->window_p ; i ++)
    {
        error[i] = rc->x1 / rc->qp[i] + rc->x2 / (rc->qp[i] * rc->rp[i]) - rc->qp[i];
        std += error[i] * error[i];
    }

    threshold = rc->window_p == 2 ? 0 : sqrt(std / rc->window_p);

    for(i = 1 ; i < rc->window_p ; i ++)
    {
        if (error[i] > threshold)
            valid[i] = FALSE;
    }
}

static void __inline
remove_mad_outlier(T264_t* t, int8_t valid[20], T264_rc_t* rc)
{
    double error[20];
    int32_t i;
    double std = 0.0;
    double threshold;

    for(i = 0 ; i < rc->mad_window_p ; i ++)
    {
        error[i] = rc->a1 * rc->mad[i + 1] + rc->a2 - rc->mad[i];
        std += error[i] * error[i];
    }

    threshold = rc->mad_window_p == 2 ? 0 : sqrt(std / rc->mad_window_p);

    for(i = 1 ; i < rc->mad_window_p ; i ++)
    {
        if (error[i] > threshold)
            valid[i] = FALSE;
    }
}

void
mad_model_estimator(T264_t* t, int8_t valid[20], int32_t window, T264_rc_t* rc)
{
    int32_t real_window = 0, i;
    int8_t estimate_x2 = FALSE;
    double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;
    double mad;
    double matrix_value;

    for(i = 0 ; i < window ; i ++)
    {
        if (valid[i])
        {
            real_window ++;
            mad = rc->mad[i];
        }
    }

    rc->a1 = rc->a2 = 0.0;
    for(i = 0 ; i < window ; i ++)
    {
        if (valid[i])
        {
            rc->a1 += rc->mad[i] / (rc->mad[i + 1] * real_window);
            if (ABS(mad - rc->mad[i]) < 0.00001)
                estimate_x2 = TRUE;
        }
    }

    if (real_window >= 1 && estimate_x2)
    {
        for(i = 0 ; i < window ; i ++)
        {
            if (valid[i])
            {
                a00 += 1.0;
                a01 += rc->mad[i + 1];
                a10 = a01;
                a11 += rc->mad[i + 1] * rc->mad[i + 1];
                b0 += rc->mad[i];
                b1 += rc->mad[i] * rc->mad[i + 1];
            }
        }

        matrix_value = a00 * a11 - a01 * a10;
        if (ABS(matrix_value) > 0.000001)
        {
            rc->a2 = (b0 * a11 - b1 * a01) / matrix_value;
            rc->a1 = (b1 * a00 - b0 * a10) / matrix_value;
        }
        else
        {
            rc->a1 = b0 / a01;
            rc->a2 = 0.0;
        }
    }
}

void
rc_update_mad_model(T264_t* t, T264_rc_t* rc)
{
    int32_t i;
    int32_t window;
    double mad;
    int8_t valid[20];

    if (t->slice_type == SLICE_P)
    {
        mad = ((double)t->sad_all) / ((double)(t->width * t->height));
        // update x1, x2
        for (i = 19 ; i > 0 ; i --)
        {
            rc->mad[i] = rc->mad[i - 1];
        }
        rc->mad[0] = mad;
        window = (int32_t)(mad > rc->mad_p ? rc->mad_p / mad * 20 : mad / rc->mad_p * 20);
        window = clip3(1, rc->p_no + 1, window);
        window = T264_MIN(window, rc->mad_window_p + 1);
        window = T264_MIN(window, 20);
        rc->mad_window_p = window;
        memset(valid, TRUE, sizeof(valid));
        mad_model_estimator(t, valid, window, rc);
        remove_mad_outlier(t, valid, rc);
        mad_model_estimator(t, valid, window, rc);
    }
}

void
quad_model_estimator(T264_t* t, int8_t valid[20], int32_t window, T264_rc_t* rc)
{
    int32_t real_window = 0, i;
    int8_t estimate_x2 = FALSE;
    double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;
    double qp;
    double matrix_value;

    for(i = 0 ; i < window ; i ++)
    {
        if (valid[i])
        {
            real_window ++;
            qp = rc->qp[i];
        }
    }

    rc->x1 = rc->x2 = 0.0;
    for(i = 0 ; i < window ; i ++)
    {
        if (valid[i])
        {
            rc->x1 += rc->qp[i] * rc->rp[i] / real_window;
            if (qp != rc->qp[i])
                estimate_x2 = TRUE;
        }
    }

    if (real_window >= 1 && estimate_x2)
    {
        for(i = 0 ; i < window ; i ++)
        {
            if (valid[i])
            {
                a00 += 1.0;
                a01 += 1.0 / rc->qp[i];
                a10 = a01;
                a11 += 1.0 / (rc->qp[i] * rc->rp[i]);
                b0 += rc->qp[i] * rc->rp[i];
                b1 += rc->rp[i];
            }
        }

        matrix_value = a00 * a11 - a01 * a10;
        if (ABS(matrix_value) > 0.000001)
        {
            rc->x1 = (b0 * a11 - b1 * a01) / matrix_value;
            rc->x2 = (b1 * a00 - b0 * a10) / matrix_value;
        }
        else
        {
            rc->x1 = b0 / a00;
            rc->x2 = 0.0;
        }
    }
}

static void 
rc_update_quad_model(T264_t* t, T264_rc_t* rc)
{
    int32_t i;
    int32_t window;
    double mad;
    int8_t valid[20];

    if (t->slice_type == SLICE_P)
    {
        mad = ((double)t->sad_all) / ((double)(t->width * t->height));
        // update x1, x2
        for (i = 19 ; i > 0 ; i --)
        {
            rc->qp[i] = rc->qp[i - 1];
            rc->rp[i] = rc->rp[i - 1];
        }
        rc->qp[0] = t->qp_y;
        rc->rp[0] = ((double)(t->frame_bits - t->header_bits)) / mad;
        window = (int32_t)(mad > rc->mad_p ? rc->mad_p / mad * 20 : mad / rc->mad_p * 20);
        window = clip3(1, rc->p_no + 1, window);
        window = T264_MIN(window, rc->window_p + 1);
        window = T264_MIN(window, 20);
        rc->window_p = window;
        if (window > 0)
        {
            memset(valid, TRUE, sizeof(valid));
            quad_model_estimator(t, valid, window, rc);
            remove_outlier(t, valid, rc);
            quad_model_estimator(t, valid, window, rc);
        }
        if (rc->p_no > 0)
        {
            // update a1, a2
            rc_update_mad_model(t, rc);
        }
        else
        {
            rc->mad[0] = mad;
        }
        rc->mad_p = mad;
    }
}

static void
rc_proc(T264_t* t, void* data, int32_t state)
{
    T264_rc_t* rc = data;
    switch (state) 
    {
    case STATE_BEFOREPIC:
        rc_init_pic(t, rc);
        break;
    case STATE_AFTERPIC:
        rc_update_pic(t, rc);
        break;
    case STATE_BEFOREGOP:
        rc_init_gop(t, rc);
        break;
    case STATE_BEFORESEQ:
        rc_init_seq(t, rc);
        break;
    default:
        break;
    }
}

void
rc_create(T264_t* t, T264_plugin_t* plugin)
{
    T264_rc_t* handle = malloc(sizeof(T264_rc_t));
    memset(handle, 0, sizeof(T264_rc_t));

    plugin->handle = handle;
    plugin->proc = rc_proc;
    plugin->close = rc_destroy;
    plugin->ret = 0;
}

void
rc_destroy(T264_t* t, T264_plugin_t* plugin)
{
    free(plugin->handle);
}
