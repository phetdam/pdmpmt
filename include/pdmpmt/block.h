/**
 * @file block.h
 * @author Derek Huang
 * @brief C/C++ header for block structs to manage sized buffers
 * @copyright MIT License
 */

#ifndef PDMPMT_BLOCK_H_
#define PDMPMT_BLOCK_H_

#include <stddef.h>

#include "pdmpmt/common.h"
#include "pdmpmt/dllexport.h"

PDMPMT_EXTERN_C_BEGIN

/**
 * Block data structure managing a buffer of unsigned longs.
 */
typedef struct {
  unsigned long *data;
  size_t size;
} pdmpmt_block_ulong;

/**
 * Allocate a new ulong block.
 *
 * On error the `data` pointer of the block will be `NULL`.
 *
 * @param size Number of elements in the block to request
 */
PDMPMT_PUBLIC
pdmpmt_block_ulong
pdmpmt_block_ulong_alloc(size_t size) PDMPMT_NOEXCEPT;

/**
 * Allocate a new ulong block filled with zeros.
 *
 * On error the `data` pointer of the block will be `NULL`.
 *
 * @param size Number of elements in the block to request
 */
PDMPMT_PUBLIC
pdmpmt_block_ulong
pdmpmt_block_ulong_calloc(size_t size) PDMPMT_NOEXCEPT;

/**
 * Free an allocated ulong block.
 *
 * The data pointer is set to `NULL` so it is clear tha block is invalid.
 *
 * @param block Address of block to free
 */
PDMPMT_PUBLIC void
pdmpmt_block_ulong_free(pdmpmt_block_ulong *block) PDMPMT_NOEXCEPT;

PDMPMT_EXTERN_C_END

#endif  // PDMPMT_BLOCK_H_
