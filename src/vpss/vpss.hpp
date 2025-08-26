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

public:
    Hi_Mpp_Vpss();
    ~Hi_Mpp_Vpss();
    bool Init();
};

#endif