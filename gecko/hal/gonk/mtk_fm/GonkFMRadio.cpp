/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Hal.h"
#include "HalLog.h"
#include "../tavarua.h"
#include "nsThreadUtils.h"
#include "mozilla/FileUtils.h"

#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fmr.h"

/* Bionic might not have the newer version of the v4l2 headers that
 * define these controls, so we define them here if they're not found.
 */
#ifndef V4L2_CTRL_CLASS_FM_RX
#define V4L2_CTRL_CLASS_FM_RX 0x00a10000
#define V4L2_CID_FM_RX_CLASS_BASE (V4L2_CTRL_CLASS_FM_RX | 0x900)
#define V4L2_CID_TUNE_DEEMPHASIS  (V4L2_CID_FM_RX_CLASS_BASE + 1)
#define V4L2_DEEMPHASIS_DISABLED  0
#define V4L2_DEEMPHASIS_50_uS     1
#define V4L2_DEEMPHASIS_75_uS     2
#define V4L2_CID_RDS_RECEPTION    (V4L2_CID_FM_RX_CLASS_BASE + 2)
#endif

#ifndef V4L2_RDS_BLOCK_MSK
struct v4l2_rds_data {
  uint8_t lsb;
  uint8_t msb;
  uint8_t block;
} __attribute__ ((packed));
#define V4L2_RDS_BLOCK_MSK 0x7
#define V4L2_RDS_BLOCK_A 0
#define V4L2_RDS_BLOCK_B 1
#define V4L2_RDS_BLOCK_C 2
#define V4L2_RDS_BLOCK_D 3
#define V4L2_RDS_BLOCK_C_ALT 4
#define V4L2_RDS_BLOCK_INVALID 7
#define V4L2_RDS_BLOCK_CORRECTED 0x40
#define V4L2_RDS_BLOCK_ERROR 0x80
#endif

namespace mozilla {
namespace hal_impl {

uint32_t GetFMRadioFrequency();

static int sRadioFD = -1;
static bool sRadioEnabled;
static bool sRDSEnabled;
static pthread_t sRadioThread;
static pthread_t sRDSThread;
static hal::FMRadioSettings sRadioSettings;
static bool sRDSSupported;
static unsigned short sFrequency = 899;//975;//89.9
static int g_idx = -1;
extern struct fmr_ds fmr_data;
static int
setControl(uint32_t id, int32_t value)
{
  struct v4l2_control control = {0};
  control.id = id;
  control.value = value;
  return ioctl(sRadioFD, VIDIOC_S_CTRL, &control);
}

class RadioUpdate : public nsRunnable {
  hal::FMRadioOperation mOp;
  hal::FMRadioOperationStatus mStatus;
public:
  RadioUpdate(hal::FMRadioOperation op, hal::FMRadioOperationStatus status)
    : mOp(op)
    , mStatus(status)
  {
  	HAL_LOG("op %d,status %d",op,status);
  }

  NS_IMETHOD Run() {
    hal::FMRadioOperationInformation info;
    info.operation() = mOp;
    info.status() = mStatus;
    info.frequency() = GetFMRadioFrequency();
    hal::NotifyFMRadioStatus(info);
    return NS_OK;
  }
};

/* Runs on the radio thread */
static void
initMsmFMRadio(hal::FMRadioSettings &aInfo)
{
  HAL_LOG("initMsmFMRadio");

  if ((g_idx = FMR_init()) < 0) {
    HAL_LOG("FMR_init fail");
  }

  HAL_LOG("FMR_init success,g_idx %d",g_idx);
  return ;
}

/* Runs on the radio thread */
static void *
runMsmFMRadio(void *)
{
  HAL_LOG("%d,runMsmFMRadio,sRadioEnabled %d",__LINE__,sRadioEnabled);

  int rc =-1;
  HAL_LOG("enter,g_idx %d",g_idx);
  if(g_idx < 0) initMsmFMRadio(sRadioSettings);
	HAL_LOG("g_idx %d ",g_idx);

  if(FMR_open_dev(g_idx) != FM_SUCCESS)
  {
    HAL_LOG("FMR_open_dev fail ");
    return nullptr;
  }

  FMR_ana_switch_inner(g_idx);

  HAL_LOG("g_idx %d, sFrequency %d",g_idx,sFrequency);
  rc = FMR_pwr_up(g_idx, sFrequency);

  sRadioEnabled = (rc == FM_SUCCESS);
  int support = -1;
  FMR_is_rdsrx_support(g_idx, &support);
  sRDSSupported = (support == 1);

  if (!sRadioEnabled) {
    NS_DispatchToMainThread(new RadioUpdate(hal::FM_RADIO_OPERATION_ENABLE,
                                            hal::FM_RADIO_OPERATION_STATUS_FAIL));
    return nullptr;
  }

  HAL_LOG("when open following line, press play/pause key, FM will change to next clearness channel");
  NS_DispatchToMainThread(new RadioUpdate(hal::FM_RADIO_OPERATION_ENABLE,
                                            hal::FM_RADIO_OPERATION_STATUS_SUCCESS));
  HAL_LOG("runMsmFMRadio finish\n");
  return nullptr;
}

/* This runs on the main thread but most of the
 * initialization is pushed to the radio thread. */
void
EnableFMRadio(const hal::FMRadioSettings& aInfo)
{
  if (sRadioEnabled) {
    HAL_LOG("%d,EnableFMRadio,Radio already enabled!",__LINE__);
    return;
  }

  HAL_LOG("At %d In (EnableFMRadio),sRadioEnabled %d",__LINE__,sRadioEnabled);

  hal::FMRadioOperationInformation info;
  info.operation() = hal::FM_RADIO_OPERATION_ENABLE;
  info.status() = hal::FM_RADIO_OPERATION_STATUS_FAIL;

  sRadioSettings = aInfo;
  {
    if (pthread_create(&sRadioThread, nullptr, runMsmFMRadio, nullptr)) {
      HAL_LOG("Couldn't create radio thread");
      hal::NotifyFMRadioStatus(info);
	  return;
    }
  }
  HAL_LOG("At %d In (EnableFMRadio),exit",__LINE__);
  return;
}

void
DisableFMRadio()
{
  HAL_LOG("%d,DisableFMRadio,sRadioEnabled %d",__LINE__,sRadioEnabled);
  if (!sRadioEnabled)
    return;

  if (sRDSEnabled)
    hal::DisableRDS();

  int type = 0;
  hal::FMRadioOperationInformation info;
  info.operation() = hal::FM_RADIO_OPERATION_DISABLE;
  info.status() = hal::FM_RADIO_OPERATION_STATUS_SUCCESS;
  int rc = -1;
  if( (rc = FMR_pwr_down(g_idx, type)) != FM_SUCCESS )
    {
      HAL_LOG("Unable to powerdown radio rc=%d", rc);
      info.status() = hal::FM_RADIO_OPERATION_STATUS_FAIL;
    }

  if( (rc = FMR_close_dev(g_idx)) != FM_SUCCESS )
    {
      HAL_LOG("Unable to turn off radio rc=%d", rc);
      info.status() = hal::FM_RADIO_OPERATION_STATUS_FAIL;
    }

  sRadioEnabled = false;
  hal::NotifyFMRadioStatus(info);
  pthread_join(sRadioThread, nullptr);

  HAL_LOG("%d,DisableFMRadio",__LINE__);
}

void
FMRadioSeek(const hal::FMRadioSeekDirection& aDirection)
{
  int ret = -1;
  int tmp_freq;
  int ret_freq;
  HAL_LOG("aDirection=%d", aDirection);

  bool isSeekUp = aDirection == hal::FMRadioSeekDirection::FM_RADIO_SEEK_DIRECTION_UP;
  HAL_LOG("aDirection %d,sFrequency=%d", aDirection,sFrequency);
  tmp_freq = sFrequency * 10;

  ret = FMR_seek(g_idx,tmp_freq, isSeekUp, &ret_freq);
  if(ret >= 0)
  {
    sFrequency = ret_freq / 10;
    HAL_LOG("sFrequency=%d", sFrequency);
  }else{
    HAL_LOG("error,Could not initiate hardware seek");
  }

  NS_DispatchToMainThread(new RadioUpdate(hal::FM_RADIO_OPERATION_SEEK,
                                          ret < 0 ?
                                          hal::FM_RADIO_OPERATION_STATUS_FAIL :
                                          hal::FM_RADIO_OPERATION_STATUS_SUCCESS));
  HAL_LOG("exit");
  return;
}

void
GetFMRadioSettings(hal::FMRadioSettings* aInfo)
{
  if (!sRadioEnabled) {
    return;
  }

  HAL_LOG("At %d In(GetFMRadioSettings),",__LINE__);
  aInfo->upperLimit() = FM_UE_FREQ_MAX;
  aInfo->lowerLimit() = FM_UE_FREQ_MIN;
}

void
SetFMRadioFrequency(const uint32_t frequency)
{

  HAL_LOG("%d,SetFMRadioFrequency,frequency %d",__LINE__,frequency);
  hal::FMRadioOperationInformation info;
  info.operation() = hal::FM_RADIO_OPERATION_DISABLE;
  info.status() = hal::FM_RADIO_OPERATION_STATUS_SUCCESS;

  int rc = -1;
  int iFre = (frequency/100);
  if( iFre < FM_UE_FREQ_MIN) iFre = FM_UE_FREQ_MIN;
  if (iFre > FM_UE_FREQ_MAX) iFre = FM_UE_FREQ_MAX;
   HAL_LOG("SetFMRadioFrequency= %d, iFre=%d\n", frequency, iFre);

  rc = FMR_tune(g_idx, iFre);
  if (rc < 0)
  {
	HAL_LOG("Could not set radio frequency,rc=%d", rc);
  }else
  {
    sFrequency = (frequency/100);
    HAL_LOG("set radio frequency success!,sFrequency %d",sFrequency);
  }
  HAL_LOG("%d,SetFMRadioFrequency",__LINE__);

  NS_DispatchToMainThread(new RadioUpdate(hal::FM_RADIO_OPERATION_TUNE,
                                          rc < 0 ?
                                          hal::FM_RADIO_OPERATION_STATUS_FAIL :
                                          hal::FM_RADIO_OPERATION_STATUS_SUCCESS));
  return;
}

uint32_t
GetFMRadioFrequency()
{
  if (!sRadioEnabled)
    return 0;

  HAL_LOG("In(GetFMRadioFrequency), sFrequency %d, return %d",sFrequency,sFrequency *100);
  return sFrequency *100;
}

bool
IsFMRadioOn()
{
  return sRadioEnabled;
}

uint32_t
GetFMRadioSignalStrength()
{
  HAL_LOG("In(GetFMRadioSignalStrength),not support signalStrength temporary");
  return 99;
}

void
CancelFMRadioSeek()
{}

/* Runs on the rds thread */
static void*
readRDSDataThread(void* data)
{
  v4l2_rds_data rdsblocks[16];
  uint16_t blocks[4];

  ScopedClose pipefd((int)data);

  ScopedClose epollfd(epoll_create(2));
  if (epollfd < 0) {
    HAL_LOG("Could not create epoll FD for RDS thread (%d)", errno);
    return nullptr;
  }

  epoll_event event = {
    EPOLLIN,
    { 0 }
  };

  event.data.fd = pipefd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefd, &event) < 0) {
    HAL_LOG("Could not set up epoll FD for RDS thread (%d)", errno);
    return nullptr;
  }

  event.data.fd = sRadioFD;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sRadioFD, &event) < 0) {
    HAL_LOG("Could not set up epoll FD for RDS thread (%d)", errno);
    return nullptr;
  }

  epoll_event events[2] = {{ 0 }};
  int event_count;
  uint32_t block_bitmap = 0;
  while ((event_count = epoll_wait(epollfd, events, 2, -1)) > 0 ||
         errno == EINTR) {
    bool RDSDataAvailable = false;
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == pipefd) {
        if (!sRDSEnabled)
          return nullptr;
        char tmp[32];
        TEMP_FAILURE_RETRY(read(pipefd, tmp, sizeof(tmp)));
      } else if (events[i].data.fd == sRadioFD) {
        RDSDataAvailable = true;
      }
    }

    if (!RDSDataAvailable)
      continue;

    ssize_t len =
      TEMP_FAILURE_RETRY(read(sRadioFD, rdsblocks, sizeof(rdsblocks)));
    if (len < 0) {
      HAL_LOG("Unexpected error while reading RDS data %d", errno);
      return nullptr;
    }

    int blockcount = len / sizeof(rdsblocks[0]);
    for (int i = 0; i < blockcount; i++) {
      if ((rdsblocks[i].block & V4L2_RDS_BLOCK_MSK) == V4L2_RDS_BLOCK_INVALID ||
           rdsblocks[i].block & V4L2_RDS_BLOCK_ERROR) {
        block_bitmap |= 1 << V4L2_RDS_BLOCK_INVALID;
        continue;
      }

      int blocknum = rdsblocks[i].block & V4L2_RDS_BLOCK_MSK;
      // In some cases, the full set of bits in an RDS group isn't
      // needed, in which case version B RDS groups can be sent.
      // Version B groups replace block C with block C' (V4L2_RDS_BLOCK_C_ALT).
      // Block C' always stores the PI code, so receivers can find the PI
      // code more quickly/reliably.
      // However, we only process whole RDS groups, so it doesn't matter here.
      if (blocknum == V4L2_RDS_BLOCK_C_ALT)
        blocknum = V4L2_RDS_BLOCK_C;
      if (blocknum > V4L2_RDS_BLOCK_D) {
        HAL_LOG("Unexpected RDS block number %d. This is a driver bug.",
                blocknum);
        continue;
      }

      if (blocknum == V4L2_RDS_BLOCK_A)
        block_bitmap = 0;

      // Skip the group if we skipped a block.
      // This stops us from processing blocks sent out of order.
      if (block_bitmap != ((1u << blocknum) - 1u)) {
        block_bitmap |= 1 << V4L2_RDS_BLOCK_INVALID;
        continue;
      }

      block_bitmap |= 1 << blocknum;

      blocks[blocknum] = (rdsblocks[i].msb << 8) | rdsblocks[i].lsb;

      // Make sure we have all 4 blocks and that they're valid
      if (block_bitmap != 0x0F)
        continue;

      hal::FMRadioRDSGroup group;
      group.blockA() = blocks[V4L2_RDS_BLOCK_A];
      group.blockB() = blocks[V4L2_RDS_BLOCK_B];
      group.blockC() = blocks[V4L2_RDS_BLOCK_C];
      group.blockD() = blocks[V4L2_RDS_BLOCK_D];
      NotifyFMRadioRDSGroup(group);
    }
  }

  return nullptr;
}

static int sRDSPipeFD;

bool
EnableRDS(uint32_t aMask)
{
  HAL_LOG("At %d In (EnableRDS),entry",__LINE__);
  if (!sRadioEnabled || !sRDSSupported)
    return false;

  if (sRDSEnabled)
    return true;

  int pipefd[2];
  int rc = pipe2(pipefd, O_NONBLOCK);
  if (rc < 0) {
    HAL_LOG("Could not create RDS thread signaling pipes (%d)", rc);
    return false;
  }

  ScopedClose writefd(pipefd[1]);
  ScopedClose readfd(pipefd[0]);

  unsigned char rds_on =1;
  rc = ioctl(sRadioFD, FM_IOCTL_RDS_ONOFF, &rds_on);
  if (rc < 0) {
      HAL_LOG("Unable to turn off radio RDS");
  }

  sRDSPipeFD = writefd;

  sRDSEnabled = true;

  rc = pthread_create(&sRDSThread, nullptr,
                      readRDSDataThread, (void*)pipefd[0]);
  if (rc) {
    HAL_LOG("Could not start RDS reception thread (%d)", rc);
    setControl(V4L2_CID_RDS_RECEPTION, false);
    sRDSEnabled = false;
    return false;
  }

  HAL_LOG("exit");
  readfd.forget();
  writefd.forget();
  return true;
}

void
DisableRDS()
{
  if (!sRadioEnabled || !sRDSEnabled)
    return;
  unsigned char rds_on =0;
  int rc = ioctl(sRadioFD, FM_IOCTL_RDS_ONOFF, &rds_on);
  if (rc < 0) {
      HAL_LOG("Unable to turn off radio RDS");
  }

  sRDSEnabled = false;

  write(sRDSPipeFD, "x", 1);

  pthread_join(sRDSThread, nullptr);

  close(sRDSPipeFD);
}

void SetFMRadioAntenna(const int32_t status)
{
  int32_t ret = -1;
  if(false == sRadioEnabled) {
    HAL_LOG("%s, FM is disabled currently",__FUNCTION__);
    return;
  }

  if((status != hal::SWITCH_STATE_OFF) && (status != hal::SWITCH_STATE_HEADSET) && (status != hal::SWITCH_STATE_HEADPHONE)) {
    HAL_LOG("%s, Error: wrong status of headphone %d",__FUNCTION__,status);
    return;
  }

  ret = FMR_ana_switch_inner(g_idx);
  if(ret < 0)
    HAL_LOG("%s, Error: Anatenna Switch Fail, ret %d",__FUNCTION__,ret);
}

} // hal_impl
} // namespace mozilla
