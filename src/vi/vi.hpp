#ifndef __VI_H_
#define __VI_H_
#include <fcntl.h>      // open
#include <sys/ioctl.h>  // ioctl
#include <unistd.h>     // close
#include <iostream>
#include <thread>
#include "hi_common.h"
#include "hi_comm_vi.h"
#include "hi_comm_video.h"
#include "hi_buffer.h"
#include "hi_comm_vb.h"
#include "mpi_isp.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "hi_mipi.h"
#include "hi_sns_ctrl.h"
#include "hi_ae_comm.h"
#include "hi_awb_comm.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#define MIPI_DEV_NODE       "/dev/hi_mipi"

class Hi_Mpp_Vi
{
private:
    HI_U32 ret = 0;
    //VI设备号，DEV和MIPI是一一对应的
    VI_DEV Dev = 1;
    //pipe号，PIPE和DEV不是一一对应的
    VI_PIPE Pipe_Id = 0;
    //管道号, 3516dv300只有通道0
    VI_CHN Chn_Id = 0;
    //宽
    HI_U32 Width = 1920;
    //高
    HI_U32 Height = 1080;
    //缓存池设置
    VB_CONFIG_S Vb_Config = {0};
    // Ae为自动曝光库，Awb为自动白平衡库
    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    const ISP_SNS_OBJ_S *pstSnsObj;
    //isp运行线程
    std::thread isp_thread;
public:
    Hi_Mpp_Vi();
    ~Hi_Mpp_Vi();
    bool Init();
    HI_U32 Mipi_Init();
    HI_U32 Mipi_DeInit();
    void isp_stop();
    HI_S32 GetPipeId() { return Pipe_Id; }
    HI_S32 GetChnId() { return Chn_Id; }
    void SetPipeId(HI_S32 id) { Pipe_Id = id; }
};



#endif