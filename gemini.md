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
- **`/vitis/` Folder**: Contains my *current* active workspace (Vitis 2023.2). This is where edits happen.
  - **`vitis/pmufw/src/`**: The active source code for the PMU Firmware.
  - **`vitis/zynqmp_fsbl/src/`**: The active source code for the FSBL
  - **`vitis/hello_world/src/`**: A **temporary** placeholder application. The code here is for initial hardware validation and will be replaced by the `security_kernel` at a later stage.

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
- [ ] In `vitis/zynqmp_fsbl/xfsbl_main.c`, update commented-out hashing logic to use the correct memory addresses and size for the `hello_world` application's executable code section.
- [ ] Demonstrate working hashing of `hello_world` code by `zynqmp_fsbl`
- [ ] Port Security Kernel source from `u96/` to a new `vitis/security_kernel/` project.
- [ ] Successfully compile ported Security Kernel.
- [ ] Integrate Security Kernel into the boot flow, replacing `hello_world`.
- [ ] Full system test of the ShEF boot flow on hardware.