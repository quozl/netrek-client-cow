/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * COW LiveConnect Sample Plugin
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 ******************************************************************************/

/*
** Ok, so we don't usually include .c files (only .h files) but we're
** doing it here to avoid some fancy make rules. First pull in the common
** glue code:
*/
#ifdef XP_UNIX
#include "npunix.c"
#endif

/*
** Next, define IMPLEMENT_Simple in order to pull the stub code in when
** compiling the COW.c file. Without doing this, we'd get a
** UnsatisfiedLinkError whenever we tried to call one of the native
** methods:
*/
#define IMPLEMENT_COW

/*
** Finally, include the native stubs, initialization routines and
** debugging code. You should be building with DEBUG defined in order to
** take advantage of the diagnostic code that javah creates for
** you. Otherwise your life will be hell.
*/
#ifdef XP_UNIX
#include "_stubs/COW.c"
#include "_stubs/netscape_plugin_Plugin.c"
/*
#include "_stubs/java_lang_Class.c"
#include "_stubs/java_lang_String.c"
*/
#else
#include "COW.c"
#include "netscape_plugin_Plugin.c"
/*
#include "java_lang_Class.c"
#include "java_lang_String.c"
*/
#endif
