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
	// check for invalid parameters
	if(!bs || !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return SIZE_MAX;
	}

	// call bitmap_total_set to get number of blocks in use
	return bitmap_total_set(bs->bitmap);
}
///
/// Counts the number of blocks marked free for use
/// \param bs BS device
/// \return Total blocks free, SIZE_MAX on error
///
size_t block_store_get_free_blocks(const block_store_t *const bs)
{
	// check for invalid parameters
	if(!bs || !bs->bitmap){
		fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
		return SIZE_MAX;
	}

	// call block_store_get_used_blocks, and its difference with BLOCK_STORE_NUM_BLOCKS
	// is the number of free blocks
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

///
/// Reads data from the specified block and writes it to the designated buffer
/// \param bs BS device
/// \param block_id Source block id
/// \param buffer Data buffer to write to
/// \return Number of bytes read, 0 on error
///
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	// check for valid block pointer
	if(!bs || !bs->bitmap || !buffer)
	{
                fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
                return 0;
        }
	// check for valid block id
        if(block_id >= BLOCK_STORE_NUM_BLOCKS)
	{
                fprintf(stderr, "%s:%d block id is invalid\n", __FILE__, __LINE__);
                return 0;
        }

	// determining how many bytes to offset in the block_data array
	size_t byte_offset = block_id * BLOCK_SIZE_BYTES;

	// copies (i.e. reads) the memory in block_data at the offset into buffer
	memcpy(buffer, bs->block_data + byte_offset, BLOCK_SIZE_BYTES);

	// if successful, should've read one block from the device
	return BLOCK_SIZE_BYTES;
}

///
/// Reads data from the specified buffer and writes it to the designated block
/// \param bs BS device
/// \param block_id Destination block id
/// \param buffer Data buffer to read from
/// \return Number of bytes written, 0 on error
///
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
        // check for valid block pointer
        if(!bs || !bs->bitmap || !buffer)
        {
                fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
                return 0;
        }
        // check for valid block id
        if(block_id >= BLOCK_STORE_NUM_BLOCKS)
        {
                fprintf(stderr, "%s:%d block id is invalid\n", __FILE__, __LINE__);
                return 0;
        }

	// determining how many bytes to offset in the block_data array
	size_t byte_offset = block_id * BLOCK_SIZE_BYTES;

    	// copies (i.e. writes) the memory in the buffer into block_data at the offset
    	memcpy(bs->block_data + byte_offset, buffer, BLOCK_SIZE_BYTES);
	
	// if successful, should've written one block to the device
    	return BLOCK_SIZE_BYTES;
}

///
/// Reads data from the specified file and writes it to the designated block store
/// \param filename the name of the file to read information from
/// \return the block store
///
block_store_t *block_store_deserialize(const char *const filename)
{
	if (filename == NULL) {
		return NULL;
	}

	// Open the file for reading in binary mode
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file for deserialization: %s\n", filename);
		return NULL;
	}

	// Allocate memory for the new block store
	block_store_t* bs = (block_store_t*)malloc(sizeof(block_store_t));
	if (bs == NULL) {
		fclose(file);
		return NULL;
	}

	// Read block_number from the file
	size_t read = fread(&bs->block_number, sizeof(size_t), 1, file);
	if (read != 1) {
		free(bs);
		fclose(file);
		return NULL;
	}

	// Read block_data from the file
	read = fread(bs->block_data, sizeof(uint8_t), BLOCK_STORE_NUM_BLOCKS, file);
	if (read != BLOCK_STORE_NUM_BLOCKS) {
		free(bs);
		fclose(file);
		return NULL;
	}

	// Allocate memory for the bitmap
	bs->bitmap = bitmap_create(BITMAP_NUM_BLOCKS);
	if (bs->bitmap == NULL) {
		free(bs);
		fclose(file);
		return NULL;
	}

	// Read the bitmap data from the file
	uint8_t* bitmap_data = (uint8_t*)bitmap_export(bs->bitmap);
	read = fread(bitmap_data, sizeof(uint8_t), BITMAP_NUM_BLOCKS, file);
	if (read != BITMAP_NUM_BLOCKS) {
		bitmap_destroy(bs->bitmap);
		free(bs);
		fclose(file);
		return NULL;
	}

	fclose(file);

	// Return the deserialized block store
	return bs;
}

///
/// Reads data from the specified block and writes it to the designated file
/// \param bs BS device
/// \param filename the name of the file to save to
/// \return Number of bytes written, 0 on error
///
size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
	if (bs == NULL || filename == NULL) {
		return 0;
	}

	// Open file for writing in binary mode
	FILE* file = fopen(filename, "wb");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file for serialization: %s\n", filename);
		return 0;
	}

	size_t bytesWritten = 0;

	// Write block_number to the file
	size_t written = fwrite(&bs->block_number, sizeof(size_t), 1, file);
	if (written != 1) {
		fclose(file);
		return 0;
	}
	bytesWritten += sizeof(size_t);

	// Write the block_data to the file
	written = fwrite(bs->block_data, sizeof(uint8_t), BLOCK_STORE_NUM_BLOCKS, file);
	if (written != BLOCK_STORE_NUM_BLOCKS) {
		fclose(file);
		return 0;
	}
	bytesWritten += BLOCK_STORE_NUM_BLOCKS;

	// Serialize the bitmap using bitmap_export to get the raw data
	if (bs->bitmap != NULL) {
		const uint8_t* bitmap_data = bitmap_export(bs->bitmap);  //Gives access to the raw bitmap data
		if (bitmap_data != NULL) {
			written = fwrite(bitmap_data, sizeof(uint8_t), BITMAP_NUM_BLOCKS, file);
			if (written != BITMAP_NUM_BLOCKS) {
				fclose(file);
				return 0;
			}
			bytesWritten += BITMAP_NUM_BLOCKS;
		}
	}

	// Pad the file to the expected size
	size_t totalSize = BLOCK_STORE_NUM_BYTES;
	if (bytesWritten < totalSize) {
		size_t padding = totalSize - bytesWritten;
		uint8_t* zeroBuffer = (uint8_t*)calloc(padding, sizeof(uint8_t)); // Allocate zero-filled buffer
		if (zeroBuffer != NULL) {
			fwrite(zeroBuffer, sizeof(uint8_t), padding, file);
			bytesWritten += padding;
			free(zeroBuffer);
		}
	}

	fclose(file);

	return bytesWritten; // Return the total bytes written to the file
}
