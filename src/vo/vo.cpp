#include "vo.hpp"

Hi_Mpp_Vo::Hi_Mpp_Vo()
{
}

Hi_Mpp_Vo::~Hi_Mpp_Vo()
{

    HI_HDMI_ID_E enHdmiId = HI_HDMI_ID_0;
    // 停止指定的HDMI接口，不使能其送显能力。 HI_MPI_HDMI_Stop 接口被调用前必须先调用 HI_MPI_HDMI_Start 接口来打开对应的HDMI，且需要保持两个接口入参中的hi_hdmi_id参数值一致。不支持多进程
    HI_MPI_HDMI_Stop(enHdmiId);
    // 关闭指定的HDMI接口。调用hi_mpi_hdmi_close接口之前必须先调用hi_mpi_hdmi_open接口，且需要保持两个接口入参中的hi_hdmi_id参数值一致。不支持多进程
    HI_MPI_HDMI_Close(enHdmiId);
    // 反初始化HDMI。本接口需要与hi_mpi_hdmi_init接口成对调用，即hi_mpi_hdmi_init函数被调用后，才允许调用hi_mpi_hdmi_deinit接口，且保证二者调用次数相同。不支持多线程
    HI_MPI_HDMI_DeInit();
    // 禁用指定的视频输出通道。当高清设备的通道绑定VPSS（Video Processing Subsystem）时，建议先调用本接口停止通道后，再解绑定VO通道与VPSS通道的绑定关系，否则可能出现HI_ERR_VO_BUSY的超时返回错误
    // 与HI_MPI_VO_EnableChn接口成对使用
    ret = HI_MPI_VO_DisableChn(VoLayer, VoChn);
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

    ret = HI_MPI_VO_Disable(VoDev);
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
/* 3516dv300不支持Vo绑定Layer，原因可能是dev和layer都只有0,不需要绑定 */
bool Hi_Mpp_Vo::Init()
{
    // 定义视频输出公共属性结构体。
    VO_PUB_ATTR_S PubAttr;
    memset(&PubAttr, 0, sizeof(VO_PUB_ATTR_S));
    // Vo接口类型
    PubAttr.enIntfSync = VO_OUTPUT_1080P60;
    // 视频输出接口类型
    PubAttr.enIntfType = VO_INTF_HDMI;
    // 背景色，表示办法为RGB888颜色表示法
    PubAttr.u32BgColor = 0x0000FF;
    // 配置视频输出设备的公共属性。视频输出设备属性为静态属性，必须在执行 HI_MPI_VO_Enable 前配置。
    ret = HI_MPI_VO_SetPubAttr(VoDev, &PubAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetPubAttr failed!\n";
        return false;
    }
    // 启用视频输出设备。在调用设备使能前，必须对设备公共属性进行配置，否则返回设备未配置错误。
    ret = HI_MPI_VO_Enable(VoDev);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_Enable failed!\n";
        return false;
    }
    // 定义视频层属性。在视频层属性中存在三个概念，即设备分辨率、显示分辨率和图像分辨率。
    VO_VIDEO_LAYER_ATTR_S LayerAttr;
    // 获取视频层属性。建议在设置视频层属性前先调用此接口获取视频层属性。
    ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &LayerAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_GetVideoLayerAttr failed!\n";
        return false;
    }
    // stdisprect表示显示的区域大小。既显示
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

    // 设置原图像的属性。既图像
    LayerAttr.stImageSize.u32Width = LayerAttr.stDispRect.u32Width;
    LayerAttr.stImageSize.u32Height = LayerAttr.stDispRect.u32Height;
    // 指定视频层输出动态范围类型
    LayerAttr.enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    HI_U32 Buf = 3;
    // 设置显示缓冲的长度。调用前需保证视频输出视频层未使能( HI_MPI_VO_EnableVideoLayer 之前调用)。缓冲长度默认值是 0，默认是 VO 直通模式显示。
    ret = HI_MPI_VO_SetDisplayBufLen(VoLayer, Buf);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VO_SetDisplayBufLen failed!\n";
        return false;
    }
    // 设置视频层属性。需要在视频层所绑定的设备处于使能状态时 (HI_MPI_VO_Enable 之后调用)才能设置视频层属性。
    ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, &LayerAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetVideoLayerAttr failed!\n";
        return false;
    }
    // 使能视频层。视频层使能前必须保证该视频层所绑定的设备处于使能状态( HI_MPI_VO_Enable 之后调用)。视频层使能前必须保证该视频层已经配置 ( HI_MPI_VO_SetVideoLayerAttr之后)。
    ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_EnableVideoLayer failed!\n";
        return false;
    }

    VO_CHN_ATTR_S ChnAttr;
    // 获取指定视频输出通道的属性。
    ret = HI_MPI_VO_GetChnAttr(VoLayer, VoChn, &ChnAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_GetChnAttr failed!\n";
        return false;
    }
    // 通道矩形显示区域。取值需要2对齐，且范围在屏幕范围之内
    ChnAttr.stRect.s32X = 0;
    ChnAttr.stRect.s32Y = 0;
    ChnAttr.stRect.u32Width = 1920;
    ChnAttr.stRect.u32Height = 1080;
    // 通道叠加优先级。优先级高的在上层，只在 SINGLE 模式下有效。属性中的优先级，数值越大优先级越高。
    ChnAttr.u32Priority = 0;
    // 是否开启抗闪烁。抗闪烁效果仅针对使用 VGS 缩放的设备的通道效，即该参数仅在 SINGLE 模式下效。
    ChnAttr.bDeflicker = HI_FALSE;
    // 配置指定视频输出通道的属性。
    ret = HI_MPI_VO_SetChnAttr(VoLayer, VoChn, &ChnAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_SetChnAttr failed!\n";
        return false;
    }
    // 启用指定的视频输出通道。调用前必须保证视频层绑定关系存在(3516dv300不需要使用 HI_MPI_VO_BindVideoLayer )，否则将返回失败。调用前必须使能相应设备上的视频层。通道使能前必须进行通道配置，否则返回通道未配置的错误。
    ret = HI_MPI_VO_EnableChn(VoLayer, VoChn);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VO_EnableChn failed!\n";
        return false;
    }
    // 初始化HDMI。本接口需要与 HI_MPI_HDMI_DeInit 接口成对调用，即本接口被调用后，需要调用 HI_MPI_HDMI_DeInit 接口，才允许再次调用 HI_MPI_HDMI_Init 接口。
    ret = HI_MPI_HDMI_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_Init failed!\n";
        return false;
    }
    HI_HDMI_ID_E enHdmiId = HI_HDMI_ID_0;
    // 打开指定的HDMI接口。需要先调用 HI_MPI_HDMI_Init 。本接口需要与 HI_MPI_HDMI_Close 接口成对调用，即本接口被调用后，才允许调用 HI_MPI_HDMI_Close 接口，且保证参数相同。不支持多进程
    ret = HI_MPI_HDMI_Open(enHdmiId);
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
    Hdmi_Attr.bEnableHdmi = HI_TRUE;  // 是否强制通过HDMI输出视频
    Hdmi_Attr.bEnableVideo = HI_TRUE; // 是否输出视频
    Hdmi_Attr.enVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60; // 视频制式
    Hdmi_Attr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444; // HDMI输出视频模式
    Hdmi_Attr.enDeepColorMode = HI_HDMI_DEEP_COLOR_24BIT; // DeepColor输出模式
    Hdmi_Attr.bxvYCCMode = HI_FALSE;  // 是否启用xvYCC输出模式
    Hdmi_Attr.enOutCscQuantization = HDMI_QUANTIZATION_LIMITED_RANGE;  // CSC输出量化范围
    Hdmi_Attr.bEnableAudio = HI_FALSE;  // 是否启用音频
    Hdmi_Attr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S; // HDMI音频源
    Hdmi_Attr.bIsMultiChannel = HI_FALSE; // 多声道或立体声
    Hdmi_Attr.enBitDepth = HI_HDMI_BIT_DEPTH_16; // 音频位宽
    Hdmi_Attr.bEnableAviInfoFrame = HI_TRUE; // 是否启用AVI InfoFrame
    Hdmi_Attr.bEnableAudInfoFrame = HI_TRUE;    // 是否启用AUDIO InfoFrame
    Hdmi_Attr.bEnableSpdInfoFrame = HI_FALSE;   // 是否启用SPD InfoFrame
    Hdmi_Attr.bEnableMpegInfoFrame = HI_FALSE;  // 是否启用MPEG InfoFrame
    Hdmi_Attr.bDebugFlag = HI_FALSE; // 是否打印调试信息
    Hdmi_Attr.bHDCPEnable = HI_FALSE; // 是否启用HDCP
    Hdmi_Attr.b3DEnable = HI_FALSE; // 是否开启3D
    Hdmi_Attr.enDefaultMode = HI_HDMI_FORCE_HDMI; // HDMI工作模式
    // 在调用 HI_MPI_HDMI_Open 接口打开HDMI之后、调用 HI_MPI_HDMI_Start 接口启用HDMI之前，调用本接口设置HDMI属性，但需保证 hi_mpi_hdmi_open 接口与本接口中的 hi_hdmi_id 参数值保持一致。
    ret = HI_MPI_HDMI_SetAttr(enHdmiId, &Hdmi_Attr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_HDMI_SetAttr failed!\n";
        return false;
    }
    // 启动指定的HDMI，使能其送显能力。 HI_MPI_HDMI_Start 接口被调用前必须先调用 HI_MPI_HDMI_Open 接口来打开对应的HDMI，且需要保持两个接口入参中的 HI_HDMI_ID_E 参数值一致
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
    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = Vpss_Grp;
    stSrcChn.s32ChnId = Vpss_Chn;
    stDestChn.enModId = HI_ID_VO;
    // 例程使用layer
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = VoChn;
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