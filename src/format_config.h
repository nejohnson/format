/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010-2023, Neil Johnson
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

#if (__STDC_HOSTED__ == 1)
  #define CONFIG_HAVE_LIBC
#endif

/*****************************************************************************/
/* Configuration Profiles                                                    */
/*                                                                           */
/* Define ONE of these profiles for preset configurations:                  */
/*   CONFIG_PROFILE_FULL  - Full-featured version (~16KB)                   */
/*   CONFIG_PROFILE_TINY  - Reduced feature set (~3KB)                      */
/*   CONFIG_PROFILE_MICRO - Minimal feature set (~2.8KB)                    */
/*                                                                           */
/* If no profile is defined, CONFIG_PROFILE_FULL is assumed.                */
/*                                                                           */
/* Advanced users can override individual CONFIG_WITH_* flags below the     */
/* profile definitions to create custom configurations.                     */
/*****************************************************************************/

/* Uncomment ONE of these to select a profile: */
/* #define CONFIG_PROFILE_MICRO */
/* #define CONFIG_PROFILE_TINY */
/* #define CONFIG_PROFILE_FULL */

/* Default to FULL profile if none specified */
#if !defined(CONFIG_PROFILE_FULL) && !defined(CONFIG_PROFILE_TINY) && !defined(CONFIG_PROFILE_MICRO)
  #define CONFIG_PROFILE_FULL
#endif

/*****************************************************************************/
/* Granular Feature Flags                                                    */
/*                                                                           */
/* These flags control individual features. They are automatically set by   */
/* the profiles above, but can be overridden for custom configurations.     */
/*****************************************************************************/

/* ---- MICRO Profile Configuration ---- */
#if defined(CONFIG_PROFILE_MICRO)

  /* API: Use direct putchar interface (requires format_putchar()) */
  #define CONFIG_API_PUTCHAR

  /* Size limits */
  #define CONFIG_MAXWIDTH 80
  #define CONFIG_MAXPREC 80
  #define CONFIG_BUFLEN 50     /* 32-bit binary (34) + margin */

  /* Minimal feature set - only basic conversions enabled */
  /* No floating point, no grouping, no long long */
  /* No character conversion, no n conversion, no continuation */
  /* No alternate form, no engineering notation, no centering */
  /* No arbitrary base support (only binary, octal, decimal, hex) */

/* ---- TINY Profile Configuration ---- */
#elif defined(CONFIG_PROFILE_TINY)

  /* API: Use consumer function interface */
  #define CONFIG_API_CONSUMER

  /* Size limits */
  #define CONFIG_MAXWIDTH 80
  #define CONFIG_MAXPREC 80
  #define CONFIG_BUFLEN 50     /* 32-bit binary (34) + margin */

  /* Enabled features */
  #define CONFIG_WITH_CHAR_CONVERSION        /* %c support */
  #define CONFIG_WITH_CONTINUATION           /* %" specifier */

  /* Disabled features (commented for clarity) */
  /* No CONFIG_WITH_FP_SUPPORT */
  /* No CONFIG_WITH_LONG_LONG_SUPPORT */
  /* No CONFIG_WITH_GROUPING_SUPPORT */
  /* No CONFIG_WITH_N_CONVERSION */
  /* No CONFIG_WITH_FIXED_POINT_SUPPORT */
  /* No CONFIG_WITH_ALTERNATE_FORM */
  /* No CONFIG_WITH_ENGINEERING_NOTATION */
  /* No CONFIG_WITH_CENTERING */

/* ---- FULL Profile Configuration ---- */
#else /* CONFIG_PROFILE_FULL */

  /* API: Use consumer function interface */
  #define CONFIG_API_CONSUMER

  /* Size limits */
  #define CONFIG_MAXWIDTH 500
  #define CONFIG_MAXPREC 500
  #define CONFIG_BUFLEN 130    /* 64-bit binary + grouping (130) */

  /* All features enabled (can be individually disabled if needed) */
  #ifndef CONFIG_WITH_FP_SUPPORT
    #define CONFIG_WITH_FP_SUPPORT           /* Floating point (%f,%e,%g,%a) */
  #endif

  #ifndef CONFIG_WITH_LONG_LONG_SUPPORT
    #define CONFIG_WITH_LONG_LONG_SUPPORT    /* long long support (ll qualifier) */
  #endif

  #ifndef CONFIG_WITH_GROUPING_SUPPORT
    #define CONFIG_WITH_GROUPING_SUPPORT     /* Digit grouping ([,3]) */
  #endif

  #ifndef CONFIG_WITH_CHAR_CONVERSION
    #define CONFIG_WITH_CHAR_CONVERSION      /* %c support */
  #endif

  #ifndef CONFIG_WITH_N_CONVERSION
    #define CONFIG_WITH_N_CONVERSION         /* %n support */
  #endif

  #ifndef CONFIG_WITH_FIXED_POINT_SUPPORT
    #define CONFIG_WITH_FIXED_POINT_SUPPORT  /* %k support (requires FP) */
  #endif

  #ifndef CONFIG_WITH_ALTERNATE_FORM
    #define CONFIG_WITH_ALTERNATE_FORM       /* # flag support */
  #endif

  #ifndef CONFIG_WITH_ENGINEERING_NOTATION
    #define CONFIG_WITH_ENGINEERING_NOTATION /* ! flag support */
  #endif

  #ifndef CONFIG_WITH_CENTERING
    #define CONFIG_WITH_CENTERING            /* ^ flag support */
  #endif

  #ifndef CONFIG_WITH_CONTINUATION
    #define CONFIG_WITH_CONTINUATION         /* %" specifier */
  #endif

  #ifndef CONFIG_WITH_ARBITRARY_BASE
    #define CONFIG_WITH_ARBITRARY_BASE       /* %I/%U with :base modifier */
  #endif

#endif /* Profile selection */

/*****************************************************************************/
/* Dependency Checks                                                         */
/*****************************************************************************/

/* Fixed-point support requires floating point support */
#if defined(CONFIG_WITH_FIXED_POINT_SUPPORT) && !defined(CONFIG_WITH_FP_SUPPORT)
  #error "CONFIG_WITH_FIXED_POINT_SUPPORT requires CONFIG_WITH_FP_SUPPORT"
#endif

/* Engineering notation requires floating point support */
#if defined(CONFIG_WITH_ENGINEERING_NOTATION) && !defined(CONFIG_WITH_FP_SUPPORT)
  #error "CONFIG_WITH_ENGINEERING_NOTATION requires CONFIG_WITH_FP_SUPPORT"
#endif

#endif /* FORMAT_CONFIG_H */
