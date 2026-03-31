import vitis
import shutil
import os
import re
import subprocess
import platform as py_platform

print("Starting ShEF Vitis 2023.2 Automation Script...")

# ==========================================
# Phase 1: Initialize Client & Workspace
# ==========================================
client = vitis.create_client()
workspace_path = os.path.normpath("../embedded_shef")
client.set_workspace(path=workspace_path)

# ==========================================
# Phase 2: Create Platform & Domains
# ==========================================
print("Generating base platform and domains...")
platform_comp = client.create_platform_component(
    name="platform", 
    hw=os.path.normpath("./nec_shef.xsa"), 
    os="standalone", 
    cpu="psu_cortexa53_0"
)

# A53 Domain (Runtime)
domain_a53 = platform_comp.get_domain(name="standalone_psu_cortexa53_0")
domain_a53.set_lib("xilffs")

# R5 Domain (Security Kernel)
platform_comp.add_domain(cpu="psu_cortexr5_0", os="standalone", name="standalone_psu_cortexr5_0")
domain_r5 = platform_comp.get_domain(name="standalone_psu_cortexr5_0")
domain_r5.set_lib("xilsecure")
domain_r5.set_lib("xilpm")

# PMU Domain
platform_comp.add_domain(name="standalone_psu_pmu_0", cpu="psu_pmu_0", os="standalone")

# Build the platform once here so the platform.xpfm file and FSBL directory are generated
print("Building initial platform to generate BSPs and FSBL...")
platform_comp.build()

# ==========================================
# Phase 3: Create Application Components
# ==========================================
print("Creating application components...")
xpfm_path = os.path.join(workspace_path, "platform", "export", "platform", "platform.xpfm")

pmufw_app = client.create_app_component(
    name="shef_pmufw", 
    platform=xpfm_path,
    domain="standalone_psu_pmu_0",
    template="zynqmp_pmufw"
)

runtime_app = client.create_app_component(
    name="runtime",
    platform=xpfm_path,
    domain="standalone_psu_cortexa53_0",
    template="empty_application"
)

security_kernel_app = client.create_app_component(
    name="security_kernel",
    platform=xpfm_path,
    domain="standalone_psu_cortexr5_0",
    template="empty_application"
)

# ==========================================
# Phase 4: Inject Custom Source Files
# ==========================================
print("Swapping custom ShEF source files...")
src_base = "."
dest_base = workspace_path

# Map directories
copy_map = {
    os.path.join(src_base, "runtime"): os.path.join(dest_base, "runtime", "src"),
    os.path.join(src_base, "security_kernel"): os.path.join(dest_base, "security_kernel", "src"),
    os.path.join(src_base, "pmufw"): os.path.join(dest_base, "shef_pmufw", "src"),
    os.path.join(src_base, "fsbl"): os.path.join(dest_base, "platform", "zynqmp_fsbl")
}

# Copy main sources
for src, dest in copy_map.items():
    shutil.copytree(src, dest, dirs_exist_ok=True)

# Copy shared includes to all destinations
src_include = os.path.join(src_base, "include")
for dest in copy_map.values():
    shutil.copytree(src_include, dest, dirs_exist_ok=True)

# ==========================================
# Phase 5: Apply Compiler Configurations
# ==========================================
print("Applying CMake compiler definitions and optimizations...")

# Standard Application Configs
runtime_app.set_app_config(key='USER_COMPILE_DEFINITIONS', values=['USE_SHEF_FIXED_CONFIG'])
pmufw_app.set_app_config(key='USER_COMPILE_DEFINITIONS', values=['USE_SHEF_FIXED_CONFIG'])

security_kernel_app.set_app_config(key='USER_COMPILE_DEFINITIONS', values=['USE_SHEF_FIXED_CONFIG'])
security_kernel_app.set_app_config(key='USER_COMPILE_OPTIMIZATION_LEVEL', values=['-Os'])

# FSBL CMake Regex Workaround (Since FSBL is a sub-component, not a top-level app)
fsbl_cmake_path = os.path.join(dest_base, "platform", "zynqmp_fsbl", "UserConfig.cmake")
with open(fsbl_cmake_path, 'r') as f:
    content = f.read()
    
if "USE_SHEF_FIXED_CONFIG" not in content:
    updated_content = re.sub(
        r'(set\(USER_COMPILE_DEFINITIONS\s*)""(\s*\))',
        r'\1"USE_SHEF_FIXED_CONFIG"\2',
        content
    )
    with open(fsbl_cmake_path, 'w') as f:
        f.write(updated_content)

# ==========================================
# Phase 6: Final Compilation
# ==========================================
print("Rebuilding platform to catch FSBL config changes...")
platform_comp.build()

print("Compiling application components into .elf files...")
runtime_app.build()
security_kernel_app.build()
pmufw_app.build()

# ==========================================
# Phase 7: Package BOOT.BIN
# ==========================================
print("Generating package.bif and executing bootgen...")

# Paths inside BIF can typically use forward slashes safely on both OSs
bif_content = f"""//arch = zynqmp; split = false; format = BIN
the_ROM_image:
{{
    [bootloader, destination_cpu = a53-0] {workspace_path}/platform/zynqmp_fsbl/build/fsbl.elf
    [destination_cpu = pmu] {workspace_path}/shef_pmufw/build/shef_pmufw.elf
    [destination_cpu = r5-0] {workspace_path}/security_kernel/build/security_kernel.elf
    [destination_cpu = a53-0] {workspace_path}/runtime/build/runtime.elf
}}
"""
bif_filename = "package.bif"
with open(bif_filename, "w") as f:
    f.write(bif_content)

# Detect OS and set bootgen executable
is_windows = py_platform.system() == "Windows"
bootgen_exe = "bootgen.bat" if is_windows else "bootgen"
output_bin = os.path.join(workspace_path, "BOOT.BIN")

# Construct command
bootgen_cmd = [
    bootgen_exe, 
    "-arch", "zynqmp", 
    "-image", bif_filename, 
    "-w", 
    "-o", output_bin
]

print(f"Executing: {' '.join(bootgen_cmd)}")

# Run bootgen - use shell=True only on Windows for .bat files if needed, 
# though list-style usually works with the full name.
result = subprocess.run(bootgen_cmd, shell=is_windows, capture_output=True, text=True)

if os.path.exists(output_bin):
    print(f"\nSUCCESS! Complete ShEF BOOT.BIN successfully generated at: {output_bin}")
else:
    print("\nFAILURE: BOOT.BIN was not created. Check bootgen output:")
    print(f"STDOUT: {result.stdout}")
    print(f"STDERR: {result.stderr}")

# Clean up client memory
vitis.dispose()