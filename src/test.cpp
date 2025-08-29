#include <iostream>
#include "vi.hpp"
#include "vpss.hpp"
int main()
{

    std::cout << "Hello world" << std::endl;
    Hi_Mpp_Vi vi;
    Hi_Mpp_Vpss vpss;
    if (vi.Init() == false)
    {
        return -1;
    }
    if (vpss.Init() == false)
    {
        return -1;
    }
    for (int i = 0; i < 30; i++)
    {
        VIDEO_FRAME_INFO_S stFrameInfom;
        HI_U32 ret = HI_MPI_VPSS_GetChnFrame(0, 0, &stFrameInfom, -1);
        if (HI_SUCCESS != ret)
        {
            std::cout << std::hex << "get frame false  ret = " << ret << std::endl ;
        }
        std::cout << "i = " << i << std::endl;
        ret = HI_MPI_VPSS_ReleaseChnFrame(0, 0, &stFrameInfom);
        if (HI_SUCCESS != ret)
        {
            std::cout << "release frame false\n";
        }
    }




    
    return 0;
}