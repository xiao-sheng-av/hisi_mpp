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
    //撤销复位Sensor。
    ret = ioctl(fd, HI_MIPI_UNRESET_SENSOR, &SnsDev);
    if (HI_SUCCESS != ret)
    {
        std::cout << "HI_MIPI_UNRESET_SENSOR failed\n";
    }
    close(fd);

    /*      开启第二步设置VI属性     */
    //获取 VI，VPSS 的工作模式。获取到stVIVPSSMode
    VI_VPSS_MODE_S vi_vpss_mode;
    ret = HI_MPI_SYS_GetVIVPSSMode(&vi_vpss_mode);
    if (HI_SUCCESS != ret)
    {
        std::cout << "Get VI-VPSS mode Param failed\n";
    }
    
    return true;
}