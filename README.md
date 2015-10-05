# HSA Debugger September 2015 Beta Release (version 0.6)

## Overview
The HSA Debugger provides a gdb-based debugging environment for debugging host application and HSAIL kernels running on AMD HSA platforms.  The kernel programming language currently supported is HSAIL 1.0.  There are two packages included in this Alpha Release:
* AMD HSAIL gdb package that contains the hsail-gdb tool 
  * based on GDB 7.8, the GNU source-level debugger
* AMD GPU Kernel Debug SDK package that contains the necessary header, library and sample files to run the hsail-gdb tool

## Table of Contents
* [Major Features](#Major)
* [What's New](#WhatsNew)
* [System Requirements](#System)
* [Package Contents](#Package)
* [Installation](#Installation)
* [Usage Examples](TUTORIAL.md)
* [Known Issues](#Known)
* [HSAIL gdb LICENSE](AMDHsailGdb/LICENSE.txt) and [SDK LICENSE](AMDGPUKernelDebugSDK/LICENSE.txt)

<A NAME="Major">
## Major Features
* Seamless host application and HSAIL kernel source debugging using a familiar gdb-based debugging environment on AMD HSA platforms
* Set HSAIL kernel breakpoints, single stepping and inspect HSAIL registers within HSAIL kernel source
* View active GPU states (active work-groups, work-items and wavefronts information)

<A NAME="WhatsNew">
## What's New in September 2015 Beta Release (version 0.6)
* Initial AMD Carrizo support (alpha).
* Improves support for HSAIL-level debugging for Kalmar applications.
* Adds support for conditional breakpoint based on the global work-item id.
* Improves support for running hsail-gdb on hsa cluster machines.
* Improves support when terminating application or hsail-gdb abruptly.
* Adds support to step into the kernel from the kernel function breakpoint using the gdb *next* command.
* Add a utility program *amd-debug-lock* to determine whether the hsa system is in a gpu locked state (requiring a system reboot to release the lock).
* Adds debian installation packages.
* Adds common prefix name for all log files generated using *HSAIL_GDB_ENABLE_LOG* environment variable.
* Enables running hsail-gdb when it is installed in a read only folder
* Removes version number from the AMDHsailGdb and AMDGPUKernelDebugSDK folder names.
* Fixes a segmentation fault if the users entered incorrect hsail breakpoint commands
* Fixes incorrect reporting of number of work-groups in *info hsail kernels* command when there are partial work-groups.
* Fixes incorrect reporting of the focus work-item in *info hsail wis* command
* Fixes incorrect reporting of the work-group id that is being switched to after the first step into the kernel.
* The temporary temp_source kernel file for debugging is now deleted after the hsail-gdb session completes.

## What's New in May 2015 Alpha Release (version 0.5)
* Initial AMD internal release.
* Initial AMD Kaveri support (alpha).

<A NAME="System">
## System Requirements
* Hardware: AMD Kaveri (beta support) and Carrizo (alpha support) APUs that implement HSA (i.e. A10-7850K, A10-7800, A10-8700, etc.)
  * Refer to the [HSA Platforms & Installation wiki page](https://github.com/HSAFoundation/HSA-Docs-AMD/wiki/HSA-Platforms-&-Installation) for additional information.
* OS: 64-bit Ubuntu 14.04
* HSA Runtime: [AMD HSA Runtime September 2015 release v1.0.3](https://github.com/HSAFoundation/HSA-Runtime-AMD)
* HSA Driver: [AMD HSA Drivers v1.6](https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD)

HSAIL applications must use the [LibHSAIL from July 27th 2015 or newer](https://github.com/HSAFoundation/HSAIL-Tools) built with *BUILD_WITH_LIBBRIGDWARF=1* to assemble HSAIL text to BRIG.

<A NAME="Package">
## Package Contents
The directory structure of the HSA Debugger packages:
* *AMDGPUKernelDebugSDK*
  * *include*
    * *AMDGPUDebug.h*, *amd_hsa_tools_interfaces.h*
  * *bin/x86_64*
    * *amd-debug-lock*
  * *lib/x86_64*
    * *libAMDGPUDebugHSA-x64.so*, *libAMDHSADebugAgent-x64.so*, *libAMDHwDbgFacilities-x64.so*
  * *samples*
    * *Common*
	  * *HSAResourceManager.h*
	  * *HSAResourceManager.cpp*
	* *MatrixMultiplication*
	  * *Makefile*, *MatrixMul.cpp*, *matrixMul_kernel.brig*, *matrixMul_kernel.hsail*
  * *LICENSE.txt*
* *AMDHsailGdb*
  * *bin/x86_64*
    * *hsail-gdb*, *amd-gdb*, *.gdbinit*
  * *LICENSE.txt*
* *ubuntu*
  * *amd-gpu-kernel-debug-sdk_0.6.856_amd64.deb*
  * *amd-hsail-gdb_0.6.856_amd64.deb*
  
If you download the HSA Debugger packages or files separately, you must create the same directory structure as shown above in order to run hsail-gdb successfully.
  
<A NAME="Installation">
## Installation
First, make sure that the HSA system is setup correctly
* [Install HSA Driver](https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD#installing-and-configuring-the-kernel)
* [Install HSA Runtime](https://github.com/HSAFoundation/HSA-Runtime-AMD/#installing-and-configuring-the-hsa-runtime)
* [Verify the setup by running HSAIL *vector_copy* sample successfully](https://github.com/HSAFoundation/HSA-Runtime-AMD#running-the-sample---vector_copy)
  * Note that the *vector_copy* sample can't be debugged with hsail-gdb since the pre-generated BRIG file was not created with debugging support.
    As part of the HSAIL debugger package, there is a sample HSAIL *MatrixMultiplication* that can be used with hsail-gdb.
  
###HSA Debugger Installation
1. Download the debian packages (*amd-gpu-kernel-debug-sdk_0.6.856_amd64.deb* and *amd-hsail-gdb_0.6.856_amd64.deb*)
    * `sudo dpkg -i amd-gpu-kernel-debug-sdk_0.6.856_amd64.deb amd-hsail-gdb_0.6.856_amd64.deb`
	  * The installed files will be placed in */opt/amd/hsa-debugger* folder.
2. Ensure the environment variable *LD_LIBRARY_PATH* contains the directory of the HSA Runtime library (the default is */opt/hsa/lib*).  You can add the following line into *.bashrc* file
  * `export LD_LIBRARY_PATH=/opt/hsa/lib:${LD_LIBRARY_PATH}`
3. Verify the setup
  * Copy the samples to a writeable folder
    * `cp -r /opt/amd/hsa-debugger/AMDGPUKernelDebugSDK/samples ~/`
  * `cd ~/samples/MatrixMultiplication`
  * `make`
    * The *Makefile* assumes that the hsa header files are located at */opt/hsa/include*.  If you encounter a compilation failure, please update the *HSADIR* within the *Makefile* to the directory of the hsa header files in the system.
  * `/opt/amd/hsa-debugger/AMDHsailGdb/bin/x86_64/hsail-gdb MatrixMul`
    * Tips: include the directory that contains the hsail-gdb tool in your *PATH* environment variable
  
<A NAME="Known">
## Known Issues for September 2015 Beta Release
* Incorrect debugging results with Kalmar Stencil2D and TileStaticStorageBandwitdh samples due to improperly generated debug info in these applications.
* Debugging HSA applications that do not properly clean up queue resources (by destroying queues) and shut down the hsa runtime may cause segmentation fault or hang in hsail-gdb.  These issues appear with existing SNACK sample applications. 
* HSAIL programs (i.e. hsa code objects) that contain more than one BRIG module are not supported
* HSAIL kernels that contain global (or read only) variables are not supported
* HSAIL kernels that contain hsail function calls are not supported
* HSAIL backtrace is not supported
* HSAIL function breakpoints must also be set to stop at HSAIL source breakpoints.
