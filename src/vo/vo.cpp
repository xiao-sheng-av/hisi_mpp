#include "vo.hpp"

Hi_Mpp_Vo::Hi_Mpp_Vo()
{
}

Hi_Mpp_Vo::~Hi_Mpp_Vo()
{
    HI_MPI_VO_Disable(Vo_Dev);
}

HI_U32 Hi_Mpp_Vo::Init()
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
        return HI_FAILURE;
    }

    ret = HI_MPI_VO_Enable(Vo_Dev);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_Enable failed!\n";
        return HI_FAILURE;
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
        return HI_FAILURE;
    }
    ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, &LayerAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetVideoLayerAttr failed!\n";
        return HI_FAILURE;
    }

    ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_EnableVideoLayer failed!\n";
        return HI_FAILURE;
    }

    ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &LayerAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_GetVideoLayerAttr failed!\n";
        return HI_FAILURE;
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
        printf("%s(%d):failed with %#x!\n",
               __FUNCTION__, __LINE__, ret);
        return HI_FAILURE;
    }

    ret = HI_MPI_VO_EnableChn(VoLayer, 0);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_EnableChn failed!\n";
        return HI_FAILURE;
    }

    ret = HI_MPI_HDMI_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Init failed!\n";
        return HI_FAILURE;
    }
    HI_HDMI_ID_E enHdmiId = HI_HDMI_ID_0;
    HI_MPI_HDMI_Open(enHdmiId);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Open failed!\n";
        return HI_FAILURE;
    }
    HI_HDMI_ATTR_S Hdmi_Attr;
    ret = HI_MPI_HDMI_GetAttr(enHdmiId, &Hdmi_Attr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_GetAttr failed!\n";
        return HI_FAILURE;
    }
    Hdmi_Attr.bEnableHdmi = HI_TRUE;
    Hdmi_Attr.bEnableVideo = HI_TRUE;
    Hdmi_Attr.enVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60;
    Hdmi_Attr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
    Hdmi_Attr.enDeepColorMode = HI_HDMI_DEEP_COLOR_24BIT;
    Hdmi_Attr.bxvYCCMode            = HI_FALSE;
    Hdmi_Attr.enOutCscQuantization  = HDMI_QUANTIZATION_LIMITED_RANGE;
    Hdmi_Attr.bEnableAudio          = HI_FALSE;
    Hdmi_Attr.enSoundIntf           = HI_HDMI_SND_INTERFACE_I2S;
    Hdmi_Attr.bIsMultiChannel       = HI_FALSE;
    Hdmi_Attr.enBitDepth            = HI_HDMI_BIT_DEPTH_16;
    Hdmi_Attr.bEnableAviInfoFrame   = HI_TRUE;
    Hdmi_Attr.bEnableAudInfoFrame   = HI_TRUE;
    Hdmi_Attr.bEnableSpdInfoFrame   = HI_FALSE;
    Hdmi_Attr.bEnableMpegInfoFrame  = HI_FALSE;
    Hdmi_Attr.bDebugFlag            = HI_FALSE;
    Hdmi_Attr.bHDCPEnable           = HI_FALSE;
    Hdmi_Attr.b3DEnable             = HI_FALSE;
    Hdmi_Attr.enDefaultMode         = HI_HDMI_FORCE_HDMI;
    HI_MPI_HDMI_SetAttr(enHdmiId, &Hdmi_Attr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_SetAttr failed!\n";
        return HI_FAILURE;
    }
    ret = HI_MPI_HDMI_Start(enHdmiId);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Start failed!\n";
        return HI_FAILURE;
    }


    return HI_SUCCESS;
}

bool Hi_Mpp_Vo::Bind_Vpss(const HI_S32 Vpss_Grp, const HI_S32 Vpss_Chn)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    stSrcChn.enModId   = HI_ID_VPSS;
    stSrcChn.s32DevId  = Vpss_Grp;
    stSrcChn.s32ChnId  = Vpss_Chn;
    stDestChn.enModId  = HI_ID_VO;
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = Vo_Dev;
    ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (HI_SUCCESS != ret)
    {
        return false;
    }
    return true;
}