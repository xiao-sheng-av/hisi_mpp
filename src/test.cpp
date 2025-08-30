#include <iostream>
#include "vi.hpp"
#include "vpss.hpp"
bool Init(Hi_Mpp_Vi & vi, Hi_Mpp_Vpss & vpss)
{
    HI_S32 Pipe_Id = vi.GetPipeId();
    HI_S32 Chn_Id = vi.GetChnId();
    if (!vi.Init())
    {
        std::cout << "vi Init failed\n";
        return false;
    }
    if (!vpss.Init())
    {
        std::cout << "vpss Init failed\n";
        return false;
    }
    if (!vpss.Vpss_Bind_Vi(Pipe_Id, Chn_Id))
    {
        std::cout << "vpss bind vi failed\n";
        return false;
    }
    return true;
}
int main()
{

    std::cout << "Hello world" << std::endl;
    Hi_Mpp_Vi vi;
    Hi_Mpp_Vpss vpss;
    Init(vi, vpss);
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