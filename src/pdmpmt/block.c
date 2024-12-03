/**
 * @file pdmpmt/block.c
 * @author Derek Huang
 * @brief C source for block structs to manage sized buffers
 * @copyright MIT License
 */

#include "pdmpmt/block.h"

#include <stddef.h>
#include <stdlib.h>

pdmpmt_block_ulong
pdmpmt_block_ulong_alloc(size_t size)
{
  // new block (invalid as data is NULL)
  pdmpmt_block_ulong block;
  block.data = NULL;
  // size must be nonzero
  if (!size)
    return block;
  // attempt malloc (NULL on error)
  block.data = malloc(sizeof(*block.data) * size);
  return block;
}

pdmpmt_block_ulong
pdmpmt_block_ulong_calloc(size_t size)
{
  // new block (invalid as data is NULL)
  pdmpmt_block_ulong block;
  block.data = NULL;
  // size must be nonzero
  if (!size)
    return block;
  // attempt calloc (NULL on error)
  block.data = calloc(size, sizeof(*block.data));
  return block;
}

void
pdmpmt_block_ulong_free(pdmpmt_block_ulong *block)
{
  // no-op if invalid
  if (!block->data)
    return;
  // free and set to NULL
  free(block->data);
  block->data = NULL;
}
