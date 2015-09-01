////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation (if
// any) (collectively, the "Materials") pursuant to the terms and conditions of
// the Software License Agreement included with the Materials. If you do not
// have a copy of the Software License Agreement, contact your AMD
// representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND. AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE. THE ENTIRE RISK ASSOCIATED WITH THE USE
// OF THE SOFTWARE IS ASSUMED BY YOU. Some jurisdictions do not allow the
// exclusion of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION: AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD. You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor. Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _AMD_HSA_TOOLS_INTERFACES_H_
#define _AMD_HSA_TOOLS_INTERFACES_H_

#include "hsa.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Hash define for exporting the API functions.
 */
#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define HSA_TOOLS_API __attribute__ ((dllexport))
#else
#define HSA_TOOLS_API __declspec(dllexport)  // Note: actually gcc seems
                                             // to also supports this
                                             // syntax.
#endif
#ifndef DLL_LOCAL
#define DLL_LOCAL
#endif

#else  // defined _WIN32 || defined __CYGWIN__
#if __GNUC__ >= 4
#define HSA_TOOLS_API __attribute__ ((visibility("default")))
#ifndef DLL_LOCAL
#define DLL_LOCAL  __attribute__ ((visibility("hidden")))
#endif
#else
#define HSA_TOOLS_API
#ifndef DLL_LOCAL
#define DLL_LOCAL
#endif
#endif
#endif  // defined _WIN32 || defined __CYGWIN__

//---------------------------------------------------------------------------//
// Pre-dispatch call back function signature. This is the call back fuction  //
// before the kernel dispatch. The call back function is to indicate the     //
// start of the kernel launch. It is used by the debugger and profiler.      //
//---------------------------------------------------------------------------//
typedef void * 
  (*hsa_ext_tools_pre_dispatch_callback_function) 
  (const hsa_agent_t agent, const hsa_queue_t *queue, 
  void *aql_translation_handle, void *correlation_handle, 
  hsa_kernel_dispatch_packet_t *aql_packet,
  void *user_args);

//---------------------------------------------------------------------------//
// Post-dispatch call back function signature. The post dispatch callback    //
// fuction is called after the kernel dispatch. The callback function is to  //
// indicate the completion of the the kernel launch. It is used by the       //
// debugger and profiler.                                                    //
//---------------------------------------------------------------------------//
typedef void * 
  (*hsa_ext_tools_post_dispatch_callback_function) 
  (const hsa_agent_t agent, const hsa_queue_t *queue, 
  void *aql_translation_handle, hsa_signal_t signal, void *user_args);

//---------------------------------------------------------------------------//
// @brief Setup the callback function pointers                               //
// @param queue Pointer to queue that the callback functions are bind to     //
// @param pre_dispatch_function Pointer to predispatch callback function     //
// @param post_dispatch_function Pointer to postdispatch callback function   //
// @retval        - HSA_STATUS_SUCCESS                                       //
//---------------------------------------------------------------------------//
hsa_status_t HSA_TOOLS_API hsa_ext_tools_set_callback_functions(
  hsa_queue_t *queue,
  hsa_ext_tools_pre_dispatch_callback_function pre_dispatch_function,
  hsa_ext_tools_post_dispatch_callback_function post_dispatch_function);

//---------------------------------------------------------------------------//
// @brief Query the callback function pointers                               //
// @param queue Pointer to queue that the callback functions are bind to     //
// @param pre_dispatch_function Pointer to predispatch callback function     //
// @param post_dispatch_function Pointer to postdispatch callback function   //
// @retval        - HSA_STATUS_SUCCESS                                       //
//---------------------------------------------------------------------------//
hsa_status_t HSA_TOOLS_API hsa_ext_tools_get_callback_functions(
    hsa_queue_t *queue,
    hsa_ext_tools_pre_dispatch_callback_function &pre_dispatch_function,
    hsa_ext_tools_post_dispatch_callback_function &post_dispatch_function);

//---------------------------------------------------------------------------//
// @brief Setup the call back function argument pointers                     //
// @param queue Pointer to queue that the callback functions are bind to     //
// @param pre_dispatch_args Pointer to predispatch callback function         //
// arguments                                                                 //
// @param post_dispatch_args Pointer to postdispatch callback function       //
// arguments                                                                 //
// @retval        - HSA_STATUS_SUCCESS                                       //
//---------------------------------------------------------------------------//
hsa_status_t HSA_TOOLS_API hsa_ext_tools_set_callback_arguments(
  hsa_queue_t *queue,
  void *pre_dispatch_args,
  void *post_dispatch_args);

//---------------------------------------------------------------------------//
// @brief Query the call back function argument pointers                     //
// @param queue Pointer to queue that the callback functions are bind to     //
// @param pre_dispatch_args Pointer to predispatch callback function         //
// arguments                                                                 //
// @param post_dispatch_args Pointer to postdispatch callback function       //
// arguments                                                                 //
// @retval        - HSA_STATUS_SUCCESS                                       //
//---------------------------------------------------------------------------//
hsa_status_t HSA_TOOLS_API hsa_ext_tools_get_callback_arguments(
    hsa_queue_t *queue,
    void * &pre_dispatch_args,
    void * &post_dispatch_args);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // _AMD_HSA_TOOLS_INTERFACES_H_
