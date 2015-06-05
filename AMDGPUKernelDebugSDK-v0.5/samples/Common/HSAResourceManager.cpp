//==============================================================================
// Copyright 2015 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation(if any)
// (collectively, the "Materials") pursuant to the terms and conditions of the
// Software License Agreement included with the Materials.If you do not have a
// copy of the Software License Agreement, contact your AMD representative for a
// copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER : THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE.THE ENTIRE RISK ASSOCIATED WITH THE USE OF
// THE SOFTWARE IS ASSUMED BY YOU.Some jurisdictions do not allow the exclusion
// of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION : AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD.  You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS : The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor.Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
/// \author Developer Tools
/// \file
/// \brief  HSA Runtime resource management class
//==============================================================================

/// \todo: Considering having an HSA application counter to keep track of how many applications are using HSA runtime.

#if defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>   // _putenv_s
#else
#include <cstdlib>    // setenv, unsetenv
#endif

#include <cstring>    // memset, memcpy
#include <string>
#include <stdarg.h>
#include <iostream>

#include "HSAResourceManager.h"

namespace DevTools
{
uint16_t HSAResourceManager::ms_hsaCount = 0;
hsa_agent_t HSAResourceManager::ms_gpu = { 0 };
bool HSAResourceManager::ms_hasRuntime = false;
bool HSAResourceManager::ms_profilingEnabled = false;
hsa_queue_t* HSAResourceManager::ms_pQueue = NULL;

// ------------------------------------- Public Functions -------------------------------------
HSAResourceManager::HSAResourceManager() :
    m_executable({0}),
    m_codeObj({0}),
    m_argsSize(0)
{
    memset(m_argsBuff, 0, sizeof(m_argsBuff));

    ms_hsaCount++;
}

HSAResourceManager::~HSAResourceManager()
{
    if (!CleanUp())
    {
        std::cerr << "Error in HSAResourceManager::~HSAResourceManager(): CleanUp() failed\n";
    }

    ms_hsaCount--;

    if (0 == ms_hsaCount)
    {
        if (!HSAResourceManager::ShutDown())
        {
            std::cerr << "Error in HSAResourceManager::~HSAResourceManager(): ShutDown() failed\n";
        }
    }

    //std::cout << "HSA resource management object remain: " << ms_hsaCount << "\n";

}

bool HSAResourceManager::InitRuntime()
{
    bool ret = true;

    if (!ms_hasRuntime)
    {
        hsa_status_t status = hsa_init();
        ret = HSA_CHECK_STATUS(status);

        if (true != ret)
        {
            std::cerr << "Error in HSAResourceManager::InitRuntime(): Initialing HSA Runtime failed, exiting...\n";
            status = hsa_shut_down();

            if (!HSA_CHECK_STATUS(status))
            {
                std::cerr << "Error in HSAResourceManager::InitRuntime(): Shutting down HSA Runtime failed.\n";
            }
        }

        // Just dump out information onto screen
        hsa_iterate_agents(QueryDevice, NULL);

        // Get GPU devices
        bool lret = true;
        status = hsa_iterate_agents(GetGPUDevice, &ms_gpu);

        if (!HSA_CHECK_STATUS(status) || 0 == ms_gpu.handle)
        {
            std::cerr << "Error in HSAResourceManager::InitRuntime(): Get GPU device failed.\n";
            lret = false;
        }

        ret &= lret;

        ms_hasRuntime = true;
    }

    return ret;
}

bool HSAResourceManager::PrintHsaVersion()
{
    uint16_t maj = 0;
    uint16_t min = 0;

    bool ret = GetHsaVersion(maj, min);

    if (ret)
    {
        std::cout << "HSA version: " << maj << "." << min << "\n";
    }
    else
    {
        std::cerr << "Error in HSAResourceManager::PrintHsaVersion(): GetHsaVersion() failed.\n";
    }

    return ret;
}

bool HSAResourceManager::GetHsaVersion(uint16_t& major, uint16_t& minor)
{
    bool ret = true;

    ret = HSAResourceManager::InitRuntime();
    if (!ret)
    {
        major = 0;
        minor = 0;
        std::cerr << "Error in HSAResourceManager::GetHsaVersion: Cannot init HSA runtime.\n";
        return ret;
    }

    hsa_status_t status = hsa_system_get_info(HSA_SYSTEM_INFO_VERSION_MAJOR, &major);

    if (!HSA_CHECK_STATUS(status))
    {
        ret = false;
        std::cerr << "Error in HSAResourceManager::GetHsaVersion(): Get HSA Major version number failed\n";
    }

    status = hsa_system_get_info(HSA_SYSTEM_INFO_VERSION_MINOR, &minor);

    if (!HSA_CHECK_STATUS(status))
    {
        ret = false;
        std::cerr << "Error in HSAResourceManager::GetHsaVersion(): Get HSA Minor version number failed\n";
    }

    // If there is no any other hsa resource manager
    if (0 == ms_hsaCount)
    {
        if (!HSAResourceManager::ShutDown())
        {
            ret = false;
            std::cerr << "Error in HSAResourceManager::GetHsaVersion(): Cannot shut down the HSA runtime which init by this function.\n";
        }
    }

    return ret;
}

bool HSAResourceManager::CreateDefaultQueue(bool enableKernelTimestamps)
{
    bool ret = true;

    if (!DestroyQueue())
    {
        ret = false;
        std::cerr << "Error in CreateDefaultQueue(): Destroying previous existing queue failed\n";
        return ret;
    }

    uint32_t queueSize = 0;
    hsa_status_t status = hsa_agent_get_info(ms_gpu, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queueSize);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::CreateDefaultQueue(): Get queue max size failed.\n";
        ret = false;
    }

    status = hsa_queue_create(ms_gpu,                 // HSA agent
                              queueSize,              // Number of packets the queue is expected to hold
                              HSA_QUEUE_TYPE_SINGLE,  // Type of the queue
                              NULL,                   // callback related to the queue. No specific requirement so it should be NULL.
                              NULL,                   // Data that is passed to callback. NULL because no callback here.
                              UINT32_MAX,             // Private segment size. Hint indicating the maximum expected usage per work item. No particular value required, so it should be UINT32_MAX
                              UINT32_MAX,             // Group segment size. Also no particular value required.
                              &ms_pQueue);            // The queue we want to create.

    if (!HSA_CHECK_STATUS(status) || NULL == ms_pQueue)
    {
        std::cerr << "Error in HSAResourceManager::CreateDefaultQueue(): Create queue failed.\n";
        ret = false;
    }

    ms_profilingEnabled = enableKernelTimestamps;

    if (enableKernelTimestamps)
    {
        status = hsa_amd_profiling_set_profiler_enabled(ms_pQueue, 1);

        if (!HSA_CHECK_STATUS(status) || NULL == ms_pQueue)
        {
            std::cerr << "Error in HSAResourceManager::CreateDefaultQueue(): hsa_amd_profiling_set_profiler_enabled() failed.\n";
            ret = false;
        }
    }

    return ret;
}

// Simple function to trim head and tail space of a string.
static void TrimHeadAndTailSpace(std::string& s)
{
    if (s.size() == 0)
    {
        return;
    }

    std::size_t nsi = s.find_first_not_of(' ');
    s.erase(0, nsi);

    // if there are all spaces in the string
    if (s.size() == 0)
    {
        return;
    }

    nsi = s.find_last_not_of(' ');
    s.erase(nsi+1);

    return;
}

bool HSAResourceManager::CreateAQLPacketFromBrig(
    const void*                   pBRIG,
    const std::string&            kernelSymbol,
    const bool                    bCreateSignal,
    hsa_kernel_dispatch_packet_t& aqlPacketOut,
    const std::string&            finalizerFlags)
{

    if (NULL == pBRIG)
    {
        std::cerr << "Error in HSAResourceManager::Finalize(): pBrig cannot be NULL.\n";
        return false;
    }

    uint64_t codeHandle = 0;
    {
        // Simple string processing, current finalizer v3 is weak for option parameter parsing.
        std::string fFlags = finalizerFlags;
        TrimHeadAndTailSpace(fFlags);

        if (!Finalize(pBRIG, kernelSymbol, codeHandle, fFlags) || 0 == codeHandle)
        {
            std::cerr << "Error in HSAResourceManager::CreateAQLPacketFromBrig(): Finalize() failed\n";
            return false;
        }
    }

    bool ret = CreateKernelDispatchPacket(codeHandle, aqlPacketOut);

    if (!ret)
    {
        std::cerr << "Error in HSAResourceManager::CreateAQLPacketFromBrig(): Compile and/or finalization failed.\n";
        return ret;
    }

    if (bCreateSignal)
    {
        ret = CreateSignal(aqlPacketOut.completion_signal);

        if (!ret)
        {
            std:: cerr << "Error in HSAResourceManager::CreateAQLPacketFromBrig(): Create signal failed.\n";
            return ret;
        }
    }

    return ret;
}

bool HSAResourceManager::CreateAQLFromExecutable(
            const std::string&            kernelSymbol,
            const bool                    bCreateSignal,
            hsa_kernel_dispatch_packet_t& aql)
{
    if (0 == m_executable.handle)
    {
        std::cerr << "Error in HSAResourceManager::CreateAQLFromExecutable(): No executable, please call CreateAQLFromBrig first instead.\n";
        return false;
    }
    bool ret = CreateKernelDispatchPacket(0, aql);

    if (!ret)
    {
        std::cerr << "Error in HSAResourceManager::CreateAQLFromExecutable(): CreateKernelDispatchPacket() failed.\n";
        return false;
    }

    // Get symbol handle
    hsa_executable_symbol_t symbolOffset;
    hsa_status_t status = hsa_executable_get_symbol(m_executable, NULL, kernelSymbol.c_str(), ms_gpu, 0, &symbolOffset);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::CreateAQLFromExecutable(): hsa_executable_get_symbol failed.\n";
        return false;
    }

    // Get code object handle
    status = hsa_executable_symbol_get_info(symbolOffset, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &aql.kernel_object);

    if (!HSA_CHECK_STATUS(status) || aql.kernel_object == 0)
    {
        std::cerr << "Error in HSAResourceManager::CreateAQLFromExecutable(): hsa_executable_symbol_get_info failed.\n";
        return false;
    }

    if (bCreateSignal)
    {
        ret = CreateSignal(aql.completion_signal);

        if (!ret)
        {
            std:: cerr << "Error in HSAResourceManager::CreateAQLFromExecutable(): Create signal failed.\n";
            return false;
        }
    }

    return true;
}

bool HSAResourceManager::Finalize(const void*       pBRIG,
                        const std::string&          kernelSymbol,
                        uint64_t&                   codeHandle,
                        const std::string&          compileFlags)
{
    if (NULL == pBRIG)
    {
        std::cerr << "Error in HSAResourceManager::Finalize(): pBrig cannot be NULL.\n";
        return false;
    }
    hsa_status_t status;
    if (0 != m_codeObj.handle)
    {
        status = hsa_code_object_destroy(m_codeObj);

        if (!HSA_CHECK_STATUS(status))
        {
            std::cerr << "Error in HSAResourceManager::Finalize(): Fail to destroy previous codeObj.\n";
            return false;
        }
        m_codeObj.handle = 0;
    }

    if (0 != m_executable.handle)
    {
        status = hsa_executable_destroy(m_executable);
        if (!HSA_CHECK_STATUS(status))
        {
            std::cerr << "Failed to destroy previous executable\n";
            return false;
        }
        m_executable.handle = 0;
    }

    // Create HSA program  ----------------------------
    hsa_ext_program_t program = {0};
#if defined(_WIN64) || defined(_LP64)
    status = hsa_ext_program_create(
                              HSA_MACHINE_MODEL_LARGE,
                              HSA_PROFILE_FULL,
                              HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO,
                              "-g",
                              &program
                          );
#else
    status = hsa_ext_program_create(
                              HSA_MACHINE_MODEL_SMALL,
                              HSA_PROFILE_FULL,
                              HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO,
                              "-g",
                              &program
                          );
#endif

    if (!HSA_CHECK_STATUS(status) || 0 == program.handle)
    {
        std::cerr << "Error in HSAResourceManager::Finalize(): Create HSA program failed.\n";
        return false;
    }

    // Add BRIG module to HSA program  ----------------------------
    hsa_ext_module_t programModule = reinterpret_cast<hsa_ext_module_t>(const_cast<void*>(pBRIG));
    status = hsa_ext_program_add_module(program, programModule);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::Finalize(): Adding BRIG module failed.\n";
        return false;
    }

    // Finalize hsail program --------------------------------------------------------
    hsa_isa_t isa;
    status = hsa_agent_get_info(ms_gpu, HSA_AGENT_INFO_ISA, &isa);
    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::Finalize(): Fail to get ISA.\n";
        return false;
    }

    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));

    status = hsa_ext_program_finalize(
                 program,
                 isa,
                 0,
                 control_directives,
                 compileFlags.c_str(),
                 HSA_CODE_OBJECT_TYPE_PROGRAM,
                 &m_codeObj
             );

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Failed to finalize program.\n";
        return false;
    }

    status = hsa_ext_program_destroy(program);
    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::Finalize(): Failed to destroy program.\n";
    }
    program.handle = 0;

    // Create executable
    status = hsa_executable_create(
                 HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", &m_executable);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Failed to create hsa executable.\n";
        return false;
    }

    // Load code object.
    status = hsa_executable_load_code_object(m_executable, ms_gpu, m_codeObj, NULL);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Failed to load code object.\n";
        return false;
    }

    // Freeze executable
    status = hsa_executable_freeze(m_executable, NULL);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Failed to freeze executable.\n";
        return false;
    }

    // Get symbol handle
    hsa_executable_symbol_t symbolOffset;
    status = hsa_executable_get_symbol(m_executable, NULL, kernelSymbol.c_str(), ms_gpu, 0, &symbolOffset);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "hsa_executable_get_symbol failed.\n";
        return false;
    }

    // Get code handle
    status = hsa_executable_symbol_get_info(
                 symbolOffset, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &codeHandle);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "hsa_executable_symbol_get_info failed.\n";
        return false;
    }

    return true;
}

bool HSAResourceManager::CreateKernelDispatchPacket(const uint64_t& codeHandle, hsa_kernel_dispatch_packet_t& aqlPacketOut)
{
    bool ret = true;
    memset(&aqlPacketOut, 0, sizeof(hsa_kernel_dispatch_packet_t));

    aqlPacketOut.header |= HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE;

    // using FENCE_SCOPE_SYSTEM base on example on HSA runtime document, version 1_00_20150130.
    aqlPacketOut.header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
    aqlPacketOut.header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
    aqlPacketOut.setup |= 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;

    aqlPacketOut.workgroup_size_x = 1;
    aqlPacketOut.workgroup_size_y = 1;
    aqlPacketOut.workgroup_size_z = 1;
    aqlPacketOut.grid_size_x = 1;
    aqlPacketOut.grid_size_y = 1;
    aqlPacketOut.grid_size_z = 1;

    // Bind kernel code
    if (0 != codeHandle)
    {
        aqlPacketOut.kernel_object = codeHandle;
    }

    return ret;
}

bool HSAResourceManager::CopyKernelDispatchPacket(
    const hsa_kernel_dispatch_packet_t& aqlPacket,
    const bool                          bCopySignal,
    hsa_kernel_dispatch_packet_t& aqlPacketOut) const
{
    if (NULL == memcpy(&aqlPacketOut, &aqlPacket, sizeof(hsa_kernel_dispatch_packet_t)))
    {
        std::cerr << "Error in HSAResourceManager::CopyKernelDispatchPacket(): memcpy() fail.\n";
        return false;
    }

    if (!bCopySignal)
    {
        aqlPacketOut.completion_signal.handle = 0;
    }

    return true;
}

bool HSAResourceManager::AppendKernelArgs(const void* pAddr, const std::size_t size, const std::size_t offsetSize)
{
    if (m_argsSize == 0)
    {
        // The first time call this function, need to add offset arguments first.
        m_argsSize += offsetSize;
        memset(m_argsBuff, 0x0, m_argsSize);
    }

    if (NULL == pAddr)
    {
        std::cerr << "Error in AppendKernelArgs(): Address of input arguments is NULL.\n";
        return false;
    }

    memcpy(m_argsBuff + m_argsSize, pAddr, size);
    m_argsSize += size;

    return true;
}

bool HSAResourceManager::RegisterKernelArgsBuffer(hsa_kernel_dispatch_packet_t& aql)
{
    hsa_status_t status = hsa_memory_register(m_argsBuff, m_argsSize);
    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::SetupKernelArgs(): Register args buffer memory failed.\n";
        aql.kernarg_address = NULL;
        return false;
    }

    aql.kernarg_address = m_argsBuff;
    return true;
}

bool HSAResourceManager::DeregisterKernelArgsBuffer()
{
    if (0 == m_argsSize)
    {
        // Nothing need to be done, return.
        return true;
    }

    bool ret = true;
    hsa_status_t status = hsa_memory_deregister(m_argsBuff, m_argsSize);

    if (!HSA_CHECK_STATUS(status))
    {
        ret  = false;
        std::cerr << "Error in HSAResourceManager::DeregisterKernelBuffer(): deregister kernel args buffer failed.\n";
    }
    else
    {
        memset(m_argsBuff, 0, gs_MAX_ARGS_BUFF_SIZE);
        m_argsSize = 0;
    }

    return ret;
}

bool HSAResourceManager::Dispatch(hsa_kernel_dispatch_packet_t& aql)
{
    if (NULL == ms_pQueue)
    {
        std::cerr << "No queue!\n";
        return false;
    }

    // Verify if we have register the kernel args buffer.
    // Assumming we have only one kernel in the application.
    if (m_argsSize != 0 && aql.kernarg_address == NULL)
    {
        this->RegisterKernelArgsBuffer(aql);
    }

    const uint32_t& queueSize = ms_pQueue->size;
    const uint32_t queueMask = queueSize - 1;

    // Write to queue
    uint64_t index = hsa_queue_load_write_index_relaxed(ms_pQueue);
    ((hsa_kernel_dispatch_packet_t*)(ms_pQueue->base_address))[index & queueMask] = aql;
    hsa_queue_store_write_index_relaxed(ms_pQueue, index + 1);

    // Ring doorbell.
    hsa_signal_store_release(ms_pQueue->doorbell_signal, static_cast<hsa_signal_value_t>(index));

    return true;
}

bool HSAResourceManager::WaitForCompletion(hsa_signal_t& completionSignal, uint64_t timeout, bool outputTimingData)
{
    bool ret = true;

    if (0 != hsa_signal_wait_acquire(completionSignal,         // signal
                                     HSA_SIGNAL_CONDITION_EQ,  // condition
                                     0,                        // compare_value
                                     timeout,                  // time_out_hint
                                     HSA_WAIT_STATE_ACTIVE)    // wait_state_hint
       )
    {
        std::cerr << "Error in HSAResourceManager::WaitForCompletion(): Signal wait return unexpected value\n";
        ret = false;
    }

    if (outputTimingData && ms_profilingEnabled)
    {
        hsa_amd_profiling_dispatch_time_t dispatch_times;
        dispatch_times.start = 0;
        dispatch_times.end = 0;
        hsa_status_t status = hsa_amd_profiling_get_dispatch_time(ms_gpu, completionSignal, &dispatch_times);

        if (!HSA_CHECK_STATUS(status))
        {
            std::cerr << "Error in HSAResourceManager::WaitForCompletion(): hsa_amd_profiling_get_dispatch_time() failed.\n";
        }

        std::cout << "Kernel dispatch executed in " << double((dispatch_times.end - dispatch_times.start) / 1e6) << " milliseconds.\n";
    }

    return ret;
}

bool HSAResourceManager::CreateSignal(hsa_signal_t& signalOut)
{
    bool ret = true;
    hsa_signal_t signal;
    // The initial value of the signal is 1.
    hsa_status_t status = hsa_signal_create(1, 0, NULL, &signal);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::CreateSignal(): hsa_signal_create failed\n";
        signal.handle = 0;
        ret = false;
    }
    else
    {
        m_signals.push_back(signal);
        signalOut = signal;
    }

    return ret;
}

bool HSAResourceManager::DestroySignal(hsa_signal_t& signal)
{
    bool ret = true;
    hsa_status_t status;

    for (unsigned int i = 0; i < m_signals.size(); ++i)
    {
        if (signal.handle == m_signals[i].handle)
        {
            m_signals[i] = m_signals[m_signals.size() - 1];
            m_signals.pop_back();
            break;
        }
    }

    status = hsa_signal_destroy(signal);

    if (!HSA_CHECK_STATUS(status))
    {
        std::cerr << "Error in HSAResourceManager::DestoySignal(): hsa_signal_destroy() failed\n";
        ret &= false;
    }

    return ret;
}

bool HSAResourceManager::HasRuntime()
{
    return ms_hasRuntime;
}

bool HSAResourceManager::ShutDown()
{
    bool ret = true;

    if (ms_hasRuntime)
    {
        if (!DestroyQueue())
        {
            ret = false;
            std::cerr << "Error in HSAResourceManager::ShutDown(): Destroying queue failed\n";
        }

        hsa_status_t status = hsa_shut_down();

        if (!HSA_CHECK_STATUS(status))
        {
            ret = false;
            std::cerr << "Error in HSAResourceManager::ShutDown():  Shutting down HSA runtime failed.\n";
        }

        ms_hasRuntime = false;
    }

    return ret;
}

bool HSAResourceManager::CleanUp()
{
    bool ret = true;
    hsa_status_t status;

    // Clean up signals
    for (unsigned int i = 0; i < m_signals.size(); ++i)
    {
        if (0 != m_signals[i].handle)
        {
            // TODO: Check the correct way to terminate a kernel.
            // If there is still kernel running, it should be terminated at this time.
            /**
                        hsa_signal_value_t sv = hsa_signal_wait_acquire(
                                            m_signals[i],
                                            HSA_EQ,                         // condition
                                            0,                              // compare_value
                                            0,                              // time_out_hint
                                            HSA_WAIT_ACTIVE);               // wait_state_hint
                        if (static_cast<hsa_signal_value_t>(0) != sv)
                        {
                            std::cout << "HSAResourceManager::CleanUp(): A kernel may not exit normally.\n";
                        }
            **/
            status = hsa_signal_destroy(m_signals[i]);
            bool lret = HSA_CHECK_STATUS(status);
            ret &= lret;

            if (true != lret)
            {
                std::cerr << "Error in HSAResourceManager::CleanUp(): Destroying signal " << i << " failed.\n";
            }

            m_signals[i].handle = 0;
        }
    }

    m_signals.clear();

    // Clean up executable and code object
    if (0 != m_executable.handle)
    {
        status = hsa_executable_destroy(m_executable);
        bool lret = HSA_CHECK_STATUS(status);
        ret &= lret;

        if (!lret)
        {
            std::cerr << "Error in HSAResourceManager::CleanUp():  hsa_executable_destroy() failed.\n";
        }

        m_executable.handle = 0;
    }

    if (0 != m_codeObj.handle)
    {
        status = hsa_code_object_destroy(m_codeObj);
        bool lret = HSA_CHECK_STATUS(status);
        ret &= lret;

        if (!lret)
        {
            std::cerr << "Error in HSAResourceManager::CleanUp(): hsa_code_object_destroy() failed.\n";
        }

        m_codeObj.handle = 0;
    }

    if (!DeregisterKernelArgsBuffer())
    {
        ret &= false;
        std::cerr << "Error in HSAResourceManager::CleanUp(): DeregisterKernelArgsBuffer() failed.\n";
    }

    return ret;
}

bool HSAResourceManager::DestroyQueue()
{
    bool ret = true;

    if (NULL != ms_pQueue)
    {
        hsa_status_t status = hsa_queue_destroy(ms_pQueue);
        ret = HSA_CHECK_STATUS(status);

        if (true != ret)
        {
            std::cerr << "Error in HSAResourceManager::DestroyQueue(): hsa_queue_destroy() falied.\n";
        }
        else
        {
            ms_pQueue = NULL;
        }
    }

    return ret;
}


// Accessors
const hsa_agent_t& HSAResourceManager::GPU()
{
    return ms_gpu;
}

hsa_queue_t* const& HSAResourceManager::Queue()
{
    return ms_pQueue;
}


/// \brief Output device type as string
///
/// \param[in] dt Device type
/// \return Device type as string
static std::string ConvertDeviceTypeToString(hsa_device_type_t dt)
{
    std::string ret;

    switch (dt)
    {
        case HSA_DEVICE_TYPE_CPU:
            ret = "CPU";
            break;

        case HSA_DEVICE_TYPE_GPU:
            ret = "GPU";
            break;

        case HSA_DEVICE_TYPE_DSP:
            ret = "DSP";
            break;

        default:
            ret = "Unknown";
            break;
    }

    return ret;
}

hsa_status_t QueryDevice(hsa_agent_t agent, void* pData)
{
    if (pData != NULL)
    {
        std::cout << "QueryDevice: Parameter is useless since this function is just a call back in order to fit in hsa_iterate_agents API.\n";
    }

    char deviceName[64];
    hsa_device_type_t deviceType;
    hsa_status_t err;

    std::cout << "HSA device attributes:\n";

    // Query the name of device
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, deviceName);

    if (!HSA_CHECK_STATUS(err))
    {
        std::cerr << "Error in QueryDevice(): Obtaining device name failed.\n";
    }

    std::cout << "\tname: " << deviceName << "\n";

    // Query type of device
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &deviceType);

    if (!HSA_CHECK_STATUS(err))
    {
        std::cerr << "Error in QueryDevice(): Obtaining device type failed.\n";
    }

    std::cout << "\ttype: " << ConvertDeviceTypeToString(deviceType) << "\n";

    return err;
}

hsa_status_t GetGPUDevice(hsa_agent_t agent, void* pData)
{
    hsa_status_t status = HSA_STATUS_SUCCESS;

    if (pData == NULL)
    {
        status = HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
    else
    {
        hsa_device_type_t deviceType;
        status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &deviceType);

        if (status == HSA_STATUS_SUCCESS)
        {
            if (deviceType == HSA_DEVICE_TYPE_GPU)
            {
                (*(hsa_agent_t*)pData) = agent;
            }
        }
        else
        {
            std::cerr << "Error in GetGPUDevice(): Obtaining device type failed.\n";
        }
    }

    return status;
}

bool HsaCheckStatus(hsa_status_t s)
{
    bool ret = true;

    if (HSA_STATUS_SUCCESS == s)
    {
        ret = true;
    }
    else
    {
        ret = false;
        std::cerr << "\nHSA status is not HSA_STATUS_SUCCESS.\n";
        std::cerr << HsaStatusStrings(s) << "\n";
    }

    return ret;
}

bool HsaCheckStatus(hsa_status_t s, const std::string& fileName, int lineNum)
{
    bool ret = true;

    if (HSA_STATUS_SUCCESS == s)
    {
        ret = true;
    }
    else
    {
        ret = false;
        std::cout << "In " << fileName << ", line " << lineNum << "\n";
        std::cerr << "HSA status is not HSA_STATUS_SUCCESS.\n";
        std::cerr << "Error code: " << s << ".\n";
        std::cerr << HsaStatusStrings(s) << ".\n";
    }

    return ret;
}

std::string HsaStatusStrings(hsa_status_t s)
{
    const char* pszbuff = NULL;
    hsa_status_string(s, &pszbuff);

    std::string str;
    if (pszbuff != NULL)
    {
        str = pszbuff;
    }
    else
    {
        str = "hsa_status_string return NULL string. Input HSA status code: " + std::to_string(static_cast<int>(s));
    }
    return str;
}

#if defined(_WIN32) || defined(_WIN64)
// setenv() and unsetenv() wrapper
static int setenv(const char* name, const char* value, int overwrite)
{
    int ret = 0;
    char* pBuffer = getenv(name);

    if (NULL == pBuffer || 1 == overwrite)
    {
        ret = _putenv_s(name, value);
    }

    return ret;
}

static int unsetenv(const char* name)
{
    int ret = 0;
    char* pBuffer = getenv(name);

    if (NULL != pBuffer)
    {
        ret = _putenv_s(name, "");
    }

    return ret;
}

#endif

bool SetSoftCPMode(bool bEnable)
{
    using namespace std;
    bool ret = true;
    const string emulateStr = "HSA_EMULATE_AQL";
    const string toolsLibStr = "HSA_TOOLS_LIB";

    if (bEnable)
    {
        string emulateVar = "1";

#ifdef _WIN64
        string toolsLibVar = "hsa-runtime-tools64.dll";
#elif defined(_WIN32)
        string toolsLibVar = "hsa-runtime-tools.dll";
#else   // Only Linux x64 platform is supported for now. 03/18/2015
        string toolsLibVar = "libhsa-runtime-tools64.so.1";
#endif

        if (0 != setenv(emulateStr.c_str(), emulateVar.c_str(), 1))
        {
            cerr << "Error in setting " << emulateStr << "\n";
            ret &= false;
        }

        if (0 != setenv(toolsLibStr.c_str(), toolsLibVar.c_str(), 1))
        {
            cerr << "Error in setting " << toolsLibStr << "\n";
            ret &= false;
        }
    }
    else
    {
        if (0 != unsetenv(emulateStr.c_str()))
        {
            cerr << "Error in unsetting " << emulateStr << "\n";
            ret &= false;
        }

        if (0 != unsetenv(toolsLibStr.c_str()))
        {
            cerr << "Error in unsetting " << toolsLibStr << "\n";
            ret &= false;
        }
    }

    // Print out the environment variable just set/unset.
    char* pBuffer = getenv(emulateStr.c_str());

    if (NULL != pBuffer)
    {
        cout << "Set " << emulateStr << " = " << pBuffer << "\n";
    }
    else
    {
        cout << emulateStr << " is unset.\n";
    }

    pBuffer = getenv(toolsLibStr.c_str());

    if (NULL != pBuffer)
    {
        cout << "Set " << toolsLibStr << " = " << pBuffer << "\n";
    }
    else
    {
        cout << toolsLibStr << " is unset.\n";
    }

    return ret;
}

} // namespace DevTools
