#include "vi.hpp"

Hi_Mpp_Vi::Hi_Mpp_Vi()
{
}

Hi_Mpp_Vi::~Hi_Mpp_Vi()
{
    // 退出程序前需要退出这些模块，否则下次运行程序会显示xx模块繁忙
    isp_stop();
    HI_MPI_AWB_UnRegister(Dev, &stAwbLib);
    HI_MPI_AE_UnRegister(Dev, &stAeLib);
    pstSnsObj->pfnUnRegisterCallback(Dev, &stAeLib, &stAwbLib);
    HI_MPI_VI_DisableChn(Pipe_Id, Chn_Id);
    HI_MPI_VI_StopPipe(Pipe_Id);
    HI_MPI_VI_DestroyPipe(Pipe_Id);
    HI_MPI_VI_DisableDev(Dev);
    // 缺少iotcl反初始化
    HI_MPI_SYS_Exit();
    HI_MPI_VB_ExitModCommPool(VB_UID_VI);
    HI_MPI_VB_Exit();

}

bool Hi_Mpp_Vi::Init()
{
    /*    初始化系统和VB池        */
    // 去初始化 MPP 系统。用户处理完图像、视频等媒体数据之后，需先调用本接口对媒体数据处理系统底层进行去初始化。
    HI_MPI_SYS_Exit();
    // 去初始化MPP视频缓存池。必须先调用 HI_MPI_SYS_Exit 去初始化 MPP 系统，再去初始化缓存池，否则返回失败。
    HI_MPI_VB_Exit();
    // 设置缓存池个数为1
    Vb_Config.u32MaxPoolCnt = 1;
    // 获取一帧图像总大小                                                此处使用SEG格式不知是否是为了演示他具有这个格式，稍后用默认模式看看，此处SEG和NONE都可
    Vb_Config.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(Width, Height, PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                  DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    // 缓存池中缓冲块个数
    Vb_Config.astCommPool[0].u32BlkCnt = 10;
    // 设置缓存池属性。只能在系统处于未初始化的状态下(也就是使用HI_MPI_VB_Init之前)，才可以设置缓存池属性，否则会返回失败。
    ret = HI_MPI_VB_SetConfig(&Vb_Config);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VB_SetConfig Fail ret = " << ret << std::endl;
        return false;
    }
    // 初始化MPP视频缓存池，必须先调用 HI_MPI_VB_SetConfig 配置缓存池属性，再初始化缓存池，否则会失败。
    ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VB_Init failed!\n";
        return false;
    }
    // 初始化MPP系统，必须先调用 HI_MPI_SYS_SetConfig(该接口功能暂时无效) 配置 MPP 系统后才能初始化，否则初始化会失败。
    // 由于 MPP 系统的正常运行依赖于缓存池，因此需要先调用 HI_MPI_VB_Init 初始化缓存池，再初始化 MPP 系统，否则会导致业务运行异常。
    // 只要一个进程进行初始化即可，不需要所的进程都做系统初始化的操作。
    ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_SYS_Init failed!\n";
        return false;
    }

    /******启动VI******/
    /*      启动第一步开启MIPI    */
    lane_divide_mode_t mipi_mode = LANE_DIVIDE_MODE_1;
    int fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        std::cout << "open hi_mipi dev failed\n";
        return false;
    }

    // 设置MIPI Rx的Lane分布模式，对SLVS无作用。
    ret = ioctl(fd, HI_MIPI_SET_HS_MODE, &mipi_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_SET_HS_MODE failed\n";
        return false;
    }

    // 设置时钟
    combo_dev_t mipidev = Dev;

    ret = ioctl(fd, HI_MIPI_ENABLE_MIPI_CLOCK, &mipidev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_ENABLE_MIPI_CLOCK failed\n";
        return false;
    }

    // 复位MIPI
    ret = ioctl(fd, HI_MIPI_RESET_MIPI, &Dev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_RESET_MIPI failed\n";
        return false;
    }

    // 使能摄像头时钟
    // 修改此处为1，对应dev = 1
    sns_clk_source_t SnsDev = Dev;

    ret = ioctl(fd, HI_MIPI_ENABLE_SENSOR_CLOCK, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_ENABLE_SENSOR_CLOCK failed\n";
        return false;
    }

    // 复位sensor

    ret = ioctl(fd, HI_MIPI_RESET_SENSOR, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_RESET_SENSOR failed\n";
        return false;
    }

    // 设置MIPI Rx、SLVS和并口设备属性。需要选择与dev对应的属性，dev1和dev0的不通用
    combo_dev_attr_t stcomboDevAttr =
        {
            .devno = 1,
            .input_mode = INPUT_MODE_MIPI,
            .data_rate = MIPI_DATA_RATE_X1,
            .img_rect = {0, 0, 1920, 1080},

            {.mipi_attr =
                 {
                     DATA_TYPE_RAW_10BIT,
                     HI_MIPI_WDR_MODE_NONE,
                     {1, 3, -1, -1}}}};
    ret = ioctl(fd, HI_MIPI_SET_DEV_ATTR, &stcomboDevAttr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_SET_DEV_ATTR failed\n";
        return false;
    }
    // 撤销复位MIPI Rx
    ret = ioctl(fd, HI_MIPI_UNRESET_MIPI, &Dev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_UNRESET_MIPI failed\n";
        return false;
    }
    // 撤销复位Sensor。
    ret = ioctl(fd, HI_MIPI_UNRESET_SENSOR, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_UNRESET_SENSOR failed\n";
        return false;
    }

    close(fd);

    /*      开启第二步设置VI属性     */
    // 获取 VI，VPSS 的工作模式。获取到stVIVPSSMode
    VI_VPSS_MODE_S vi_vpss_mode;
    ret = HI_MPI_SYS_GetVIVPSSMode(&vi_vpss_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "Get VI-VPSS mode Param failed\n";
        return false;
    }
    // 设置VI_VPSS模式
    ret = HI_MPI_SYS_SetVIVPSSMode(&vi_vpss_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "Set VI-VPSS mode Param failed\n";
        return false;
    }

    VI_DEV_ATTR_S ViDevAttr = {
        VI_MODE_MIPI,            // 物理接口模式，这里使用的是mipi
        VI_WORK_MODE_1Multiplex, // 一路复合工作模式
        /*分量掩码配置。当enIntfMode=VI_MODE_BT1120_STANDARD 时，需要配置 Y（对应该变量数组的 0 下标）和 C（对应该变量数组
        的 1 下标）的分量掩码，其他模式时配置单分量掩码（对应该变量数组的 0 下标），另一个分量（对应该变量数组的 1 下标）配成 0x0。*/
        {0xFFC00000, 0x0},
        VI_SCAN_PROGRESSIVE, // 输入扫描模式 (逐行、隔行)。VI_SCAN_PROGRESSIVE表示为逐行
        {-1, -1, -1, -1},    // 取值范围[-1, 3]，推荐统一设置为默认值-1，此参数无意义。
        VI_DATA_SEQ_YUYV,    // 输入数据顺序 (仅支持 yuv 格式)。
        // 同步时序配置，BT.601 和 DC 模式时必须配置，其它模式时无效。
        {/*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
         VI_VSYNC_PULSE,
         VI_VSYNC_NEG_LOW,
         VI_HSYNC_VALID_SINGNAL,
         VI_HSYNC_NEG_HIGH,
         VI_VSYNC_VALID_SINGAL,
         VI_VSYNC_VALID_NEG_HIGH,
         /*hsync_hfb    hsync_act    hsync_hhb*/
         {
             0, 1280, 0,
             /*vsync0_vhb vsync0_act vsync0_hhb*/
             0, 720, 0,
             /*vsync1_vhb vsync1_act vsync1_hhb*/
             0, 0, 0}},
        // 输入数据类型，Sensor 输入一般为 RGB，AD 输入一般为YUV。
        VI_DATA_TYPE_RGB,
        /*因为走线约束等硬件原因，可能出现 AD/Sensor 的数据线与 VI 数据线连接数据高低位反接，比如，AD_DATA0与 VIU_DATA7 连接，
        AD_DATA7 与 VIU_DATA0 连接，以此类推。当 AD/Sensor 管脚与 VI 管脚正向连接时，取 bDataReverse = HI_FALSE；当反向连接时，取bDataReverse = HI_TRUE。*/
        HI_FALSE,
        /*VI 设备可设置要捕获图像的高宽。捕获图像的最小宽高与最大宽高：宽度：[VI_DEV_MIN_WIDTH，VI_DEV_MAX_WIDTH]
        高度：[VI_DEV_MIN_HEIGHT，VI_DEV_MAX_HEIGHT]
        Hi3516DV300：stSize 需不大于 2688*1944，否则报错。
        Hi3516CV500：stSize 需不大于 2304*1296，否则报错。*/
        {1920, 1080},
        // Bayer 域缩放之后的宽、高，以及相位调整的类型。
        {{
             {1920, 1080},
         },
         // 相位调整类型：此处都是不进行调整
         {VI_REPHASE_MODE_NONE,
          VI_REPHASE_MODE_NONE}},
        // 设备的速率。参数详情请参考“系统控制”DATA_RATE_E 的描述。
        {WDR_MODE_NONE,
         1080},
        // 一拍一像素
        DATA_RATE_X1};
    // 设置 VI 设备属性。基本设备属性默认了部分芯片配置，满足绝大部分的 sensor 对接要求。
    // 在调用前要保证 VI 设备处于禁用状态。如果 VI 设备已处于使能状态，可以使用 HI_MPI_VI_DisableDev 来禁用设备。
    ret = HI_MPI_VI_SetDevAttr(Dev, &ViDevAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_SetDevAttr failed!\n";
        return false;
    }
    // 启用 VI 设备。
    /*启用前必须已经设置设备属性，否则返回失败。
      可重复启用，不返回失败。
      Hi3516CV500 在同一时刻只支持启动一个 VI DEV。
      Hi3516DV300/Hi3559V200/Hi3556V200 支持同时启动两个 VI DEV */
    ret = HI_MPI_VI_EnableDev(Dev);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_EnableDev failed!\n";
        return false;
    }
    // VI_DEV_BIND_PIPE_S:定义 VI DEV 与 PIPE 的绑定关系.
    VI_DEV_BIND_PIPE_S DevBindPipe;
    memset(&DevBindPipe, 0, sizeof(DevBindPipe));
    // 该 VI Dev 所绑定的 PIPE 数目
    DevBindPipe.u32Num = 1;
    // 该 VI Dev 绑定的 PIPE 号。
    DevBindPipe.PipeId[0] = Pipe_Id;
    // 必须先使能 VI 设备后才能绑定物理 PIPE。不支持动态绑定。
    ret = HI_MPI_VI_SetDevBindPipe(Dev, &DevBindPipe);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_SetDevBindPipe failed!\n";
        return false;
    }

    // PIPE对VI的数据进行处理，然后输出到通道
    // 只有PIPE0 支持并行模式。
    // 物理 PIPE 属性中的 u32MaxW、u32MaxH、enPixFmt、enBitWidth 等必须与前端进入 VI 的时序设置保持一致，否则会出现错误。
    VI_PIPE_ATTR_S PipeAttr = {
        // VI PIPE 的 Bypass 模式。VI的数据是否经过FE和BE处理，VI_PIPE_BYPASS_NONE表示不处理，静态属性，创建 PIPE 时设定，不可更改。
        VI_PIPE_BYPASS_NONE,
        // 是否关闭下采样和 CSC。HI_FALSE：yuv skip 不使能HI_TRUE：yuv skip 使能。静态属性，创建 PIPE 时设定，不可更改。
        HI_FALSE,
        // ISP 是否 bypass。HI_FALSE：ISP 正常运行。HI_TRUE：ISP bypass，不运行 ISP。静态属性，创建 PIPE 时设定，不可更改。
        HI_FALSE,
        // 输入图像宽度。静态属性，创建 PIPE 时设定，不可更改。
        1920,
        // 输入图像高度。静态属性，创建 PIPE 时设定，不可更改。
        1080,
        // 像素格式。  enPixFmt
        PIXEL_FORMAT_RGB_BAYER_10BPP,
        // 视频压缩格式，                                                                                        这里又是行压缩，跟前面池子大小获取有歧义
        COMPRESS_MODE_LINE,
        // 输入图像的 bit 位宽。静态属性，创建 PIPE 时设定，不可更改，仅当像素格式enPixFmt 为 YUV 像素格式时效 。
        DATA_BITWIDTH_10,
        // NR 使能开关。注意：Hi3519AV100/Hi3516EV200 不支持 NR 功能，此参数无意义。
        HI_TRUE,
        // NR表示降噪。NR 属性结构体。静态属性，创建 PIPE 时设定，不可更改。注意：Hi3519AV100/Hi3516EV200 不支持 NR 功能，此参数无意义。
        {
            PIXEL_FORMAT_YVU_SEMIPLANAR_420, // 重构帧的像素格式。
            DATA_BITWIDTH_8,                 // 重构帧的 bit 位宽。
            VI_NR_REF_FROM_RFR,              // 参考帧来源选择。   这里是重构帧作为参考帧。
            COMPRESS_MODE_NONE               // 重构帧是否压缩。
        },
        // Sharpen 使能开关。dv3516不支持该功能
        HI_FALSE,
        // 一样不进行帧率控制
        {-1, -1}};
    // 创建一个 VI PIPE。不支持重复创建。
    ret = HI_MPI_VI_CreatePipe(Pipe_Id, &PipeAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_CreatePipe failed!" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    // 启用 VI PIPE。PIPE 必须已创建(HI_MPI_VI_CreatePipe)
    ret = HI_MPI_VI_StartPipe(Pipe_Id);
    if (ret != HI_SUCCESS)
    {
        // 销毁一个PIPE
        HI_MPI_VI_StopPipe(Pipe_Id);
        HI_MPI_VI_DestroyPipe(Pipe_Id);
        std::cout << "HI_MPI_VI_StartPipe failed!\n";
        return false;
    }

    // VI 通道属性
    VI_CHN_ATTR_S ChnAttr =
        {
            // 目标图像大小
            {1920, 1080},
            // 输出像素格式，静态属性，设置 CHN 时设定，不可更改。
            PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            // 动态范围，静态属性，设置 CHN 时设定，不可更改。DYNAMIC_RANGE_SDR8 表示的是 8bit 数据，其余为 10bit 数据。
            DYNAMIC_RANGE_SDR8,
            // 目标图像视频数据格式
            VIDEO_FORMAT_LINEAR,
            // 输出图像压缩模式
            COMPRESS_MODE_NONE,
            // Mirror 使能开关。
            HI_FALSE,
            // Flip 使能开关。
            HI_FALSE,
            // 用户获取图像的队列深度。
            0,
            // 帧率控制,当源帧率为-1 时，目标帧率必须为-1(不进行帧率控制)，其他情况下，目标帧率不能大于源帧率。
            {-1, -1}};
    // 设置VI通道属性。3516dv300只有通道0
    ret = HI_MPI_VI_SetChnAttr(Pipe_Id, Chn_Id, &ChnAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_SetChnAttr failed!\n";
        return false;
    }

    VI_VPSS_MODE_E enMastPipeMode = vi_vpss_mode.aenMode[0];
    if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode)
    {
        // PIPE 必须已创建，否则会返回失败。必须先设置通道属性。上面三个模式启动VI通道不生效，直接返回成功。对应 HI_MPI_VI_DisableChn
        ret = HI_MPI_VI_EnableChn(Pipe_Id, Chn_Id);
        if (ret != HI_SUCCESS)
        {
            std::cout << "HI_MPI_VI_EnableChn failed!\n";
            return false;
        }
    }
    pstSnsObj = &stSnsGc2053Obj;
    stAeLib.s32Id = Pipe_Id;
    stAwbLib.s32Id = Pipe_Id;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    // 对应 pfnUnRegisterCallback
    ret = pstSnsObj->pfnRegisterCallback(Pipe_Id, &stAeLib, &stAwbLib);
    if (ret != HI_SUCCESS)
    {
        std::cout << "sensor_register_callback failed!\n";
        return false;
    }

    ISP_SNS_COMMBUS_U uSnsBusInfo;
    uSnsBusInfo.s8I2cDev = Dev;
    ret = pstSnsObj->pfnSetBusInfo(Pipe_Id, uSnsBusInfo);
    if (ret != HI_SUCCESS)
    {
        std::cout << "set sensor bus info failed!\n";
        return false;
    }
    // 向ISP注册海思AE库。对应 HI_MPI_AE_UnRegister
    ret = HI_MPI_AE_Register(Pipe_Id, &stAeLib);
    if (ret != HI_SUCCESS)
    {
        std::cout << "ae_register failed!\n";
        return false;
    }
    // 向ISP注册海思AWB库。对应 HI_MPI_AWB_UnRegister
    ret = HI_MPI_AWB_Register(Pipe_Id, &stAwbLib);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_AWB_Register failed!\n";
        return false;
    }

    // 初始化ISP内部资源。
    ret = HI_MPI_ISP_MemInit(Pipe_Id);
    if (ret != HI_SUCCESS)
    {
        std::cout << "Init Ext memory failed!\n";
        return false;
    }

    ISP_PUB_ATTR_S PubAttr =
        {
            // 裁剪窗口起始位置和图像宽高，必须设置为与vi pipe属性中的size保持一致，且不能比所绑定的dev属性中的宽高大，如果配置不符，会导致出图异常。
            {0, 0, 1920, 1080},
            // Sensor输出的图像宽高。
            {1920, 1080},
            // 输入图像帧率，取值范围为(0.00, 65535.00]。
            30,
            // Bayer数据格式。BAYER_RGGB表示RGGB排列方式。
            BAYER_RGGB,
            // WDR模式选择。
            WDR_MODE_NONE,
            // 用于进行Sensor初始化序列的选择，在分辨率和帧率相同时，配置不同的sns_mode对应不同的初始化序列；其他情况，sns_mode默认配置为0，可通过sns_size和frame_rate进行初始化序列的选择。
            0,
        };
    // 设置ISP公共属性。调用本接口前，必须先调用 HI_MPI_ISP_MemInit 接口初始化ISP内部资源。ISP支持运行过程中动态裁剪图像的起始位置。
    ret = HI_MPI_ISP_SetPubAttr(Pipe_Id, &PubAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "SetPubAttr failed!\n";
        return false;
    }
    // 调用本接口前，必须先调用 HI_MPI_ISP_SetPubAttr 接口设置图像公共属性。并且不支持多进程
    // ISP初始化后，需要一帧时间给硬件读取算法系数表。
    ret = HI_MPI_ISP_Init(Pipe_Id);
    if (ret != HI_SUCCESS)
    {
        std::cout << "ISP Init failed!\n";
        return false;
    }
    // 启动isp，该函数为阻塞，所以需要一个线程去运行
    // 调用本接口前，必须先调用 HI_MPI_ISP_Init 接口初始化ISP firmware。
    // 不支持多进程
    isp_thread = std::thread(HI_MPI_ISP_Run, Pipe_Id);
    return true;
}

void Hi_Mpp_Vi::isp_stop()
{
    // 调用hi_mpi_isp_run之后，再调用本接口退出ISP firmware。
    HI_MPI_ISP_Exit(Pipe_Id);
    isp_thread.join();
}
