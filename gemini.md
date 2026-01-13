# Project Context: ShEF Port to ZuBoard 1CG

## 1. Project Goal
I am recreating the "ShEF" (Shielded Embedded Firmware) security framework. 
- **Original Hardware:** Ultra96 (ZynqMP ZU3EG)
- **Target Hardware:** Avnet ZuBoard 1CG (ZynqMP ZU1CG)
- **Objective:** Re-implement the secure boot flow (PMU Firmware -> Security Kernel -> Host OS) on the new board.

## 2. Environment & Tooling
- **Current Environment:** Vitis 2023.2 (Unified IDE) / Vivado 2023.2
- **Original Environment:** Vitis/SDK 2019.2
- **Critical Constraint:** The original code contains outdated Xilinx drivers and definitions. 
  - *Example:* The `XFpga` and `CSUDMA` libraries have changed significantly.
  - *Action:* You must identify deprecated functions in the original code and refactor them to use the modern Vitis 2023.2 APIs (e.g., `XFpga_BitStream_Load` instead of `XFpga_PL_BitSream_Load`).

## 3. Directory Structure & Key Files
- **`shef_paper.pdf`**: The original academic paper defining the security architecture. Read this for conceptual logic (e.g., "Why does the PMU load the R5?").
- **`u96/` Folder**: Contains the *original* reference source code (Vitis 2019.2). Use this to understand the *intent* of the code.
- **`vitis_2/` Folder**: The active workspace as of 2026-01-01.
- **`vitis_2/platform/zynqmp_fsbl`**: The current working baseline FSBL project.
- **`/vitis/` Folder**: Deprecated workspace.

### Technical Documents
The contents of the ShEF paper have been saved to `E:x\shef\Documents.ShEF_paper.txt`. Reference this document for questions relevant to the system architecture.
Key chapters from the Zynq UltraScale+ MPSoC Technical Reference Manual (ug1085) have been saved as `.txt` files in `E:\x\shef\Documents\` for efficient searching.
- `ultrascale_TRM_CH03.txt` (Application Processing Unit)
- `ultrascale_TRM_CH04.txt` (Real-time Processing Unit)
- `ultrascale_TRM_CH06.txt` (Platform Management Unit)
- `ultrascale_TRM_CH10.txt` (System Addresses)
- `ultrascale_TRM_CH11.txt` (Boot and Configuration)
- `ultrascale_TRM_CH12.txt` (Security)
- `ultrascale_TRM_CH13.txt` (Interrupts)
- `ultrascale_TRM_CH19.txt` (DMA Controller)

## 4. Key Architecture Concepts
- **PMU as Root-of-Trust:** The PMU Firmware is modified to include RSA-4096 keys. It verifies the Security Kernel before releasing its reset.
- **Boot Flow:** 1. BootROM loads PMUFW.
  2. PMUFW initializes ShEF modules.
  3. PMUFW loads/verifies Security Kernel (R5).
  4. System hands off to FSBL (A53).

## 5. Instructions for Code Generation
When suggesting code changes:
1. Check if the driver API has changed between 2019 and 2023.
2. If `xparameters.h` constants (like `XPAR_...`) are missing, assume the System Device Tree (SDT) flow is used and look for the generic `_0` or `_1` indices.
3. Ensure all pointers passed to hardware drivers (DMA/SHA) are cast to non-volatile types.
4. When providing a suggestion, explicitly state the relevant line of code where this suggestion starts. For example: If you recommend inserting a new function in the code that begins at line 230, you'll state that the changes begin at line 230.

## 6. Project Milestones
- [X] Verify basic `hello_world` functionality on ZuBoard 1CG.
- [X] Port PMUFW source from `u96/` to `vitis/pmufw/`.
- [X] Successfully compile ported PMUFW.
- [X] Partially Port custom FSBL source from `u96/` to `vitis/zynqmp_fsbl/`.
- [X] Create a new, working baseline FSBL for the A53 core in a clean `vitis_2` workspace.
- [X] Port custom hashing and handoff logic from `u96/fsbl` into the new `vitis_2` FSBL.
- [ ] Re-enable and integrate secure boot features into the `vitis_2` FSBL.
- [ ] In `vitis_2/platform/zynqmp_fsbl/xfsbl_main.c`, update hashing logic to use the correct memory addresses and size for the `security_kernel` application's executable code sections.
- [X] Port Security Kernel source from `u96/` to a new `vitis_2/security_kernel/` project.
- [X] Successfully compile ported Security Kernel.
- [~] Test the full system boot flow (FSBL -> Security Kernel -> Runtime) on the QEMU simulator. (Skipped: Moving directly to hardware)
- [X] Verify basic boot flow (FSBL -> Security Kernel -> Runtime) on hardware with ShEF features disabled.
- [ ] Full system test of the ShEF boot flow on hardware.

## 7. Current Status & Next Steps

### As of January 13, 2026 (End of Day)
- **FSBL Hashing & KeyGen Complete:** The FSBL now correctly measures all three partitions (ATCM, DDR, OCM) of the Security Kernel and generates a KeyGen Seed. The "DDR Buffer Workaround" was removed after confirming direct TCM hashing is possible with correct initialization.
- **Security Kernel Initialization Fix:** Resolved a hard crash in `rpu_gic_init` by hardcoding the IPI Interrupt ID to `65` (the macro `XPAR_XIPIPSU_0_INTERRUPTS` was resolving to an invalid value).
- **Chain of Trust Verified:** Confirmed that the Security Kernel (R5) successfully reads the exact same `Kernel Hash` and `Keygen Seed` from OCM that the FSBL (A53) wrote.
- **Runtime Functional:** The Runtime application boots and reaches its command loop.

### Next Steps
1.  **Test Key Generation:** Re-enable `ed25519_create_keypair` in the Security Kernel to verify the reduced stack size (0xD00) can handle the cryptographic load.
2.  **Test Certificate Generation:** Re-enable the `get_kernel_certificate_hash` and `get_kernel_certificate_signature` functions to test the IPI communication with the PMU.
3.  **Full System Integration:** Uncomment the final logic to allow the Runtime to trigger the attestation flow.