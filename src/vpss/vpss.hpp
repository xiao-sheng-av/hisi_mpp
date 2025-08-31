#ifndef __VPSS_H_
#define __VPSS_H_
#include <iostream>
#include "mpi_vpss.h"
#include "mpi_sys.h"
class Hi_Mpp_Vpss
{
private:
    HI_U32 ret = -1;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;
    FILE *Out_File;
public:
    Hi_Mpp_Vpss();
    ~Hi_Mpp_Vpss();
    bool Init();
    bool Vpss_Bind_Vi(const HI_S32 Pipe_Id, const HI_S32 Chn_Id);
    HI_S32 Get_ChnId() const { return VpssChn;}
    HI_S32 Get_Grp() const { return VpssGrp;}
    bool Write_Frame(const VIDEO_FRAME_S *Frame_Info);
};

#endif