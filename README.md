# HSA Debugger May 2015 Alpha Release (version 0.5)

## Overview
The HSA Debugger provides a gdb-based debugging environment for debugging host application and HSAIL kernels running on AMD HSA platforms.  The kernel programming language currently supported is HSAIL 1.0.  There are two packages included in this Alpha Release:
* AMD HSAIL gdb package that contains the hsail-gdb tool 
  * based on GDB 7.8, the GNU source-level debugger
* AMD GPU Kernel Debug SDK package that contains the necessary header, library and sample files

### Major Features
* Seamless host application and HSAIL kernel source debugging using a familiar gdb-based debugging environment on AMD HSA platforms
* Set HSAIL kernel breakpoints, single stepping and inspect HSAIL registers within HSAIL kernel source
* View active GPU states (active work-groups, work-items and wavefronts information)

## System Requirements
* Hardware: AMD Kaveri APUs that implement HSA (i.e. A10-7850K, A10-7800, etc.)
  * Refer to the [HSA Platforms & Installation wiki page](https://github.com/HSAFoundation/HSA-Docs-AMD/wiki/HSA-Platforms-&-Installation) for additional information.
* OS: 64-bit Ubuntu 14.04 (recommended) and Fedora 21
* HSA Runtime: [AMD HSA Runtime May 2015 release](https://github.com/HSAFoundation/HSA-Runtime-AMD)
* HSA Driver: [AMD HSA Drivers v1.4](https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD)

HSAIL applications must use the latest [LibHSAIL](https://github.com/HSAFoundation/HSAIL-Tools) built with BUILD_WITH_LIBBRIGDWARF=1 to assemble HSAIL text to BRIG.

## Package Contents
The directory structure of the HSA Debugger packages:
* AMDHsailGdb-v0.5
  * bin/x86_64
    * hsail-gdb, amd-gdb, .gdbinit
  * LICENSE.txt
* AMDGPUKernelDebugSDK-v0.5
  * include
    * AMDGPUDebug.h, amd_hsa_common_h, amd_hsa_kernel_code.h, amd_hsa_tools_interfaces.h
  * lib/x86_64
    * libAMDGPUDebugHSA-x64.so, libAMDHSADebugAgent-x64.so, libAMDHwDbgFacilities-x64.so, libhsa-runtime-tools64.so.1
  * samples
    * Common
	  * HSAResourceManager.h
	  * HSAResourceManager.cpp
	* MatrixMultiplication
	  * Makefile, MatrixMul.cpp, matrixMul_kernel.brig, matrixMul_kernel.hsail
  * LICENSE.txt

If you download the HSA Debugger packages or files separately, you must create the same directory structure as shown above in order to run hsail-gdb successfully.
  
## Installation
First, make sure that the HSA system is setup correctly
* [Install HSA Driver](https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD#installing-and-configuring-the-kernel)
* [Install HSA Runtime](https://github.com/HSAFoundation/HSA-Runtime-AMD/#installing-and-configuring-the-hsa-runtime)
* [Verity the setup by running HSAIL vector_copy sample successfully](https://github.com/HSAFoundation/HSA-Runtime-AMD#running-the-sample---vector_copy)
  * Note that the vector_copy sample can't be debugged with hsail-gdb since the pre-generated BRIG file was not created with debugging support.
    As part of the HSAIL debugger package, there is a sample HSAIL MatrixMultiplication that can be used with hsail-gdb.
  
###HSA Debugger Installation
1. Download the HSA-Debugger-AMD from the repository
  * git clone https://github.com/HSAFoundation/HSA-Debugger-AMD.git
2. Ensure the environment variable LD_LIBRARY_PATH contains the directory of the HSA Runtime library (the default is /opt/hsa/lib).  You can add the following line into .bashrc file
  * export LD_LIBRARY_PATH=/opt/hsa/lib:${LD_LIBRARY_PATH}
3. Verify the setup
  * cd AMDGPUKernelDebugSDK-v0.5/samples/MatrixMultiplication
  * make
    * The Makefile assumes that the hsa header files are located at /opt/hsa/include.  If you encounter a compilation failure, please update the HSADIR within the Makefile to the directory of the hsa header files in the system.
  * ../../../AMDHsailGdb-v0.5/bin/x86_64/hsail-gdb MatrixMul
    * Tips: include the directory that contains the hsail-gdb tool in your $PATH environment variable
  
## Usage Examples

### How do I start my HSAIL program?
You can start your program in hsail-gdb just like you would any program under gdb
* hsail-gdb MatrixMul
* You should now be in the gdb prompt and can start execution of the program
* *(gdb)* start

### How do I set breakpoints in my HSAIL program?
To set breakpoints in HSAIL kernels, hsail-gdb defines
* **HSAIL kernel function breakpoint:** Similar to a gdb function breakpoint, allows you stop the application just before a **specific** HSAIL dispatch starts
* **Generic HSAIL kernel breakpoint:** Stop the application before **any** HSAIL dispatch starts
* **Source line breakpoint:** A breakpoint that is set on a particular line of HSAIL source

#### Setting HSAIL Function Breakpoints
The gdb break command has been extended to *break hsail* in order to set HSAIL breakpoints.
To set a specific HSAIL kernel function breakpoints: 
* break hsail:$kernel_name

For matrix multiplication, you can specify the kernel name
* *(gdb)* break hsail:&__OpenCL_matrixMul_kernel

This will stop the application's execution just before the HSAIL kernel (in this case, the matrix multiplication kernel) begins executing on the device.

To set a general HSAIL kernel function breakpoint, use either of the following command: 
* *(gdb)* break hsail
* *(gdb)* break hsail:*

This will stop the application just before every dispatch begins executing on the device.

#### Setting HSAIL Kernel Source Breakpoints
In order to break into HSAIL kernels, you need to set HSAIL source breakpoints. Hsail-gdb saves the kernel source for the present dispatch to a temporary file called *temp_source*. HSAIL source breakpoints can be set by specifying the line number from the HSAIL source file. The *temp_source* file is overwritten by hsail-gdb on every dispatch.

Once you hit a kernel function breakpoint, you can view the *temp_source* file and choose a line number. You can set the source breakpoint using the syntax
* break hsail:line_number

For example, this will set a breakpoint at line 150 in the *temp_source*

```
(gdb) b hsail:150
HSAIL breakpoint 9 (PC:0x06d8 add_u32 $s3, $s3, 1; temp_source@line 150)
```

When you continue the program's execution, the application will stop when any work-item reaches line 150 in *temp_source*.

#### Managing HSAIL Breakpoints
* You can use the same gdb commands such as *info bre* to view information about the active HSAIL and host breakpoints
The command *info bre* shows multiple HSAIL kernel source breakpoints, an HSAIL function breakpoint and a host breakpoint

```
(gdb) info bre
Num     Type             Disp Enb Address            What
1       HSAIL breakpoint keep y   ---                Every HSAIL dispatch(*)
breakpoint already hit 2 times
4       HSAIL breakpoint keep y   PC:0x06d8          add_u32 $s3, $s3, 1; temp_source@line 150
breakpoint already hit 320 times
5       HSAIL breakpoint keep y   ---                &__Gdt_vectoradd_kernel
6       breakpoint       keep y   0x0000000000407105 in RunTest() at MultiKernelDispatch.cpp:100
```

* You can also delete HSAIL breakpoints using the same command as GDB's host breakpoints *del breakpoint_number*

### How do I single step in  a HSAIL kernel?
You can single step in a HSAIL dispatch using the conventional step command. 
Only a single step is supported at a time. 

The following shows how hsail-gdb steps 4 source lines after hitting a kernel source breakpoint

```
(gdb) b hsail:50
HSAIL breakpoint 4 (PC:0x022c ld_kernarg_align(8)_width(all)_u64  $d1, [%inputA]; temp_source@line 50)
(gdb) c
Continuing.
[New Thread 0x7ffeeddcb700 (LWP 13642)]
Waiting for completion...
[hsail-gdb]: Breakpoint: 4	at ld_kernarg_align(8)_width(all)_u64  $d1, [%inputA];
Stopped on HSAIL breakpoint
[Switching to Thread 0x7ffeeddcb700 (LWP 13642)]

(gdb) step
[hsail-gdb]: Breakpoint: 	at line 52
Stopped on HSAIL breakpoint

(gdb) s
[hsail-gdb]: Breakpoint: 	at line 53
Stopped on HSAIL breakpoint

(gdb) s
[hsail-gdb]: Breakpoint: 	at line 54
Stopped on HSAIL breakpoint

(gdb) c
Continuing.
```


### How do I print HSAIL registers?
To print HSAIL registers in a HSAIL kernel, the gdb *print* command has been extended. To print HSAIL registers.
* print hsail:$register_name

This will print the value $register_name for a single work-item. For example, printing HSAIL register $s0 will provide the value of register $s0

```
(gdb) print hsail:$s0
$4 = 0
```

To view the data of a different work-item, you need switch focus between different work-items. The *hsail thread* command allows you to set the focus on a different work-item by specifying its work-item and work-group ID. It should be noted that you cannot switch focus to work-items not scheduled on the device. 

Switching the focus to another work-item and printing $s0 allows us to view data for the other work-item.

```
(gdb) hsail thread wg:0,0,0,wi:1,0,0
[hsail-gdb: Switching to work-group (0,0,0) and work-item (1,0,0)]

(gdb) print hsail:$s0
$3 = 1
```

### How do I view HSAIL dispatch info?
The *info* command has been extended to *info hsail*.
The *info hsail* command allows you to view the present state of the HSAIL dispatch and also allows you to view information about the HSAIL dispatches that have executed over the lifetime of the application.
* *(gdb)* info hsail

This will print all the possible options for *info hsail*. The *info hsail* command allows you to view information about the active dispatch, active work-groups and active work-items on the device.
The possible inputs to *info hsail* are below

```
info hsail [kernels]: print all HSAIL kernel dispatches
info hsail [kernel <kernel name>]: print all HSAIL kernel dispatches with a specific kernel name
info hsail [work-groups | wgs]: print all HSAIL work-group items
info hsail [work-group <flattened id> | wg <flattened id> | work-group <x,y,z> | wg <x,y,z>]: print a specific HSAIL work-group item
info hsail [work-item | wi]: print the focus HSAIL work-item
info hsail [work-item <x,y,z> | wi <x,y,z>]: print a specific HSAIL work-item
```

For example, *info hsail kernels* on an application that dispatches two kernels shows
```
(gdb) info hsail kernels
Kernels info
Index                    KernelName  DispatchCount  # of Work-groups  Work-group Dimensions
    0       &__Gdt_vectoradd_kernel              1             1,1,1                 64,1,1
   *1    &__OpenCL_matrixMul_kernel              1             8,5,1                16,16,1

```

The *info hsail work-groups* command will show the active work-groups for the active dispatch

```
(gdb) info hsail work-groups
Index            Work-group ID   Flattened Work-group ID
   *0                    0,0,0                         0
    1                    1,0,0                         1
    2                    2,0,0                         2
```
The *info hsail work-item* command will show the focus work-item for the active dispatch

```
(gdb) info hsail wi
Information for Work-item
Index   Wavefront ID             Work-item ID    Absolute Work-item ID          PC            Source line
   *0     0x408002e0                  0, 0, 0                  0, 0, 0       0x6d8   temp_source@line 150
```

### Others
A useful tutorial on how to use GDB can be found on [RMS's site](http://www.unknownroad.com/rtfm/gdbtut/).

## Known Issues for Alpha Release
* HSAIL programs that contain more than one BRIG module are not supported
* HSAIL kernels that contain global (or read only) variables are not supported
* HSAIL kernels that contain function calls are not supported
* HSAIL backtrace is not supported
* Terminating hsail-gdb while in the middle of an HSAIL kernel's execution will hang the entire system
 
