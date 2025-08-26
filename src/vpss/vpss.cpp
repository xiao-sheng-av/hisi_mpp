#include "vpss.hpp"
Hi_Mpp_Vpss::Hi_Mpp_Vpss()
{
}

Hi_Mpp_Vpss::~Hi_Mpp_Vpss()
{
}

bool Hi_Mpp_Vpss::Init()
{
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssGrpAttr.u32MaxW = 1920;
    stVpssGrpAttr.u32MaxH = 1080;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;
    ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VPSS_CreateGrp false!" << ret << std::endl;
    }

    VPSS_CHN_ATTR_S VpssChnAttr;
    VpssChnAttr.u32Width = 1920;
    VpssChnAttr.u32Height = 1080;
    VpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
    VpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;
    VpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    VpssChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
    VpssChnAttr.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VpssChnAttr.stFrameRate.s32SrcFrameRate = 30;
    VpssChnAttr.stFrameRate.s32DstFrameRate = 30;
    //此处要修改为非0
    VpssChnAttr.u32Depth = 4;
    VpssChnAttr.bMirror = HI_FALSE;
    VpssChnAttr.bFlip = HI_FALSE;
    VpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, 0, &VpssChnAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VPSS_SetChnAttr failed!\n";
    }

    ret = HI_MPI_VPSS_EnableChn(VpssGrp, 0);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VPSS_EnableChn failed!\n";
    }

    ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VPSS_StartGrp failed!\n";
    }
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    stSrcChn.enModId   = HI_ID_VI;
    stSrcChn.s32DevId  = 1;
    stSrcChn.s32ChnId  = 0;
    stDestChn.enModId  = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;
    ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_SYS_Bind failed!\n";
    }
    return true;
}