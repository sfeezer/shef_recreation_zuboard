# Project Context: ShEF Port to ZuBoard 1CG (Methodology & Automation Phase)

## 1. Project Goal
I am recreating and documenting the "ShEF" (Shielded Embedded Firmware) security framework for the Avnet ZuBoard 1CG.
- **Original Hardware:** Ultra96 (ZynqMP ZU3EG)
- **Target Hardware:** Avnet ZuBoard 1CG (ZynqMP ZU1CG)
- **Objective:** Establish a repeatable methodology and automation suite to recreate the secure boot flow (PMU Firmware -> Security Kernel -> Host OS) on the ZuBoard.

## 2. Environment & Tooling
- **Current Environment:** Vitis 2023.2 (Unified IDE) / Vivado 2023.2
- **Host OS:** Windows (win32)
- **Original Environment:** Vitis/SDK 2019.2
- **Automation Targets:** Vitis CLI (`vitis -i`), XSCT, Bootgen, and Python for scripting.

## 3. Directory Structure & Key Files
- **`shef_zu1cg/` Folder**: The **Source of Truth**. Contains the partner's working ShEF implementation.
- **`shef_source_materials/` Folder**: Clean staging area containing essential source files (.c, .h, .S, .ld) and .xsa for reproduction and automation.
  - `pmufw/`, `fsbl/`, `security_kernel/`, `runtime/`, `host/`, `keys/`
- **`u96/` Folder**: The *original* reference source code (Vitis 2019.2) used for legacy comparison.
- **`Documents/`**: Contains technical documentation, including the ShEF paper and ZynqMP TRM chapters.

## 4. Key Architecture Concepts
- **PMU as Root-of-Trust:** The PMU Firmware is modified to include RSA-4096 keys. It verifies the Security Kernel before releasing its reset.
- **Boot Flow:** 
  1. BootROM loads PMUFW.
  2. PMUFW initializes ShEF modules.
  3. PMUFW loads/verifies Security Kernel (R5).
  4. System hands off to FSBL (A53).
- **Split-Channel IPI:** Uses IPI-0 for master requests and IPI-1 for PMU responses to avoid race conditions and ISR errors.

## 5. Future Automation Requirements
- **Key Generation & Integration:** Automate the generation of RSA and Ed25519 keys, and the insertion of these keys into the PMUFW and host-side metadata.
- **Toolchain Portability:** Automate the compilation of the `ed25519` verification utility to ensure the host script works on any new developer machine (Windows or Linux).

## 6. Instructions for Workflow & Code Generation
1. **Step-by-Step Confirmation:** Do not rush ahead. When a milestone is finished, always discuss the path forward with the user before beginning work on the next step.
2. **Reproducibility First:** Every code change or build step must be documented so it can be automated later.
3. **Handle Hard-coding:** Identify and flag hard-coded addresses or parameters (e.g., memory offsets, key indices) for future parameterization.
4. **Line-Specific Suggestions:** When providing a suggestion, explicitly state the relevant line of code where this suggestion starts.

## 6. Project Roadmap

### Milestone 1: Reproduction (Complete)
- [X] Explore `shef_zu1cg` to identify build requirements and dependencies.
- [X] Stage essential source files from `shef_zu1cg` into `shef_source_materials`.
- [X] Compile all components (PMUFW, FSBL, Security Kernel, Runtime) from the staged source.
- [X] Generate a new working `BOOT.BIN` using `bootgen`.
- [X] Verify functionality on hardware (successful boot and attestation).

### Milestone 2: Documentation & Delta Analysis (Complete)
- [X] Compare `shef_zu1cg` code against the original `u96` source.
- [X] Document all necessary refactors (RSA API shifts, IPI interrupt ID mapping, memory expansions).
- [X] Map the exact "recipe" required to port ShEF to a new ZynqMP target (documented in COMPARISON_REPORT.txt).

### Milestone 3: Configuration Interface & Parameterization (In Progress)
- [X] Identify and flag problematic hard-coded parameters (memory offsets, IPI masks, key indices).
- [X] Create a central configuration header (`shef_config.h`) and bridge (`shef_env.h`) to manage parameters.
- [X] Replace hard-coded values in the embedded source (PMUFW, FSBL, Security Kernel, Runtime) with macros.
- [ ] Synchronize the host-side Python script with the new configuration system.

### Milestone 4: Full Project Automation
- [ ] Develop scripts to automate the generation of fresh cryptographic keys and their integration into source headers.
- [ ] Automate the compilation of host-side tools (e.g., ed25519) from source.
- [ ] Develop TCL/Python scripts to automate workspace creation in Vitis 2023.2.
- [ ] Implement a one-command build process for the entire ShEF stack.

## 7. Current Status & Next Steps

### As of February 16, 2026

- **Goal:** Complete parameterization and bridge headers.

- **Status:** 

    - Milestone 2 & 3: Refactoring complete for all embedded components (FSBL, PMUFW, Security Kernel, Runtime).

    - Toggle system (`USE_SHEF_FIXED_CONFIG`) implemented and verified.

    - Hardware Test: Security Kernel executes and generates real signatures using centralized keys.

- **Next Step:** 

    - Refactor `host.py` to use a shared Python-based configuration file (`shef_config.py`) to resolve key mismatches.

    - Begin Milestone 4 (Automation).
