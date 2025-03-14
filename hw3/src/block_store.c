#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "bitmap.h"
#include "block_store.h"
// include more if you need


// You might find this handy. I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

///
/// This creates a new BS device, ready to go
/// \return Pointer to a new block storage device, NULL on error
///
block_store_t *block_store_create()
{
	block_store_t * b = (block_store_t *)calloc(1, sizeof(block_store_t));
	if(b == NULL)
	{
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return NULL;
	}
	b->bitmap = bitmap_overlay(BLOCK_STORE_NUM_BLOCKS, b->block_data);
    if (!b->bitmap) {
        fprintf(stderr, "%s:%d Bitmap overlay creation failed\n", __FILE__, __LINE__);
        free(b);
        return NULL;
    }

    // Mark bitmap blocks as allocated
    for (size_t i = 0; i < BITMAP_NUM_BLOCKS; ++i) {
        block_store_request(b, BITMAP_START_BLOCK + i);
    }
	return b;
}

///
/// Destroys the provided block storage device
/// This is an idempotent operation, so there is no return value
/// \param bs BS device
///
void block_store_destroy(block_store_t *const bs)
{
	if(bs != NULL){
		if(bs->bitmap!=NULL){
			bitmap_destroy(bs->bitmap);
		}
		free(bs);
	}
	else{
		fprintf(stderr, "%s:%d block is already free\n", __FILE__, __LINE__);
	}
}
///
/// Searches for a free block, marks it as in use, and returns the block's id
/// \param bs BS device
/// \return Allocated block's id, SIZE_MAX on error
///
size_t block_store_allocate(block_store_t *const bs)
{
	// check parameters for null
	if(!bs || !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return SIZE_MAX;
	}
	// get first free memory
	size_t free = bitmap_ffz(bs->bitmap);
	// check if the location is outside of memory
	if(free == SIZE_MAX || free >= BLOCK_STORE_NUM_BLOCKS){
		return SIZE_MAX;
	}
	// set memory
	bitmap_set(bs->bitmap,free);
	// return location
	return free;
}
///
/// Attempts to allocate the requested block id
/// \param bs the block store object
/// \block_id the requested block identifier
/// \return boolean indicating succes of operation
///
bool block_store_request(block_store_t *const bs, const size_t block_id)
{
	// check for null parameters
	if(!bs|| !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return false;
	}
	// check for valid block id
	if(block_id >= BLOCK_STORE_NUM_BLOCKS){
		fprintf(stderr, "%s:%d block id is invalid\n", __FILE__, __LINE__);
		return false;
	}
	// check for already allocated memory
	if(bitmap_test(bs->bitmap,block_id)){
		fprintf(stderr, "%s:%d block is already allocated\n", __FILE__, __LINE__);
		return false;
	}
	// set the bitmap for new memory
	bitmap_set(bs->bitmap, block_id);
	// return the value if memory is set
	if(bitmap_test(bs->bitmap,block_id)){
		return true;
	}
	
	return false;
}

void block_store_release(block_store_t *const bs, const size_t block_id)
{
	// check for null parameters
	if(!bs|| !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return;
	}
	// check for valid block id
	if(block_id >= BLOCK_STORE_NUM_BLOCKS){
		fprintf(stderr, "%s:%d block id is invalid\n", __FILE__, __LINE__);
		return;
	}
	// reset value in the bitmap
	bitmap_reset(bs->bitmap, block_id);
}
///
/// Counts the number of blocks marked as in use
/// \param bs BS device
/// \return Total blocks in use, SIZE_MAX on error
///
size_t block_store_get_used_blocks(const block_store_t *const bs)
{
	if(!bs || !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return SIZE_MAX;
	}
	return bitmap_total_set(bs->bitmap);
}
///
/// Counts the number of blocks marked free for use
/// \param bs BS device
/// \return Total blocks free, SIZE_MAX on error
///
size_t block_store_get_free_blocks(const block_store_t *const bs)
{
	if(!bs || !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return SIZE_MAX;
	}
	size_t used = block_store_get_used_blocks(bs);
	return BLOCK_STORE_NUM_BLOCKS - used;
}
///
/// Returns the total number of user-addressable blocks
///  (since this is constant, you don't even need the bs object)
/// \return Total blocks
///
size_t block_store_get_total_blocks()
{
	return BLOCK_STORE_NUM_BLOCKS;
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	UNUSED(bs);
	UNUSED(block_id);
	UNUSED(buffer);
	return 0;
}

size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
	UNUSED(bs);
	UNUSED(block_id);
	UNUSED(buffer);
	return 0;
}

block_store_t *block_store_deserialize(const char *const filename)
{
	UNUSED(filename);
	return NULL;
}

size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
	UNUSED(bs);
	UNUSED(filename);
	return 0;
}
