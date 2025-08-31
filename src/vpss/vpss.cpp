#include "vpss.hpp"
Hi_Mpp_Vpss::Hi_Mpp_Vpss()
{
    Out_File = fopen("out.yuv", "wb");
}

Hi_Mpp_Vpss::~Hi_Mpp_Vpss()
{
}

bool Hi_Mpp_Vpss::Init()
{

    VPSS_GRP_ATTR_S stVpssGrpAttr;
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    // 不进行帧率控制
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    // 输入图像动态范围。此处和PIPE重构帧中设置一样，为8bit
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    // 图像格式
    stVpssGrpAttr.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    // 输入图像宽
    stVpssGrpAttr.u32MaxW = 1920;
    // 输入图像高
    stVpssGrpAttr.u32MaxH = 1080;
    // 是否开启降噪
    stVpssGrpAttr.bNrEn = HI_TRUE;
    // NR属性 重构帧的压缩属性
    stVpssGrpAttr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;
    // NR属性 运动矢量模式 NR_MOTION_MODE_NORMAL为普通模式
    stVpssGrpAttr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;
    // 不支持重复创建。
    // 创建一个VPSS组
    ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VPSS_CreateGrp false!" << ret << std::endl;
    }

    VPSS_CHN_ATTR_S VpssChnAttr;
    // 宽
    VpssChnAttr.u32Width = 1920;
    // 高
    VpssChnAttr.u32Height = 1080;
    // 修改为user模式才能从VPSS中获取frame
    VpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
    // 目标图像压缩模式。
    VpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;
    // 目标图像动态范围
    VpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    // 目标图像视频格式。
    VpssChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
    // 输入图像像素格式。静态属性，创建Group时设定，不可更改。
    VpssChnAttr.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    // 组帧率。
    VpssChnAttr.stFrameRate.s32SrcFrameRate = 30;
    VpssChnAttr.stFrameRate.s32DstFrameRate = 30;
    // 此处要修改为大于0才能从VPSS中获取frame
    VpssChnAttr.u32Depth = 4;
    // 水平镜像使能。
    VpssChnAttr.bMirror = HI_FALSE;
    // 垂直翻转功能
    VpssChnAttr.bFlip = HI_FALSE;
    // 设置幅形比的类型。 ASPECT_RATIO_NONE表示不开启幅形比功能。
    VpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    // GROUP 必须已创建。扩展通道不支持此接口。
    ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &VpssChnAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VPSS_SetChnAttr failed!\n";
    }
    // GROUP 必须已创建。
    ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VPSS_EnableChn failed!\n";
    }
    // GROUP 必须已创建。重复调用该函数设置同一个组返回成功。
    ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VPSS_StartGrp failed!\n";
    }
    return true;
}

bool Hi_Mpp_Vpss::Vpss_Bind_Vi(const HI_S32 Pipe_Id, const HI_S32 Chn_Id)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    // VI 和 VDEC 作为数据源，是以通道为发送者，向其他模块发送数据，用户将设备号置为 0，SDK 不检查输入的设备号。
    // 例程中两个都赋值，并没有将dev赋值为0
    stSrcChn.enModId = HI_ID_VI;
    stSrcChn.s32DevId = Pipe_Id;
    stSrcChn.s32ChnId = Chn_Id; // 通道号
    // VPSS 作为数据接收者时，是以设备（GROUP）为接收者，接收其他模块发来的数据，用户将通道号置为 0。
    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp; // VPSS池号
    stDestChn.s32ChnId = 0;       // VPSS通道号
    // 同一个数据接收者只能绑定一个数据源。
    ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_SYS_Bind failed!\n";
        return false;
    }
    return true;
}

bool Hi_Mpp_Vpss::Write_Frame(const VIDEO_FRAME_S *Frame_Info)
{
    HI_U32 Size = (Frame_Info->u32Stride[0]) * (Frame_Info->u32Height) * 3 / 2;
    HI_CHAR *Y_Addr = (HI_CHAR *)HI_MPI_SYS_Mmap(Frame_Info->u64PhyAddr[0], Size);
    HI_CHAR *UV_Addr = Y_Addr + (Frame_Info->u32Stride[0] * Frame_Info->u32Height);
    // 写入Y
    if (Y_Addr == nullptr)
    {
        std::cout << "Y_Addr == nullptr!" << std::endl;
        return false;
    }

    for (int h = 0; h < Frame_Info->u32Height; h++)
    {
        HI_CHAR *Temp = Y_Addr + h * Frame_Info->u32Stride[0];
        fwrite(Temp, Frame_Info->u32Width, 1, Out_File);
    }
    // for (int h = 0; h < Frame_Info->u32Height; h++)
    // {
    //     HI_CHAR * temp = UV_Addr + h * Frame_Info->u32Stride[0];
    //     fwrite(temp, Frame_Info->u32Width, 1, Out_File);
    // }
    HI_MPI_SYS_Munmap(Y_Addr, Size);
    return true;
}