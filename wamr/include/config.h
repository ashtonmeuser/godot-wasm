
/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/* Replaced by godot-wasm */

#define WASM_ENABLE_INTERP 1
#define WASM_ENABLE_FAST_INTERP 1
#define WASM_ENABLE_SHARED_HEAP 0
#define WASM_ENABLE_SHARED_MEMORY 0
#define WASM_ENABLE_LIBC_WASI 0
#define WASM_ENABLE_GC 0
#define WASM_ENABLE_WASM_CACHE 0
#define WASM_ENABLE_INVOKE_NATIVE 0
#define WASM_ENABLE_JIT 0
#define WASM_ENABLE_FAST_JIT 0
#define WASM_ENABLE_SIMDE 0

#define BH_MALLOC wasm_runtime_malloc
#define BH_FREE wasm_runtime_free

#ifndef WASM_TABLE_MAX_SIZE
#define WASM_TABLE_MAX_SIZE 1024
#endif
 
/* Default wasm block address cache size and conflict list size */
#ifndef BLOCK_ADDR_CACHE_SIZE
#define BLOCK_ADDR_CACHE_SIZE 64
#endif
#define BLOCK_ADDR_CONFLICT_SIZE 2 

#ifndef DEFAULT_QUEUE_LENGTH
#define DEFAULT_QUEUE_LENGTH 50
#endif

/* Default min/max heap size of each app */
#ifndef APP_HEAP_SIZE_DEFAULT
#define APP_HEAP_SIZE_DEFAULT (8 * 1024)
#endif
#define APP_HEAP_SIZE_MIN (256)

/* The ems memory allocator supports maximal heap size 1GB,
   see ems_gc_internal.h */
#define APP_HEAP_SIZE_MAX (1024 * 1024 * 1024)

/* Default/min native stack size of each app thread */
#if defined(PTHREAD_STACK_DEFAULT) && defined(PTHREAD_STACK_MIN)
#define APP_THREAD_STACK_SIZE_DEFAULT PTHREAD_STACK_DEFAULT
#define APP_THREAD_STACK_SIZE_MIN PTHREAD_STACK_MIN
#else
#define APP_THREAD_STACK_SIZE_DEFAULT (128 * 1024)
#define APP_THREAD_STACK_SIZE_MIN (24 * 1024)
#endif

/* Max native stack size of each app thread */
#if !defined(APP_THREAD_STACK_SIZE_MAX)
#define APP_THREAD_STACK_SIZE_MAX (8 * 1024 * 1024)
#endif

#ifndef WASM_CONST_EXPR_STACK_SIZE
#if WASM_ENABLE_GC != 0
#define WASM_CONST_EXPR_STACK_SIZE 8
#else
#define WASM_CONST_EXPR_STACK_SIZE 4
#endif
#endif

#define WASM_STACK_GUARD_SIZE (4096 * 6)
#define DEFAULT_WASM_STACK_SIZE (16 * 1024)
#define AOT_CURRENT_VERSION 5


#ifndef _CONFIG_H_
#define _CONFIG_H_

#endif /* end of _CONFIG_H_ */
