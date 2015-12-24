## Table of Contents
* [How do I start my HSAIL program?](#HowDoIStart)
* [How do I set breakpoints in my HSAIL program?](#HowDoISetBreakpoints)
  * [Setting HSAIL Function Breakpoints](#SetHsailFunctionBreakpoints)
  * [Setting HSAIL Kernel Source Breakpoints](#SetHsailKernelSourceBreakpoints)
  * [Setting Conditional HSAIL Kernel Source Breakpoints](#SetConditionalHsailKernelSourceBreakpoints)
  * [Managing HSAIL Breakpoints](#ManagingHsailBreakpoints)
* [How do I single step in  a HSAIL kernel?](#HowDoISingleStep)
* [How do I print HSAIL registers?](#HowDoIPrintHsailRegisters)
* [How do I view HSAIL dispatch info?](#HowDoIViewHsailDispatchInfo)
* [How do I debug SNACK Applications?](#HowDoIDebugSNACKApplications)
* [Generating Logs for Reporting Issues in hsail-gdb](#GeneratingLogs)
* [Others](#Others)

<A NAME="Usage">
<A NAME="HowDoIStart">
### How do I start my HSAIL program?
You can start your program in hsail-gdb just like you would any program under gdb
* `hsail-gdb MatrixMul`
* You should now be in the gdb prompt and can start execution of the program
* `(gdb) start`

<A NAME="HowDoISetBreakpoints">
### How do I set breakpoints in my HSAIL program?
To set breakpoints in HSAIL kernels, hsail-gdb defines
* **HSAIL kernel function breakpoint:** Similar to a gdb function breakpoint, allows you stop the application just before a **specific** HSAIL dispatch starts
* **Generic HSAIL kernel breakpoint:** Stop the application before **any** HSAIL dispatch starts
* **Source line breakpoint:** A breakpoint that is set on a particular line of HSAIL source

<A NAME="SetHsailFunctionBreakpoints">
#### Setting HSAIL Function Breakpoints
The gdb `break` command has been extended to `break hsail` in order to set HSAIL breakpoints.
To set a specific HSAIL kernel function breakpoints: 
* `break hsail:kernel_name`

For matrix multiplication, you can specify the kernel name
* `(gdb) break hsail:&__OpenCL_matrixMul_kernel`

This will stop the application's execution just before the HSAIL kernel (in this case, the matrix multiplication kernel) begins executing on the device.

To set a general HSAIL kernel function breakpoint, use either of the following command: 
* `(gdb) break hsail`
* `(gdb) break hsail:*`

This will stop the application just before every dispatch begins executing on the device.

<A NAME="SetHsailKernelSourceBreakpoints">
#### Setting HSAIL Kernel Source Breakpoints
In order to break into HSAIL kernels, you need to set HSAIL source breakpoints. Hsail-gdb saves the kernel source for the present dispatch to a temporary file called *temp_source*. HSAIL source breakpoints can be set by specifying the line number from the *temp_source* HSAIL source file. The *temp_source* file is overwritten by hsail-gdb on every dispatch.

Once you hit a kernel function breakpoint, you can view the *temp_source* file and choose a line number. You can set the source breakpoint using the syntax
* `break hsail:line_number`

For example, this will set a breakpoint at line 150 in the *temp_source*

```
(gdb) b hsail:150
HSAIL breakpoint 1 (PC:0x08d0 mad_u32 $s0, $s1, $s0, $s3; temp_source@line 150)
```

When you continue the program's execution, the application will stop when any work-item reaches line 150 in *temp_source*.

<A NAME="SetConditionalHsailKernelSourceBreakpoints">
#### Setting Conditional HSAIL Kernel Source Breakpoints
Conditional HSAIL breakpoints allow you to stop the application only when a particular workitem hits a breakpoint. You can set a conditional source breakpoint by specifying the a work-item using the syntax:
* `break hsail:line_number if wg:x,y,z wi:x,y,z`
For example, this will set a breakpoint at line 150 and only stop the application if the work-item in workgroup 2,0,0 and local work-item 1,0,0
```
(gdb) b hsail:150 if wg:2,0,0 wi:16,0,0
HSAIL breakpoint 1 (PC:0x08d0 mad_u32 $s0, $s1, $s0, $s3; temp_source@line 150)
```
When the application is executed, the dispatch will stop when line 150 is executed for the above workitem as shown below:

```
[hsail-gdb]: Switching to work-group (2,0,0) and work-item (1,0,0)
[hsail-gdb]: Condition: active work-group: 2, 0, 0 @ work-item: 1, 0, 0
[hsail-gdb]: Breakpoint 2 at mad_u32 $s0, $s1, $s0, $s3; temp_source@line 150
Stopped on HSAIL breakpoint
```
<A NAME="ManagingHsailBreakpoints">
#### Managing HSAIL Breakpoints
* You can use the same gdb commands such as `info bre` to view information about the active HSAIL and host breakpoints
The command `info bre` shows multiple HSAIL kernel source breakpoints, an HSAIL function breakpoint and a host breakpoint

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

* You can also delete HSAIL breakpoints using the same command as GDB's host breakpoints `del breakpoint_number`

<A NAME="HowDoISingleStep">
### How do I single step in  a HSAIL kernel?
You can single step in a HSAIL dispatch using the conventional `step` command. 
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

<A NAME="HowDoIPrintHsailRegisters">
### How do I print HSAIL registers?
To print HSAIL registers in a HSAIL kernel, the gdb `print` command has been extended. To print HSAIL registers.
* `print hsail:$register_name`

This will print the value *$register_name* for a single work-item. For example, printing HSAIL register *$s0* will provide the value of register *$s0*

```
(gdb) print hsail:$s0
$4 = 0
```

To view the data of a different work-item, you need switch focus between different work-items. The `hsail thread` command allows you to set the focus on a different work-item by specifying its work-item and work-group ID. It should be noted that you cannot switch focus to work-items not scheduled on the device. 

Switching the focus to another work-item and printing *$s0* allows us to view data for the other work-item.

```
(gdb) hsail thread wg:0,0,0 wi:1,0,0
[hsail-gdb]: Switching to work-group (0,0,0) and work-item (1,0,0)

(gdb) print hsail:$s0
$3 = 1
```

<A NAME="HowDoIViewHsailDispatchInfo">
### How do I view HSAIL dispatch info?
The `info` command has been extended to `info hsail`.
The `info hsail` command allows you to view the present state of the HSAIL dispatch and also allows you to view information about the HSAIL dispatches that have executed over the lifetime of the application.
* `(gdb) info hsail`

This will print all the possible options for `info hsail`. The `info hsail` command allows you to view information about the active dispatch, active work-groups and active work-items on the device.
The possible inputs to `info hsail` are below

```
info hsail [kernels]: print all HSAIL kernel dispatches
info hsail [kernel <kernel name>]: print all HSAIL kernel dispatches with a specific kernel name
info hsail [work-groups | wgs]: print all HSAIL work-group items
info hsail [work-group <flattened id> | wg <flattened id> | work-group <x,y,z> | wg <x,y,z>]: print a specific HSAIL work-group item
info hsail [work-item | wi]: print the focus HSAIL work-item
info hsail [work-item <x,y,z> | wi <x,y,z>]: print a specific HSAIL work-item
```

For example, `info hsail kernels` on an application that dispatches two kernels shows
```
(gdb) info hsail kernels
Kernels info
Index                    KernelName  DispatchCount  # of Work-groups  Work-group Dimensions
    0       &__Gdt_vectoradd_kernel              1             1,1,1                 64,1,1
   *1    &__OpenCL_matrixMul_kernel              1             8,5,1                16,16,1

```

The `info hsail work-groups` command will show the active work-groups for the active dispatch

```
(gdb) info hsail work-groups
Index            Work-group ID   Flattened Work-group ID
   *0                    0,0,0                         0
    1                    1,0,0                         1
    2                    2,0,0                         2
```
The `info hsail work-item` command will show the focus work-item for the active dispatch

```
(gdb) info hsail wi
Information for Work-item
Index   Wavefront ID             Work-item ID    Absolute Work-item ID          PC            Source line
   *0     0x408002e0                  0, 0, 0                  0, 0, 0       0x6d8   temp_source@line 150
```

<A NAME="HowDoIDebugSNACKApplications">
### How do I Debug SNACK Applications?
To debug SNACK applications you need to let CLOC know to compile the HSAIL kernels for debug and embed the HSAIL kernel source in the binary. 

This can be done adding the environment variable below before calling the buildrun.sh script for the SNACK applications.
```
export LIBHSAIL_OPTIONS_APPEND="-g -include-source"
```
You can then debug the SNACK application just like the other HSAIL applications described above.

<A NAME="GeneratingLogs">
### Generating Logs for Reporting Issues in hsail-gdb
Additional log files can be generated by hsail-gdb. These log files should be sent to the hsail-gdb developers to allow them to diagnose issues.
Logging is enabled with the `HSAIL_GDB_ENABLE_LOG` environment variable as shown below

```
export HSAIL_GDB_ENABLE_LOG='DebugLogs'
hsail-gdb MatrixMul
```

The environment variable enables logging and provides a prefix for the log file names.
As the `MatrixMul` application executes, log files with the prefix `DebugLogs_` will be generated. 
The log files generated include logs from GDB, the HSA debug Agent and the HSA code objects used in the applications. Each debug session's log file's name will include a unique `SessionID`.

<A NAME="Others">
### Others
A useful tutorial on how to use GDB can be found on [RMS's site](http://www.unknownroad.com/rtfm/gdbtut/).
