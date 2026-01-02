
cmake_minimum_required(VERSION 3.15)
project(bsp)
        
set(DRIVER_TARGETS scugic;zdma;sysmonpsu;axipmon;coresightps_dcc;resetps;clockps;csudma;ddrcpsu;emacps;gpiops;iicps;ipipsu;qspipsu;rtcpsu;sdps;spips;ttcps;uartps;usbpsu;wdtps)
set(DRIVER_LOCATIONS E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/scugic_v5_2;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/zdma_v1_17;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/sysmonpsu_v2_9;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/axipmon_v6_10;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/coresightps_dcc_v1_9;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/resetps_v1_6;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/clockps_v1_5;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/csudma_v1_14;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/ddrcpsu_v1_5;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/emacps_v3_19;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/gpiops_v3_12;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/iicps_v3_18;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/ipipsu_v2_14;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/qspipsu_v1_18;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/rtcpsu_v1_13;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/sdps_v4_2;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/spips_v3_9;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/ttcps_v3_18;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/uartps_v3_13;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/usbpsu_v1_14;E:/Xilinx/Vitis/2023.2/data/embeddedsw/XilinxProcessorIPLib/drivers/wdtps_v3_6)

list(LENGTH DRIVER_TARGETS no_of_drivers)
set(index 0)

while(${index} LESS ${no_of_drivers})
    list(GET DRIVER_TARGETS ${index} drv)
    list(GET DRIVER_LOCATIONS ${index} drv_dir)
    set(exp_dir "${drv_dir}/examples")
    add_custom_target(
        ${drv}_example ALL
        COMMAND lopper -f -O E:/x/shef/vitis/platform/psu_cortexa53_0/standalone_psu_cortexa53_0/bsp/libsrc/${drv} E:/x/shef/vitis/platform/psu_cortexa53_0/standalone_psu_cortexa53_0/bsp/hw_artifacts/psu_cortexa53_0_baremetal.dts -- bmcmake_metadata_xlnx psu_cortexa53_0 ${exp_dir} drvcmake_metadata
        BYPRODUCTS x${drv}_exlist.yaml)
    MATH(EXPR index "${index}+1")
endwhile()
