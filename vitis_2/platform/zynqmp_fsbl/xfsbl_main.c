/******************************************************************************
* Copyright (c) 2015 - 2021 Xilinx, Inc.  All rights reserved.
* Copyright (c) 2022 - 2023 Advanced Micro Devices, Inc.  All rights reserve.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
*
* @file xfsbl_main.c
*
* This is the main file which contains code for the FSBL.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  kc   10/21/13 Initial release
* 1.00  ba   02/22/16 Added performance measurement feature.
* 2.0   bv   12/02/16 Made compliance to MISRAC 2012 guidelines
*                     Added warm restart support
* 3.0   bv   03/03/21 Print multiboot offset in FSBL banner
*       bsv  04/28/21 Added support to ensure authenticated images boot as
*                     non-secure when RSA_EN is not programmed
*
* </pre>
*
* @note
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xfsbl_hw.h"
#include "xfsbl_main.h"
#include "bspconfig.h"
#include "xfsbl_authentication.h"
#include "dev_key.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
static void XFsbl_UpdateMultiBoot(u32 MultiBootValue);
static void XFsbl_FallBack(void);
static void XFsbl_MarkUsedRPUCores(XFsblPs *FsblInstPtr, u32 PartitionNum);

/************************** Variable Definitions *****************************/
XFsblPs FsblInstance = {0x3U, XFSBL_SUCCESS, 0U, 0U, 0U, 0U};
static XSecure_Sha3 csu_sha3;

/*****************************************************************************/
/** This is the FSBL main function and is implemented stage wise.
 *
 * @param	None
 *
 * @return	None
 *
 *****************************************************************************/
int main(void )
{
	/**
	 * Local variables
	 */
	u32 FsblStatus = XFSBL_SUCCESS;
	u32 FsblStage = XFSBL_STAGE1;
	u32 PartitionNum=0U;
	u32 EarlyHandoff = FALSE;
#ifdef XFSBL_PERF
	XTime tCur = 0;
#endif
#ifdef ENABLE_POS
	u32 WarmBoot;

	WarmBoot = XFsbl_HookGetPosBootType();
	if (0U != WarmBoot) {
		XFsbl_HandoffExit(0U, XFSBL_NO_HANDOFFEXIT);
	}
#endif

#if defined(EL3) && (EL3 != 1)
#error "FSBL should be generated using only EL3 BSP"
#endif

	/**
	 * Initialize globals.
	 */
	while (FsblStage<=XFSBL_STAGE_DEFAULT) {

		switch (FsblStage)
		{

		case XFSBL_STAGE1:
			{
				/**
				 * Initialize the system
				 */

				FsblStatus = XFsbl_Initialize(&FsblInstance);
				if (XFSBL_SUCCESS != FsblStatus)
				{
					FsblStatus += XFSBL_ERROR_STAGE_1;
					FsblStage = XFSBL_STAGE_ERR;
				} else {

					/**
					 *
					 * Include the code for FSBL time measurements
					 * Initialize the global timer and get the value
					 */

					FsblStage = XFSBL_STAGE2;
				}
			}break;

		case XFSBL_STAGE2:
			{
				/* Reading Timer value for Performance measurement.*/
#ifdef XFSBL_PERF
				/* Get Start time for Boot Device init. */
				XTime_GetTime(&tCur);
#endif

				XFsbl_Printf(DEBUG_INFO,
						"================= In Stage 2 ============ \n\r");

				/**
				 * 	Primary Device
				 *  Secondary boot device
				 *  DeviceOps
				 *  image header
				 *  partition header
				 */

				FsblStatus = XFsbl_BootDeviceInitAndValidate(&FsblInstance);
				if ( (XFSBL_SUCCESS != FsblStatus) &&
						(XFSBL_STATUS_JTAG != FsblStatus) )
				{
					XFsbl_Printf(DEBUG_GENERAL,"Boot Device "
							"Initialization failed 0x%0lx\n\r", FsblStatus);
					FsblStatus += XFSBL_ERROR_STAGE_2;
					FsblStage = XFSBL_STAGE_ERR;
				} else if (XFSBL_STATUS_JTAG == FsblStatus) {

					/*
					 * Mark RPU cores as usable in JTAG boot
					 * mode.
					 */
					Xil_Out32(XFSBL_R5_USAGE_STATUS_REG,
						  (Xil_In32(XFSBL_R5_USAGE_STATUS_REG) |
						   (XFSBL_R5_0_STATUS_MASK |
						    XFSBL_R5_1_STATUS_MASK)));

					/**
					 * This is JTAG boot mode, go to the handoff stage
					 */
					FsblStage = XFSBL_STAGE4;
				} else {
					XFsbl_Printf(DEBUG_INFO,"Initialization Success \n\r");

					/**
					 * Start the partition loading from 1
					 * 0th partition will be FSBL
					 */
					PartitionNum = 0x1U;

					/* Clear RPU status register */
					Xil_Out32(XFSBL_R5_USAGE_STATUS_REG,
						  (Xil_In32(XFSBL_R5_USAGE_STATUS_REG) &
						  ~(XFSBL_R5_0_STATUS_MASK |
						    XFSBL_R5_1_STATUS_MASK)));

					FsblStage = XFSBL_STAGE3;
				}
#ifdef XFSBL_PERF
				XFsbl_MeasurePerfTime(tCur);
				XFsbl_Printf(DEBUG_PRINT_ALWAYS, " : Boot Dev. Init. Time\n\r");
#endif
			} break;

		case XFSBL_STAGE3:
			{

				XFsbl_Printf(DEBUG_INFO,
					"======= In Stage 3, Partition No:%d ======= \n\r",
					PartitionNum);

				/**
				 * Load the partitions
				 *  image header
				 *  partition header
				 *  partition parameters
				 */
				FsblStatus = XFsbl_PartitionLoad(&FsblInstance,
								  PartitionNum);
				if (XFSBL_SUCCESS != FsblStatus)
				{
					/**
					 * Error
					 */
					XFsbl_Printf(DEBUG_GENERAL,"Partition %d Load Failed, 0x%0lx\n\r",
							PartitionNum, FsblStatus);
					FsblStatus += XFSBL_ERROR_STAGE_3;
					FsblStage = XFSBL_STAGE_ERR;
				} else {
					XFsbl_Printf(DEBUG_INFO,"Partition %d Load Success \n\r",
									PartitionNum);

					XFsbl_MarkUsedRPUCores(&FsblInstance,
							       PartitionNum);
					/**
					 * Check loading all partitions is completed
					 */

					FsblStatus = XFsbl_CheckEarlyHandoff(&FsblInstance, PartitionNum);

					if (PartitionNum <
						(FsblInstance.ImageHeader.ImageHeaderTable.NoOfPartitions-1U))
					{
						if (TRUE == FsblStatus) {
							EarlyHandoff = TRUE;
							FsblStage = XFSBL_STAGE4;
						}
						else {
							/**
							 * No need to change the Fsbl Stage
							 * Load the next partition
							 */
							PartitionNum++;
						}
					} else {
						/**
						 * No more partitions present, go to handoff stage
						 */
						XFsbl_Printf(DEBUG_INFO,"All Partitions Loaded \n\r");

#ifdef XFSBL_PERF
						XFsbl_MeasurePerfTime(FsblInstance.PerfTime.tFsblStart);
						XFsbl_Printf(DEBUG_PRINT_ALWAYS, ": Total Time \n\r");
						XFsbl_Printf(DEBUG_PRINT_ALWAYS, "Note: Total execution time includes print times \n\r");
#endif
						FsblStage = XFSBL_STAGE4;
						EarlyHandoff = FsblStatus;


						// // Hash the security kernel partition
						// xil_printf("ShEF: Reached Hashing Block\r\n");
						// u8 kernel_hash[XFSBL_HASH_TYPE_SHA3] __attribute__ ((aligned (4))) = {0};
						// u32 Status;

						// u32 kernel_length;
						// const XFsblPs_PartitionHeader * partition_header;
						// PTRSIZE kernel_load_addr;
						
						// // Temporary buffer in DDR for DMA access (TCM is not DMA-accessible during reset)
						// u8* temp_buffer = (u8*)0x10000000;

						// //Partition 1: ATCM (Index 4)
						// partition_header = &FsblInstance.ImageHeader.PartitionHeader[4];
						// kernel_length = partition_header->TotalDataWordLength * 4U;
						// //ATCM address needs offset adjustment for A53 view
						// kernel_load_addr = ((PTRSIZE)(partition_header->DestinationLoadAddress)) + XFSBL_R50_HIGH_ATCM_START_ADDRESS;

						// xil_printf("ShEF: Hashing Partition 4 (ATCM) at %016x, len: %d\r\n", kernel_load_addr, kernel_length);
						
						// // Copy to DDR for DMA
						// xil_printf("ShEF: Copying to DDR buffer @ %p\r\n", temp_buffer);
						// (void)XFsbl_MemCpy(temp_buffer, (u8*)kernel_load_addr, kernel_length);
						
						// // Verify copy
						// xil_printf("ShEF: Data at DDR (first 16 bytes): ");
						// for(int j=0; j<16; j++) xil_printf("%02x ", temp_buffer[j]);
						// xil_printf("\r\n");

						// // Flush cache to ensure DMA sees the data in physical RAM
						// Xil_DCacheFlushRange((INTPTR)temp_buffer, kernel_length);

						// xil_printf("ShEF: Explicitly Re-initializing CSU DMA\r\n");
						// Status = XFsbl_CsuDmaInit(&CsuDma);
						// if (Status != XST_SUCCESS) {
						// 	xil_printf("ShEF: CSU DMA Init failed: %d\r\n", Status);
						// }

						// xil_printf("ShEF: Starting SHA3 Init\r\n");
						// Status = XSecure_Sha3Initialize(&csu_sha3, &CsuDma);
						
						// if (Status != XST_SUCCESS) {
						// 	xil_printf("ShEF: Error! SHA3 Init failed: %d\r\n", Status);
						// } else {
						// 	// START THE ENGINE (Releases Reset)
						// 	xil_printf("ShEF: Starting SHA3 Engine\r\n");
						// 	XSecure_Sha3Start(&csu_sha3);

						// 	// FORCE SSS CONFIGURATION: Route DMA (5) to SHA3 (Bits 11:8)
						// 	// Read current SSS
						// 	u32 current_sss = Xil_In32(0xFFCA0008);
						// 	// Clear SHA3 source (Bits 11:8)
						// 	current_sss &= ~(0xF00);
						// 	// Set SHA3 source to DMA (0x5)
						// 	current_sss |= (0x5 << 8);
						// 	xil_printf("ShEF: Forcing SSS_CFG to %08x\r\n", current_sss);
						// 	Xil_Out32(0xFFCA0008, current_sss);

						// 	// DIAGNOSTICS
						// 	u32 sss_cfg = Xil_In32(0xFFCA0008);
						// 	u32 sha3_rst = Xil_In32(0xFF5E0058); // CRL_APB_RST_LPD_IOU2 (Just for reference)
						// 	u32 dma_status = XCsuDma_ReadReg(CsuDma.Config.BaseAddress, 0x8); // Status offset
							
						// 	xil_printf("ShEF: DIAG - SSS_CFG: %08x, SHA3_RST: %08x, DMA_STS: %08x\r\n", sss_cfg, sha3_rst, dma_status);

						// 	xil_printf("ShEF: Starting SHA3 Update (DMA)\r\n");
						// 	Status = XSecure_Sha3Update(&csu_sha3, temp_buffer, kernel_length);
						// 	if (Status != XST_SUCCESS) {
						// 		xil_printf("ShEF: Error! SHA3 Update failed: %d\r\n", Status);
						// 	} else {
						// 		xil_printf("ShEF: SHA3 Update Done\r\n");
						// 	}
						// }

                        // //Write the kernel hash to OCM
						// xil_printf("ShEF: Kernel Hash (ATCM only): ");
						// int i;
						// for (i = 0; i < XFSBL_HASH_TYPE_SHA3; i++){
						// 	xil_printf("%02x", kernel_hash[i]);
						// 	Xil_Out8(OCM_SEC_BUFFER_ADDRESS + i, kernel_hash[i]);
						// }
						// xil_printf("\r\n");

/*
						//Partition 2: DDR (Index 5)
						partition_header = &FsblInstance.ImageHeader.PartitionHeader[5];
						kernel_length = partition_header ->TotalDataWordLength * 4U;
						kernel_load_addr = ((PTRSIZE)(partition_header->DestinationLoadAddress));

						XFsbl_Printf(DEBUG_GENERAL, "Partition Address: %016x\r\n", kernel_load_addr);
						XFsbl_Printf(DEBUG_GENERAL, "Partition Length: %08d\r\n", kernel_length);

						XSecure_Sha3Update(&csu_sha3, (u8*)kernel_load_addr, kernel_length);

						//Partition 3: OCM (Index 6)
						partition_header = &FsblInstance.ImageHeader.PartitionHeader[6];
						kernel_length = partition_header ->TotalDataWordLength * 4U;
						kernel_load_addr = ((PTRSIZE)(partition_header->DestinationLoadAddress));


						XFsbl_Printf(DEBUG_GENERAL, "Partition Address: %016x\r\n", kernel_load_addr);
						XFsbl_Printf(DEBUG_GENERAL, "Partition Length: %08d\r\n", kernel_length);

						XSecure_Sha3Update(&csu_sha3, (u8*)kernel_load_addr, kernel_length);

						Status = XSecure_Sha3Finish(&csu_sha3, kernel_hash);
						if (Status != XST_SUCCESS) {
							xil_printf("ShEF: Error! SHA3 Finish failed: %d\r\n", Status);
						} else {
							xil_printf("ShEF: SHA3 Finish Done. Total Len: %d\r\n", csu_sha3.Sha3Len);
						}


						//Generate the keygen seed using the kernel hash and the device key
						XSecure_Sha3Initialize(&csu_sha3, &CsuDma);
						XSecure_Sha3Update(&csu_sha3, kernel_hash, XFSBL_HASH_TYPE_SHA3);
						XSecure_Sha3Update(&csu_sha3, root_sk, 512);
						XSecure_Sha3Update(&csu_sha3, root_mod, 512);


						XSecure_Sha3Finish(&csu_sha3, kernel_hash);

						XFsbl_Printf(DEBUG_GENERAL,"\r\nKeygen Seed: ");

						//Truncate the SHA3-384 output to SHA3-256
						for (i = 0; i < 32; i++){
							XFsbl_Printf(DEBUG_GENERAL, "%02x", kernel_hash[i]);
							Xil_Out8(OCM_SEC_BUFFER_ADDRESS + XFSBL_HASH_TYPE_SHA3 + i, kernel_hash[i]);
						}
*/
					}
				} /* End of else loop for Load Success */
			} break;

		case XFSBL_STAGE4:
			{

				XFsbl_Printf(DEBUG_INFO,
						"================= In Stage 4 ============ \n\r");

				XFsbl_Printf(DEBUG_PRINT_ALWAYS, "--- FSBL Handoff Stage ---\n\r");
				XFsbl_Printf(DEBUG_PRINT_ALWAYS, "*** ShEF FSBL Porting Complete. Proceeding to Handoff. ***\n\r");
				/**
				 * Handoff to the applications
				 * Handoff address
				 * xip
				 * ps7 post config
				 */
				FsblStatus = XFsbl_Handoff(&FsblInstance, PartitionNum, EarlyHandoff);

				if (XFSBL_STATUS_CONTINUE_PARTITION_LOAD == FsblStatus) {
					XFsbl_Printf(DEBUG_INFO,"Early handoff to a application complete \n\r");
					XFsbl_Printf(DEBUG_INFO,"Continuing to load remaining partitions \n\r");

					PartitionNum++;
					FsblStage = XFSBL_STAGE3;
				}
				else if (XFSBL_STATUS_CONTINUE_OTHER_HANDOFF == FsblStatus) {
					XFsbl_Printf(DEBUG_INFO,"Early handoff to a application complete \n\r");
					XFsbl_Printf(DEBUG_INFO,"Continuing handoff to other applications, if present \n\r");
					EarlyHandoff = FALSE;
				}
				else if (XFSBL_SUCCESS != FsblStatus) {
					/**
					 * Error
					 */
					XFsbl_Printf(DEBUG_GENERAL,"Handoff Failed 0x%0lx\n\r", FsblStatus);
					FsblStatus += XFSBL_ERROR_STAGE_4;
					FsblStage = XFSBL_STAGE_ERR;
				} else {
					/**
					 * we should never be here
					 */
					FsblStage = XFSBL_STAGE_DEFAULT;
				}
			} break;

		case XFSBL_STAGE_ERR:
			{
				XFsbl_Printf(DEBUG_INFO,
						"================= In Stage Err ============ \n\r");

				XFsbl_ErrorLockDown(FsblStatus);
				/**
				 * we should never be here
				 */
				FsblStage = XFSBL_STAGE_DEFAULT;
			}break;

		case XFSBL_STAGE_DEFAULT:
		default:
			{
				/**
				 * we should never be here
				 */
				XFsbl_Printf(DEBUG_GENERAL,"In default stage: "
						"We should never be here \n\r");

				/**
				 * Exit FSBL
				 */
				XFsbl_HandoffExit(0U, XFSBL_NO_HANDOFFEXIT);

			}break;

		} /* End of switch(FsblStage) */
		if(FsblStage==XFSBL_STAGE_DEFAULT) {
			break;
		}
	} /* End of while(1)  */

	/**
	 * We should never be here
	 */
	XFsbl_Printf(DEBUG_GENERAL,"In default stage: "
				"We should never be here \n\r");
	/**
	 * Exit FSBL
	 */
	XFsbl_HandoffExit(0U, XFSBL_NO_HANDOFFEXIT);

	return 0;
}

void XFsbl_PrintFsblBanner(void )
{
	s32 PlatInfo;
	/**
	 * Print the FSBL Banner
	 */
#if !defined(XFSBL_PERF) || defined(FSBL_DEBUG) || defined(FSBL_DEBUG_INFO) \
			|| defined(FSBL_DEBUG_DETAILED)
	XFsbl_Printf(DEBUG_PRINT_ALWAYS,
                 "Zynq MP First Stage Boot Loader \n\r");
	XFsbl_Printf(DEBUG_PRINT_ALWAYS,
                 "Release %d.%d   %s  -  %s\r\n",
                 SDK_RELEASE_YEAR, SDK_RELEASE_QUARTER,__DATE__,__TIME__);

	XFsbl_Printf(DEBUG_GENERAL, "MultiBootOffset: 0x%0x\r\n",
		XFsbl_In32(CSU_CSU_MULTI_BOOT));

	if(FsblInstance.ResetReason == XFSBL_PS_ONLY_RESET) {
		XFsbl_Printf(DEBUG_GENERAL,"Reset Mode	:	PS Only Reset\r\n");
	} else if (XFSBL_MASTER_ONLY_RESET == FsblInstance.ResetReason) {
		XFsbl_Printf(DEBUG_GENERAL,"Reset Mode	:	Master Subsystem Only Reset\r\n");
	} else if(FsblInstance.ResetReason == XFSBL_SYSTEM_RESET) {
		XFsbl_Printf(DEBUG_GENERAL,"Reset Mode	:	System Reset\r\n");
	} else {
		/*MISRAC compliance*/
	}
#endif

	/**
	 * Print the platform
	 */

	PlatInfo = (s32)XGet_Zynq_UltraMp_Platform_info();
	if (PlatInfo == XPLAT_ZYNQ_ULTRA_MPQEMU)
	{
		XFsbl_Printf(DEBUG_GENERAL, "Platform: QEMU, ");
	} else  if (PlatInfo == XPLAT_ZYNQ_ULTRA_MP)
	{
		XFsbl_Printf(DEBUG_GENERAL, "Platform: REMUS, ");
	} else  if (PlatInfo == XPLAT_ZYNQ_ULTRA_MP_SILICON)
	{
		XFsbl_Printf(DEBUG_GENERAL, "Platform: Silicon (%d.0), ",
				XGetPSVersion_Info()+1U);
	} else {
		XFsbl_Printf(DEBUG_GENERAL, "Platform Not identified \r\n");
	}

	return ;
}



/*****************************************************************************/
/**
 * This function is called in FSBL error cases. Error status
 * register is updated and fallback is applied
 *
 * @param ErrorStatus is the error code which is written to the
 * 		  error status register
 *
 * @return none
 *
 * @note Fallback is applied only for fallback supported bootmodes
 *****************************************************************************/
void XFsbl_ErrorLockDown(u32 ErrorStatus)
{
	u32 BootMode;

	/**
	 * Print the FSBL error
	 */
	XFsbl_Printf(DEBUG_GENERAL,"Fsbl Error Status: 0x%08lx\r\n",
		ErrorStatus);

	/**
	 * Update the error status register
	 * and Fsbl instance structure
	 */
	XFsbl_Out32(XFSBL_ERROR_STATUS_REGISTER_OFFSET, ErrorStatus);
	FsblInstance.ErrorCode = ErrorStatus;

	/**
	 * Read Boot Mode register
	 */
	BootMode = XFsbl_In32(CRL_APB_BOOT_MODE_USER) &
			CRL_APB_BOOT_MODE_USER_BOOT_MODE_MASK;

	/**
	 * Fallback if bootmode supports
	 */
	if ( (BootMode == XFSBL_QSPI24_BOOT_MODE) ||
			(BootMode == XFSBL_QSPI32_BOOT_MODE) ||
			(BootMode == XFSBL_NAND_BOOT_MODE) ||
			(BootMode == XFSBL_SD0_BOOT_MODE) ||
			(BootMode == XFSBL_EMMC_BOOT_MODE) ||
			(BootMode == XFSBL_SD1_BOOT_MODE) ||
			(BootMode == XFSBL_SD1_LS_BOOT_MODE) )
	{
		XFsbl_FallBack();
	} else {
		/**
		 * Be in while loop if fallback is not supported
		 */
		XFsbl_Printf(DEBUG_GENERAL,"Fallback not supported \n\r");

		/**
		 * Exit FSBL
		 */
		XFsbl_HandoffExit(0U, XFSBL_NO_HANDOFFEXIT);
	}

	/**
	 * Should never be here
	 */
	return ;
}

/*****************************************************************************/
/**
 * In Fallback,  soft reset is applied to the system after incrementing
 * the multiboot register. A hook is provided to before the fallback so
 * that users can write their own code before soft reset
 *
 * @param none
 *
 * @return none
 *
 * @note We will not return from this function as it does soft reset
 *****************************************************************************/
static void XFsbl_FallBack(void)
{
	u32 RegValue;

#ifdef XFSBL_WDT_PRESENT
	if (XFSBL_MASTER_ONLY_RESET != FsblInstance.ResetReason) {
		/* Stop WDT as we are restarting */
		XFsbl_StopWdt();
	}
#endif

	/* Hook before FSBL Fallback */
	(void)XFsbl_HookBeforeFallback();

	/* Read the Multiboot register */
	RegValue = XFsbl_In32(CSU_CSU_MULTI_BOOT);

	XFsbl_Printf(DEBUG_GENERAL,"Performing FSBL FallBack\n\r");

	XFsbl_UpdateMultiBoot(RegValue+1U);

	return;
}


/*****************************************************************************/
/**
 * This is the function which actually updates the multiboot register and
 * does the soft reset. This function is called in fallback case and
 * in the cases where user would like to jump to a different image,
 * corresponding to the multiboot value being passed to this function.
 * The latter case is a generic one and need arise because of error scenario.
 *
 * @param MultiBootValue is the new value for the multiboot register
 *
 * @return none
 *
 * @note We will not return from this function as it does soft reset
 *****************************************************************************/

static void XFsbl_UpdateMultiBoot(u32 MultiBootValue)
{
	u32 RegValue;

	XFsbl_Out32(CSU_CSU_MULTI_BOOT, MultiBootValue);

	/**
	 * Due to a bug in 1.0 Silicon, PS hangs after System Reset if RPLL is used.
	 * Hence, just for 1.0 Silicon, bypass the RPLL clock before giving
	 * System Reset.
	 */
	if (XGetPSVersion_Info() == (u32)XPS_VERSION_1)
	{
		RegValue = XFsbl_In32(CRL_APB_RPLL_CTRL) |
				CRL_APB_RPLL_CTRL_BYPASS_MASK;
		XFsbl_Out32(CRL_APB_RPLL_CTRL, RegValue);
	}

	/* make sure every thing completes */
	dsb();
	isb();

	if (XFSBL_MASTER_ONLY_RESET != FsblInstance.ResetReason) {

	/* Soft reset the system */
	XFsbl_Printf(DEBUG_GENERAL,"Performing System Soft Reset\n\r");
	RegValue = XFsbl_In32(CRL_APB_RESET_CTRL);
	XFsbl_Out32(CRL_APB_RESET_CTRL,
			RegValue|CRL_APB_RESET_CTRL_SOFT_RESET_MASK);

	/* wait here until reset happens */
	while(1) {
	;
	}
	}
	else
	{
		for(;;){
			/*We should not be here*/
		}
	}

	return;

}

/*****************************************************************************/
/**
 * This function measures the total time taken between two points for FSBL
 * performance measurement.
 *
 * @param Current/start time
 *
 * @return none
 *
 * @note none
 *****************************************************************************/
#ifdef XFSBL_PERF

void XFsbl_MeasurePerfTime(XTime tCur)
{
	XTime tEnd = 0;
	XTime tDiff = 0;
	u64 tPerfNs;
	u64 tPerfMs = 0;
	u64 tPerfMsFrac = 0;

	XTime_GetTime(&tEnd);
	tDiff = tEnd - tCur;

	/* Convert tPerf into nanoseconds */
	tPerfNs = ((double)tDiff / (double)COUNTS_PER_SECOND) * 1e9;

	tPerfMs = tPerfNs / 1e6;
	tPerfMsFrac = tPerfNs % (u64)1e6;

	/* Print the whole (in ms.) and fractional part */
	XFsbl_Printf(DEBUG_PRINT_ALWAYS, "%d.%06d ms.",
			(u32)tPerfMs, (u32)tPerfMsFrac);
}

#endif

static void XFsbl_MarkUsedRPUCores(XFsblPs *FsblInstPtr, u32 PartitionNum)
{
	u32 DestCpu, RegValue;

	DestCpu = XFsbl_GetDestinationCpu(&FsblInstPtr->ImageHeader.
					  PartitionHeader[PartitionNum]);

	RegValue = Xil_In32(XFSBL_R5_USAGE_STATUS_REG);

	/*
	 * Check if any RPU core is used. If it is used set particular bit of
	 * that core to indicate PMU that it is used and it is not need to
	 * power down.
	 */
	switch (DestCpu) {
	case XIH_PH_ATTRB_DEST_CPU_R5_0:
	case XIH_PH_ATTRB_DEST_CPU_R5_L:
		Xil_Out32(XFSBL_R5_USAGE_STATUS_REG, RegValue |
			  XFSBL_R5_0_STATUS_MASK);
		break;
	case XIH_PH_ATTRB_DEST_CPU_R5_1:
		Xil_Out32(XFSBL_R5_USAGE_STATUS_REG, RegValue |
			  XFSBL_R5_1_STATUS_MASK);
		break;
	case XIH_PH_ATTRB_DEST_CPU_NONE:
		if ((FsblInstance.ProcessorID == XIH_PH_ATTRB_DEST_CPU_R5_0) ||
		    (FsblInstance.ProcessorID == XIH_PH_ATTRB_DEST_CPU_R5_L)) {
			Xil_Out32(XFSBL_R5_USAGE_STATUS_REG, RegValue |
				  XFSBL_R5_0_STATUS_MASK);
		}
	}
}
