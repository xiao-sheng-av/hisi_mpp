#include <iostream>
#include "vi.hpp"
#include "vpss.hpp"
#include "vo.hpp"
bool Init(Hi_Mpp_Vi & vi, Hi_Mpp_Vpss & vpss, Hi_Mpp_Vo & vo)
{
    HI_S32 Pipe_Id = vi.GetPipeId();
    HI_S32 Chn_Id = vi.GetChnId();
    if (vi.Init() == false)
    {
        std::cout << "vi Init failed\n";
        return false;
    }
    if (vpss.Init() == false)
    {
        std::cout << "vpss Init failed\n";
        return false;
    }
    if (vpss.Bind_Vi(Pipe_Id, Chn_Id) == false)
    {
        std::cout << "vpss bind vi failed\n";
        return false;
    }
    if (vo.Init() != HI_SUCCESS)
    {
        std::cout << "vo Init failed\n";
        return false;
    }
    if (vo.Bind_Vpss(vpss.Get_Grp(), vpss.Get_ChnId()) != true)
    {
        std::cout << "vpss bind vo false\n";
    }
    return true;
}
int main()
{

    std::cout << "Hello world" << std::endl;
    Hi_Mpp_Vi vi;
    Hi_Mpp_Vpss vpss;
    Hi_Mpp_Vo vo;
    HI_S32 Vpss_Chn = vpss.Get_ChnId();
    HI_S32 Vpss_Grp = vpss.Get_Grp();
    if (Init(vi, vpss, vo) == false)
    {
        std::cout << "Init false\n";
        return -1;
    }

    getchar();

    
    return 0;
}