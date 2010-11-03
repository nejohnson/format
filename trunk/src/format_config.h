/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010, Neil Johnson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ************************************************************************* */
 
#ifndef FORMAT_CONFIG_H
#define FORMAT_CONFIG_H
 
/*****************************************************************************/
/* Query the environment about what capabilities are available.              */
/*****************************************************************************/

/** Check if we are in the C hosted configuration **/
 
#if defined(__STDC_HOSTED__)
  #define CONFIG_HAVE_LIBC
#endif

/*****************************************************************************/
/**
    Some platforms va_list is an array type, on others it is a pointer (such as
    pointer to char).  These macros hide this important difference.
    
    The following is inspired from code from the kannel project.
    See http://www.kannel.org
**/

#if (defined(__linux__) && (defined(__powerpc__)    \
                           || defined(__s390__)     \
                           || defined(__x86_64)))   \
    || (defined(__FreeBSD__) && defined(__amd64__)) \
    || (defined(DARWIN) && defined(__x86_64__))
  #define CONFIG_VA_LIST_AS_ARRAY_TYPE
#endif

/*****************************************************************************/
/** If building in a freestanding environment with GCC then we usually need to
    provide our own memcpy(), otherwise we'll be pulling in glibc more than we
    strictly need to (if at all).
**/
#if !defined(CONFIG_HAVE_LIBC) && defined(__GNUC__)
  #define CONFIG_NEED_LOCAL_MEMCPY
#endif

 
 
#endif /* FORMAT_CONFIG.H */
