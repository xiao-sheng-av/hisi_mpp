#ifndef __VO_H_
#define __VO_H_
#include <iostream>
#include "hi_common.h"
#include "mpi_vo.h"
#include "mpi_hdmi.h"
#include "mpi_sys.h"
#define ALIGN_DOWN(x, a)         ( ( (x) / (a)) * (a) )
class Hi_Mpp_Vo
{
private:
    // 3516dv300只支持输出设备0，在hi_defines.h中有定义
    VO_DEV VoDev = 0;
    HI_U32 ret;
    // 3516dv300只支持视频层0
    VO_LAYER VoLayer = 0;
    VO_CHN VoChn = 2;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

public:
    Hi_Mpp_Vo();
    ~Hi_Mpp_Vo();
    bool Init();
    bool Bind_Vpss(const HI_S32 Vpss_Grp, const HI_S32 Vpss_Chn);
    HI_U32 UnBind_Vpss();
};


#endif