#ifndef __VI_H_
#define __VI_H_
#include <fcntl.h>      // open
#include <sys/ioctl.h>  // ioctl
#include <unistd.h>     // close
#include <iostream>
#include "hi_common.h"
#include "hi_comm_vi.h"
#include "hi_comm_video.h"
#include "hi_buffer.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "hi_mipi.h"
#define MIPI_DEV_NODE       "/dev/hi_mipi"

class Hi_Mpp_Vi
{
private:
    HI_U32 ret = 0;
    //VI设备号，与MIPI设备绑定关系是固定的,这里我插入的是Hi3516的MIPI1
    VI_DEV dev = 1;
    //宽
    HI_U32 width = 1920;
    //高
    HI_U32 height = 1080;
    //缓存池设置
    VB_CONFIG_S vb_config = {0};
public:
    Hi_Mpp_Vi();
    ~Hi_Mpp_Vi();
    bool Init();
};



#endif