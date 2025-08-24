#include "vi.hpp"

Hi_Mpp_Vi::Hi_Mpp_Vi()
{
}

Hi_Mpp_Vi::~Hi_Mpp_Vi()
{
    // 退出程序前需要退出这些模块，否则下次运行程序会显示xx模块繁忙
    HI_MPI_SYS_Exit();
    HI_MPI_VB_ExitModCommPool(VB_UID_VI);
    HI_MPI_VB_Exit();
}

bool Hi_Mpp_Vi::Init()
{
    /*    初始化系统和VB池        */
    // 去初始化 MPP 系统。
    HI_MPI_SYS_Exit();
    // 去初始化MPP视频缓存池
    HI_MPI_VB_Exit();
    // 设置缓存池个数为1
    vb_config.u32MaxPoolCnt = 1;
    // 获取一帧图像总大小                                                此处使用SEG格式不知是否是为了演示他具有这个格式，稍后用默认模式看看
    vb_config.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(width, height, PIXEL_FORMAT_YUV_SEMIPLANAR_420,
                                                                  DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    // 缓存池中缓冲块个数
    vb_config.astCommPool[0].u32BlkCnt = 10;
    // 设置缓存池属性
    ret = HI_MPI_VB_SetConfig(&vb_config);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VB_SetConfig Fail ret = " << ret << std::endl;
        return false;
    }
    // 初始化MPP视频缓存池
    ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_VB_Init failed!\n";
        return HI_FAILURE;
    }
    // 初始化MPP系统
    ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MPI_SYS_Init failed!\n";
        HI_MPI_VB_Exit();
        return HI_FAILURE;
    }

    /******启动VI******/
    /*      启动第一步开启MIPI    */
    lane_divide_mode_t mipi_mode = LANE_DIVIDE_MODE_1;
    int fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        std::cout << "open hi_mipi dev failed\n";
        return -1;
    }

    // 设置MIPI Rx的Lane分布模式，对SLVS无作用。
    ret = ioctl(fd, HI_MIPI_SET_HS_MODE, &mipi_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_SET_HS_MODE failed\n";
    }

    // 设置时钟
    ret = ioctl(fd, HI_MIPI_ENABLE_MIPI_CLOCK, &dev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_ENABLE_MIPI_CLOCK failed\n";
    }

    // 复位MIPI
    ret = ioctl(fd, HI_MIPI_RESET_MIPI, &dev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_RESET_MIPI failed\n";
    }

    // 使能摄像头时钟
    sns_clk_source_t SnsDev = 0;
    ret = ioctl(fd, HI_MIPI_ENABLE_SENSOR_CLOCK, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_ENABLE_SENSOR_CLOCK failed\n";
    }

    // 复位sensor
    ret = ioctl(fd, HI_MIPI_RESET_SENSOR, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_RESET_SENSOR failed\n";
    }

    // 设置MIPI Rx、SLVS和并口设备属性。此处可能和mipi口有关，待观察
    combo_dev_attr_t stcomboDevAttr = {
        .devno = 0,
        .input_mode = INPUT_MODE_MIPI,
        .data_rate = MIPI_DATA_RATE_X1,
        .img_rect = {0, 0, 1920, 1080},

        {.mipi_attr =
             {
                 DATA_TYPE_RAW_10BIT, // raw格式
                 HI_MIPI_WDR_MODE_NONE,
                 {0, 2, -1, -1}}}};
    ret = ioctl(fd, HI_MIPI_SET_DEV_ATTR, &stcomboDevAttr);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_SET_DEV_ATTR failed\n";
    }
    // 撤销复位MIPI Rx
    ret = ioctl(fd, HI_MIPI_UNRESET_MIPI, &dev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_UNRESET_MIPI failed\n";
    }
    // 撤销复位Sensor。
    ret = ioctl(fd, HI_MIPI_UNRESET_SENSOR, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_UNRESET_SENSOR failed\n";
    }
    close(fd);

    /*      开启第二步设置VI属性     */
    // 获取 VI，VPSS 的工作模式。获取到stVIVPSSMode
    VI_VPSS_MODE_S vi_vpss_mode;
    ret = HI_MPI_SYS_GetVIVPSSMode(&vi_vpss_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "Get VI-VPSS mode Param failed\n";
    }
    // 设置VI_VPSS模式
    ret = HI_MPI_SYS_SetVIVPSSMode(&vi_vpss_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "Set VI-VPSS mode Param failed\n";
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
         // 输入数据类型，Sensor 输入一般为 RGB，AD 输入一般为YUV。
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
        DATA_RATE_X1};
    ret = HI_MPI_VI_SetDevAttr(dev, &ViDevAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_SetDevAttr failed!\n";
        return HI_FAILURE;
    }

    ret = HI_MPI_VI_EnableDev(dev);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_EnableDev failed!\n";
        return HI_FAILURE;
    }

    VI_DEV_BIND_PIPE_S DevBindPipe = {0};
    DevBindPipe.PipeId[0] = dev;
    DevBindPipe.u32Num = 1;
    ret = HI_MPI_VI_SetDevBindPipe(dev, &DevBindPipe);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_SetDevBindPipe failed!\n";
        return HI_FAILURE;
    }

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
        // 输入图像的 bit 位宽。静态属性，创建 PIPE 时设定，不可更改，仅当像素格式enPixFmt 为 YUV 像素格式时效 。    同理跟前面池子获取大小有歧义
        DATA_BITWIDTH_10,
        // NR 使能开关。还不知道是啥注意：Hi3519AV100/Hi3516EV200 不支持 NR 功能，此参数无意义。
        HI_TRUE,
        // NR 属性结构体。静态属性，创建 PIPE 时设定，不可更改。注意：Hi3519AV100/Hi3516EV200 不支持 NR 功能，此参数无意义。
        {
            PIXEL_FORMAT_YVU_SEMIPLANAR_420, // 重构帧的像素格式。
            DATA_BITWIDTH_8,                 // 重构帧的 bit 位宽。
            VI_NR_REF_FROM_RFR,              // 参考帧来源选择。   这里是重构帧作为参考帧。
            COMPRESS_MODE_NONE               // 重构帧是否压缩。
        },
        // Sharpen 使能开关。dv3516不支持该功能
        HI_FALSE,
        {-1, -1}};
        //此处dev和pipe一样，所以直接用dev代替，还不知道dev和pipe关系
    ret = HI_MPI_VI_CreatePipe(dev, &PipeAttr);
    if (ret != HI_SUCCESS)
    {
        std::cout << "HI_MPI_VI_CreatePipe failed!\n";
    }

    return true;
}