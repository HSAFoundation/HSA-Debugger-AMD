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

#ifndef _HSA_RESOURCE_MANAGER_H_
#define _HSA_RESOURCE_MANAGER_H_

#include <cstddef>
#include <cstdint>  //  UINT64_MAX
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push, 3)
#endif
#include <hsa.h>
#include <hsa_ext_amd.h>
#include <hsa_ext_finalize.h>
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif

/// \brief Check status macro that also report file name and line number in application debug build.
///
/// \param[in] x The status need to be check
/// \return true if x == HSA_STATUS_SUCCESS
#if defined (_DEBUG) || defined (DEBUG)
#define HSA_CHECK_STATUS( x ) DevTools::HsaCheckStatus(x, __FILE__, __LINE__)
#else
#define HSA_CHECK_STATUS( x ) DevTools::HsaCheckStatus(x)
#endif

namespace DevTools
{
static const std::size_t gs_MAX_ARGS_BUFF_SIZE = 256;

class HSAResourceManager
{
public:
    /// \brief Default constructor, initialize member variable
    HSAResourceManager();

    /// \brief Destructor, will call Destroy() to release resources.
    ~HSAResourceManager();

    /// \brief Call hsa_init() and setup a default GPU device
    /// \param[in] verbosePrint  set to true to print extra message outputs to console
    ///
    /// \return true if there is no error
    static bool InitRuntime(bool verbosePrint = true);

    /// \brief DetroyQueue() and then shut down HSA runtime
    ///
    /// \return true if there is no error
    static bool ShutDown();

    /// \brief Print hsa version numbers
    ///
    /// \return true if no error
    static bool PrintHsaVersion();

    /// \brief Get HSA version numbers
    ///
    /// \param[out] major Major version number
    /// \param[out] minor Minor version number
    /// \return true if no error
    static bool GetHsaVersion(uint16_t& major, uint16_t& minor);

    /// \brief Create a default HSA GPU queue. Note: assuming there is a
    ///        single queue with a single program
    /// \param[in] enableKernelTimestamps flag indicating whether or not profiling is enabled on this queue
    /// \return true if there is no error
    static bool CreateDefaultQueue(bool enableKernelTimestamps = false);

    /// \brief Finalize BRIG and create a default aql packet with 1 workitem
    ///
    /// \param[in]  brig            BRIG module to be finalize to ISA.
    /// \param[in]  kernelSymbol    Kernel entry point.
    /// \param[in]  bCreateSignal   Tell the function to whether to create
    ///                               a default signal value for AQL packet or not.
    /// \param[out] aqlPacketOut    The AQL packet which the finalized HSA program
    ///                             information will be put into.
    /// \param[in]  finalizerFlags  Additional compilation flags for finalizer.
    /// \return true if there is no error
    bool CreateAQLPacketFromBrig(
            const void*                   pBRIG,
            const std::string&            kernelSymbol,
            const bool                    bCreateSignal,
            hsa_kernel_dispatch_packet_t& aqlPacketOut,
            const std::string&            finalizerFlags = "");

    /// \brief Create a default aql packet from the existing executable with relative kernel symbol
    ///
    /// \param[in]  kernelSymbol    Kernel entry point
    /// \param[in]  bCreateSignal   Tell the function to whether to create
    ///                             a default signal value for AQL packet or not.
    /// \param[out] aqlPacketOut    The AQL packet which the finalized HSA program
    ///                             information will be put into.
    /// \return true if there is no error
    bool CreateAQLFromExecutable(
            const std::string&            kernelSymbol,
            const bool                    bCreateSignal,
            hsa_kernel_dispatch_packet_t& aqlPacketOut);

    /// \brief Copy one aql packet setting to another
    ///
    /// \param[in]  aqlPacket     The AQL packet to be copied from
    /// \param[in]  bCopySignal   Tell the function whether to copy the signal value from iAql.
    /// \param[out] aqlPacketOut  The AQL packet to be copied to.
    /// \return true if there is no error
    bool CopyKernelDispatchPacket(
        const hsa_kernel_dispatch_packet_t& aqlPacket,
        const bool                          bCopySignal,
        hsa_kernel_dispatch_packet_t& aqlPacketOut) const;

    /// \brief Append a kernel argument into argument buffer, m_argsBuff
    ///
    /// \param[in] pAddrToCopyIn The address of input kernel argument
    /// \param[in] argsSizeInBytes Size of the input kernel argument
    /// \param[in] offsetSize Depends on the source, we have additional global offset argument.
    ///                       For now the default value is for OpenCL kernel.
    /// \return true if there is no error
    bool AppendKernelArgs(
        const void*       pAddrToCopyIn,
        const std::size_t argsSizeInBytes,
        const std::size_t offsetSize = sizeof(uint64_t)*3);

    /// \brief Register kernel arguments buffer to the runtime
    ///
    /// \param[in] aqlPacket The aql packet which will take the kernel argument buffer
    /// \return true if there is no error
    bool RegisterKernelArgsBuffer(hsa_kernel_dispatch_packet_t& aqlPacket);

    /// \brief Deregister kernel arguments buffer.
    ///
    /// \return true if there is no error
    bool DeregisterKernelArgsBuffer();

    /// \brief Dispatch AQL kernel dispatch packet
    ///
    /// \param[in] aqlPacket The AQL packet going to be dispatch.
    /// \return true if there is no error
    bool Dispatch(hsa_kernel_dispatch_packet_t& aqlPacket);

    /// \brief Wait for the AQL packet completion signal value be set to 0 as completion.
    ///        Once the AQL dispatch complete, the signal value will be set back to 1 by
    ///        this function.
    ///
    /// \param[in] completionSignal The completion signal to be waited on
    /// \param[in] timeout          Maximum duration of the wait.  Specified in the same unit as the system timestamp.
    /// \param[in] outputTimingData flag indicating whether or not kernel timing info
    ///            is output to the console (only available if CreateDefaultQueue was
    ///            called with kernel timestamps enabled)
    /// \return true if there is no error
    bool WaitForCompletion(hsa_signal_t& completionSignal, uint64_t timeout = UINT64_MAX, bool outputTimingData = false);

    /// \brief Create a signal with default value 1.
    ///
    /// \param[out] signalOut The signal handle to be put into the created signal variable.
    /// \return true if there is no error
    bool CreateSignal(hsa_signal_t& signalOut);

    /// \brief Destroy a signal
    ///
    /// \param[in,out] signal The signal going to be destroy.
    /// \return true if there is no error
    bool DestroySignal(hsa_signal_t& signal);

    /// \brief Clean up function, release all signal variables which are created by CreateSignal(),
    ///        and m_prog, the HSA program handle member variable.
    ///
    /// \return true if there is no error
    bool CleanUp();

    /// \brief Destroy the queue created by CreateDefaultQueue().
    ///
    /// \return true if there is no error
    static bool DestroyQueue();

    /// \brief return whether it has an HSA runtime initialized in it.
    ///
    /// \return true if there is already an HSA runtime.
    static bool HasRuntime();

    // Accessors
    /// \brief return the GPU agent device
    ///
    /// \return GPU agent device
    static const hsa_agent_t& GPU();

    /// \brief Query GPU chip ID
    ///
    /// \return GPU chip ID
    static const uint32_t& GPUChipID();

    /// \brief return the default queue
    ///
    /// \return the default queue
    static hsa_queue_t* const& Queue();

private:
    /// \brief Copy constructor, temporary banned
    HSAResourceManager(const HSAResourceManager&);

    /// \brief assignment operator, temporary banned
    void operator=(const HSAResourceManager&);

    /// \brief Finalize pBRIG to m_executable
    bool Finalize(
        const void*                 pBRIG,
        const std::string&          compileFlags);

    // Member variables
    static uint16_t ms_hsaCount;
    static bool ms_hasRuntime;
    static bool ms_profilingEnabled;

    static hsa_agent_t ms_gpu;
    static uint32_t    ms_chipID;

    static hsa_queue_t* ms_pQueue;

    hsa_executable_t m_executable;
    hsa_code_object_t m_codeObj;

    std::vector<hsa_signal_t> m_signals;

#if defined(_WIN64) || defined(_WIN32)
    __declspec(align(16))
        unsigned char m_argsBuff[gs_MAX_ARGS_BUFF_SIZE];
#else
    __attribute__((aligned(16)))
        unsigned char m_argsBuff[gs_MAX_ARGS_BUFF_SIZE];
#endif

    std::size_t m_argsSize;
};

/// \brief Output device name and type to std::cout
///
/// \param[in] agent Current HSA agent
/// \param pData Dummy argument, in order to make this function as a valid callback function to hsa_iterate_agents()
/// \return HSA_STATUS_SUCCESS if no error
hsa_status_t QueryDevice(
    hsa_agent_t    agent,
    void*          pData = NULL);

/// \brief Get HSA GPU device
///
/// \param[in] agent Current HSA agent
/// \param[out] pData The GPU device to be put into
/// \return Check HSA status if any error occur
hsa_status_t GetGPUDevice(
    hsa_agent_t   agent,
    void*         pData);

/// \brief Check status.
///
/// \param[in] s The status need to be check
/// \return true if s == HSA_STATUS_SUCCESS
bool HsaCheckStatus(hsa_status_t s);

/// \brief Check status.
///
/// \param[in] s The status need to be check
/// \param[in] fileName Current file name
/// \param[in] lineNum Current line number in file
/// \return true if s == HSA_STATUS_SUCCESS
bool HsaCheckStatus(hsa_status_t s, const std::string& fileName, int lineNum);

/// \brief Convert hsa status enum to string
///
/// \param[in] s  the hsa status enum
/// \return return the string
std::string HsaStatusStrings(hsa_status_t s);

/// \brief Enable or disable soft CP mode (set HSA_EMULATE_AQL and HSA_TOOLS_LIB environment variables)
///
/// \param[in] bEnable       true to enable soft CP mode, false to disable soft CP mode
/// \param[in] verbosePrint  set to true to print the extra message outputs to console
/// \return true if successful
bool SetSoftCPMode(bool bEnable, bool verbosePrint = true);

} // namespace DevTools

#endif // _HSA_RESOURCE_MANAGER_H_
