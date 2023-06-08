/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FMR_H__
#define __FMR_H__

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <dlfcn.h>

#include "fm.h"

#undef LOGV
#define LOGV(...) ALOGV(__VA_ARGS__)
#undef LOGD
#define LOGD(...) ALOGD(__VA_ARGS__)
#undef LOGI
#define LOGI(...) ALOGI(__VA_ARGS__)
#undef LOGW
#define LOGW(...) ALOGW(__VA_ARGS__)
#undef LOGE
#define LOGE(...) ALOGE(__VA_ARGS__)


#define CUST_LIB_NAME "libfmcust.so"
#define FM_DEV_NAME "/dev/fm"
#define HEADSET_STATE_PATH "/sys/class/switch/h2w/state"

#define FM_RDS_PS_LEN 8

struct fm_fake_channel
{
    int freq;
    int rssi_th;
    int reserve;
};

struct fm_fake_channel_t
{
    int size;
    struct fm_fake_channel *chan;
};

struct CUST_cfg_ds
{
    int16_t chip;
    int32_t band;
    int32_t low_band;
    int32_t high_band;
    int32_t seek_space;
    int32_t max_scan_num;
    int32_t seek_lev;
    int32_t scan_sort;
    int32_t short_ana_sup;
    int32_t rssi_th_l2;
    struct fm_fake_channel_t *fake_chan;
};

struct fm_cbk_tbl {
    //Basic functions.
    int (*open_dev)(const char *pname, int *fd);
    int (*close_dev)(int fd);
    int (*pwr_up)(int fd, int band, int freq);
    int (*pwr_down)(int fd, int type);
    int (*seek)(int fd, int *freq, int band, int dir, int lev);
    int (*scan)(int fd, uint16_t *tbl, int *num, int band, int sort);
    int (*fastget_rssi)(int fd, struct fm_rssi_req *rssi_req);
    int (*get_cqi)(int fd, int num, char *buf, int buf_len);
    int (*stop_scan)(int fd);
    int (*tune)(int fd, int freq, int band);
    int (*set_mute)(int fd, int mute);
    int (*is_fm_pwrup)(int fd, int *pwrup);
    int (*is_rdsrx_support)(int fd, int *supt);
    int (*turn_on_off_rds)(int fd, int onoff);
    int (*get_chip_id)(int fd, int *chipid);
    //FOR RDS RX.
    int (*read_rds_data)(int fd, RDSData_Struct *rds, uint16_t *rds_status);
    int (*get_pi)(int fd, RDSData_Struct *rds, uint16_t *pi);
    int (*get_ps)(int fd, RDSData_Struct *rds, uint8_t **ps, int *ps_len);
    int (*get_pty)(int fd, RDSData_Struct *rds, uint8_t *pty);
    int (*get_rssi)(int fd, int *rssi);
    int (*get_rt)(int fd, RDSData_Struct *rds, uint8_t **rt, int *rt_len);
    int (*active_af)(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *ret_freq);
    int (*active_ta)(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *backup_freq, uint16_t *ret_freq);
    int (*deactive_ta)(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *backup_freq, uint16_t *ret_freq);
    //FM long/short antenna switch
    int (*ana_switch)(int fd, int antenna);
    //For FM RX EM mode
    int (*get_badratio)(int fd, int *badratio);
    int (*get_stereomono)(int fd, int *stemono);
    int (*set_stereomono)(int fd, int stemono);
    int (*get_caparray)(int fd, int *caparray);
    int (*get_hw_info)(int fd, struct fm_hw_info *info);
    int (*is_dese_chan)(int fd, int freq);
    int (*soft_mute_tune)(int fd, fm_softmute_tune_t *para);
    int (*desense_check)(int fd, int freq, int rssi);
    int (*set_search_threshold)(int fd, int th_idx,int th_val);
    int (*full_cqi_logger)(int fd,fm_full_cqi_log_t *log_parm);
    /*New search*/
    int (*pre_search)(int fd);
    int (*restore_search)(int fd);
};

typedef int (*CUST_func_type)(struct CUST_cfg_ds *);
typedef void (*init_func_type)(struct fm_cbk_tbl *);

struct fmr_ds {
    int fd;
    int err;
    uint16_t cur_freq;
    uint16_t backup_freq;
    void *priv;
    void *custom_handler;
    struct CUST_cfg_ds cfg_data;
    struct fm_cbk_tbl tbl;
    CUST_func_type get_cfg;
    void *init_handler;
    init_func_type init_func;
    RDSData_Struct rds;
    struct fm_hw_info hw_info;
    fm_bool scan_stop;
};

enum fmr_err_em {
    ERR_SUCCESS = 1000, // kernel error begin at here
    ERR_INVALID_BUF,
    ERR_INVALID_PARA,
    ERR_STP,
    ERR_GET_MUTEX,
    ERR_FW_NORES,
    ERR_RDS_CRC,
    ERR_INVALID_FD, //  native error begin at here
    ERR_UNSUPPORT_CHIP,
    ERR_LD_LIB,
    ERR_FIND_CUST_FNUC,
    ERR_UNINIT,
    ERR_NO_MORE_IDX,
    ERR_RDS_NO_DATA,
    ERR_UNSUPT_SHORTANA,
    ERR_MAX
};

enum fmr_rds_onoff {
    FMR_RDS_ON,
    FMR_RDS_OFF,
    FMR_MAX
};

typedef enum {
    FM_LONG_ANA = 0,
    FM_SHORT_ANA
} fm_antenna_type;


#define CQI_CH_NUM_MAX 255
#define CQI_CH_NUM_MIN 0


/****************** Function declaration ******************/
//fmr_err.cpp
char *FMR_strerr();
void FMR_seterr(int err);

//fmr_core.cpp
int FMR_init(void);
int FMR_get_cfgs(int idx);
int FMR_open_dev(int idx);
int FMR_close_dev(int idx);
int FMR_pwr_up(int idx, int freq);
int FMR_pwr_down(int idx, int type);
int FMR_seek(int idx, int start_freq, int dir, int *ret_freq);
int FMR_scan(int idx, uint16_t *tbl, int *num);
int FMR_stop_scan(int idx);
int FMR_tune(int idx, int freq);
int FMR_set_mute(int idx, int mute);
int FMR_is_fm_pwrup(int idx, int *pwrup);
int FMR_is_rdsrx_support(int idx, int *supt);
int FMR_turn_on_off_rds(int idx, int onoff);
int FMR_get_chip_id(int idx, int *chipid);
int FMR_read_rds_data(int idx, uint16_t *rds_status);
int FMR_get_pi(int idx, uint16_t *pi);
int FMR_get_ps(int idx, uint8_t **ps, int *ps_len);
int FMR_get_pty(int idx, uint8_t *pty);
int FMR_get_rssi(int idx, int *rssi);
int FMR_get_rt(int idx, uint8_t **rt, int *rt_len);
int FMR_active_af(int idx, uint16_t *ret_freq);
int FMR_active_ta(int idx, uint16_t *ret_freq);
int FMR_deactive_ta(int idx, uint16_t *ret_freq);

int FMR_ana_switch(int idx, int antenna);
int FMR_ana_switch_inner(int idx);
int FMR_get_badratio(int idx, int *badratio);
int FMR_get_stereomono(int idx, int *stemono);
int FMR_set_stereomono(int idx, int stemono);
int FMR_get_caparray(int idx, int *caparray);
int FMR_get_hw_info(int idx, int **info, int *info_len);
int FMR_Pre_Search(int idx);
int FMR_Restore_Search(int idx);
int FMR_EMSetTH(int idx, int th_idx, int th_val);
int FMR_EM_CQI_logger(int idx,uint16_t cycle);
void FM_interface_init(struct fm_cbk_tbl *cbk_tbl);

//common part
int COM_open_dev(const char *pname, int *fd);
int COM_close_dev(int fd);
int COM_pwr_up(int fd, int band, int freq);
int COM_pwr_down(int fd, int type);
int COM_seek(int fd, int *freq, int band, int dir, int lev);
int COM_Soft_Mute_Tune(int fd, fm_softmute_tune_t *para);
int COM_fastget_rssi(int fd, struct fm_rssi_req *rssi_req);
int COM_get_cqi(int fd, int num, char *buf, int buf_len);
int COM_stop_scan(int fd);
int COM_tune(int fd, int freq, int band);
int COM_set_mute(int fd, int mute);
int COM_is_fm_pwrup(int fd, int *pwrup);
int COM_is_rdsrx_support(int fd, int *supt);
int COM_turn_on_off_rds(int fd, int onoff);
int COM_get_chip_id(int fd, int *chipid);
int COM_read_rds_data(int fd, RDSData_Struct *rds, uint16_t *rds_status);
int COM_get_pi(int fd, RDSData_Struct *rds, uint16_t *pi);
int COM_get_ps(int fd, RDSData_Struct *rds, uint8_t **ps, int *ps_len);
int COM_get_pty(int fd, RDSData_Struct *rds, uint8_t *pty);
int COM_get_rssi(int fd, int *rssi);
int COM_get_rt(int fd, RDSData_Struct *rds, uint8_t **rt, int *rt_len);
int COM_active_af(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *ret_freq);
int COM_active_ta(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *backup_freq, uint16_t *ret_freq);
int COM_deactive_ta(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *backup_freq, uint16_t *ret_freq);
int COM_ana_switch(int fd, int antenna);
int COM_get_badratio(int fd, int *badratio);
int COM_get_stereomono(int fd, int *stemono);
int COM_set_stereomono(int fd, int stemono);
int COM_get_caparray(int fd, int *caparray);
int COM_get_hw_info(int fd, struct fm_hw_info *info);
int COM_is_dese_chan(int fd, int freq);
int COM_desense_check(int fd, int freq, int rssi);
int COM_pre_search(int fd);
int COM_restore_search(int fd);
int COM_set_search_threshold(int fd, int th_idx,int th_val);
int COM_full_cqi_logger(int fd, fm_full_cqi_log_t *log_parm);

#define FMR_ASSERT(a) { \
    if ((a) == NULL) { \
        LOGE("%s,invalid buf\n", __func__);\
        return -ERR_INVALID_BUF; \
    } \
}
#endif

