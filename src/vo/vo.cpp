#include "vo.hpp"

Hi_Mpp_Vo::Hi_Mpp_Vo()
{


}

Hi_Mpp_Vo::~Hi_Mpp_Vo()
{
    HI_HDMI_ID_E enHdmiId = HI_HDMI_ID_0;
    // 停止指定的HDMI接口，不使能其送显能力。hi_mpi_hdmi_stop接口被调用前必须先调用hi_mpi_hdmi_open接口来打开对应的HDMI，且需要保持两个接口入参中的hi_hdmi_id参数值一致。不支持多进程
    HI_MPI_HDMI_Stop(enHdmiId);
    // 关闭指定的HDMI接口。调用hi_mpi_hdmi_close接口之前必须先调用hi_mpi_hdmi_open接口，且需要保持两个接口入参中的hi_hdmi_id参数值一致。不支持多进程
    HI_MPI_HDMI_Close(enHdmiId);
    // 反初始化HDMI。本接口需要与hi_mpi_hdmi_init接口成对调用，即hi_mpi_hdmi_init函数被调用后，才允许调用hi_mpi_hdmi_deinit接口，且保证二者调用次数相同。不支持多线程
    HI_MPI_HDMI_DeInit();
    // 禁用指定的视频输出通道。当高清设备的通道绑定VPSS（Video Processing Subsystem）时，建议先调用本接口停止通道后，再解绑定VO通道与VPSS通道的绑定关系，否则可能出现HI_ERR_VO_BUSY的超时返回错误
    // 与HI_MPI_VO_EnableChn接口成对使用
    ret = HI_MPI_VO_DisableChn(VoLayer, 0);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_DisableChn failed!\n";
    }
    // 禁用视频层。视频层禁用前必须保证其上的通道全部禁用(HI_MPI_VO_DisableChn)。
    ret = HI_MPI_VO_DisableVideoLayer(VoLayer);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_DisableVideoLayer failed!\n";
    }

    ret = HI_MPI_VO_Disable(Vo_Dev);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_Disable failed!\n";
    }
    ret = UnBind_Vpss();
    if (ret != HI_SUCCESS)
    {
        std::cout << "UnBind_Vpss failed!\n";
    }
}

bool Hi_Mpp_Vo::Init()
{
    VO_PUB_ATTR_S PubAttr;
    memset(&PubAttr, 0, sizeof(VO_PUB_ATTR_S));
    PubAttr.enIntfSync = VO_OUTPUT_1080P60;
    PubAttr.enIntfType = VO_INTF_HDMI;
    PubAttr.u32BgColor = 0x0000FF; // 背景色，表示办法为RGB888颜色表示法
    ret = HI_MPI_VO_SetPubAttr(Vo_Dev, &PubAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetPubAttr failed!\n";
        return false;
    }

    ret = HI_MPI_VO_Enable(Vo_Dev);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_Enable failed!\n";
        return false;
    }

    VO_VIDEO_LAYER_ATTR_S LayerAttr;
    memset(&LayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));
    // stdisprect表示显示的区域大小
    LayerAttr.stDispRect.u32Width = 1920;
    LayerAttr.stDispRect.u32Height = 1080;
    LayerAttr.stDispRect.s32X = 0;
    LayerAttr.stDispRect.s32Y = 0;
    // 帧率
    LayerAttr.u32DispFrmRt = 60;
    // 视频层上的通道是否采用聚集的方式使用内存
    LayerAttr.bClusterMode = HI_FALSE;
    // 视频层是否开启倍帧
    LayerAttr.bDoubleFrame = HI_FALSE;
    // 视频层输入像素格式
    LayerAttr.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    // 设置原图像的属性
    LayerAttr.stImageSize.u32Width = LayerAttr.stDispRect.u32Width;
    LayerAttr.stImageSize.u32Height = LayerAttr.stDispRect.u32Height;
    // 指定视频层输出动态范围类型
    LayerAttr.enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    HI_U32 Buf = 3;
    ret = HI_MPI_VO_SetDisplayBufLen(VoLayer, Buf);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VO_SetDisplayBufLen failed!\n";
        return false;
    }
    ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, &LayerAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetVideoLayerAttr failed!\n";
        return false;
    }

    ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_EnableVideoLayer failed!\n";
        return false;
    }

    ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &LayerAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_GetVideoLayerAttr failed!\n";
        return false;
    }

    VO_CHN_ATTR_S ChnAttr;
    memset(&ChnAttr, 0, sizeof(VO_CHN_ATTR_S));
    ChnAttr.stRect.s32X = ALIGN_DOWN((LayerAttr.stImageSize.u32Width / 1) * (0 % 1), 2);
    ChnAttr.stRect.s32Y = ALIGN_DOWN((LayerAttr.stImageSize.u32Height / 1) * (0 / 1), 2);
    ChnAttr.stRect.u32Width = ALIGN_DOWN(LayerAttr.stImageSize.u32Width / 1, 2);
    ChnAttr.stRect.u32Height = ALIGN_DOWN(LayerAttr.stImageSize.u32Height / 1, 2);
    ChnAttr.u32Priority = 0;
    ChnAttr.bDeflicker = HI_FALSE;

    ret = HI_MPI_VO_SetChnAttr(VoLayer, 0, &ChnAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetChnAttr failed!\n";
        return false;
    }

    ret = HI_MPI_VO_EnableChn(VoLayer, 0);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_EnableChn failed!\n";
        return false;
    }

    ret = HI_MPI_HDMI_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Init failed!\n";
        return false;
    }
    HI_HDMI_ID_E enHdmiId = HI_HDMI_ID_0;
    HI_MPI_HDMI_Open(enHdmiId);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Open failed!\n";
        return false;
    }
    HI_HDMI_ATTR_S Hdmi_Attr;
    ret = HI_MPI_HDMI_GetAttr(enHdmiId, &Hdmi_Attr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_GetAttr failed!\n";
        return false;
    }
    Hdmi_Attr.bEnableHdmi = HI_TRUE;
    Hdmi_Attr.bEnableVideo = HI_TRUE;
    Hdmi_Attr.enVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60;
    Hdmi_Attr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
    Hdmi_Attr.enDeepColorMode = HI_HDMI_DEEP_COLOR_24BIT;
    Hdmi_Attr.bxvYCCMode = HI_FALSE;
    Hdmi_Attr.enOutCscQuantization = HDMI_QUANTIZATION_LIMITED_RANGE;
    Hdmi_Attr.bEnableAudio = HI_FALSE;
    Hdmi_Attr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S;
    Hdmi_Attr.bIsMultiChannel = HI_FALSE;
    Hdmi_Attr.enBitDepth = HI_HDMI_BIT_DEPTH_16;
    Hdmi_Attr.bEnableAviInfoFrame = HI_TRUE;
    Hdmi_Attr.bEnableAudInfoFrame = HI_TRUE;
    Hdmi_Attr.bEnableSpdInfoFrame = HI_FALSE;
    Hdmi_Attr.bEnableMpegInfoFrame = HI_FALSE;
    Hdmi_Attr.bDebugFlag = HI_FALSE;
    Hdmi_Attr.bHDCPEnable = HI_FALSE;
    Hdmi_Attr.b3DEnable = HI_FALSE;
    Hdmi_Attr.enDefaultMode = HI_HDMI_FORCE_HDMI;
    HI_MPI_HDMI_SetAttr(enHdmiId, &Hdmi_Attr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_SetAttr failed!\n";
        return false;
    }
    ret = HI_MPI_HDMI_Start(enHdmiId);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Start failed!\n";
        return false;
    }

    return true;
}

bool Hi_Mpp_Vo::Bind_Vpss(const HI_S32 Vpss_Grp, const HI_S32 Vpss_Chn)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = Vpss_Grp;
    stSrcChn.s32ChnId = Vpss_Chn;
    stDestChn.enModId = HI_ID_VO;
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = Vo_Dev;
    ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (HI_SUCCESS != ret)
    {
        return false;
    }
    return true;
}

HI_U32 Hi_Mpp_Vo::UnBind_Vpss()
{
    ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (HI_SUCCESS != ret)
    {
        return HI_FALSE;
    }
    return HI_SUCCESS;
}