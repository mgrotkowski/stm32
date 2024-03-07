#ifndef GIF_H
#define GIF_H
#include <stdint.h>
#include "globals.h"

// Loaded GIF offsets 
#define MAX_CODE_SIZE 12
#define GCT_OFFSET 0xD
#define DATA_OFFSET 0x35e
#define LZW_SMALLEST_CODE_SIZE 6

extern const uint32_t gce_offsets[NUM_FRAMES];

typedef struct FrameMetadata
{
    uint16_t x_offset;
    uint16_t y_offset;
    uint16_t height;
    uint16_t width;
    uint32_t data_offset;
    uint32_t color_table_offset;
    uint16_t lzw_smallest_code_size;
    uint8_t transparency;
    uint8_t transparency_idx;
    
} FrameMetadata;


typedef struct ImageDescriptorHeader
{

    uint16_t x_offset;
    uint16_t y_offset;
    uint16_t height;
    uint16_t width;
    uint8_t flags;

} ImageDescriptorHeader;

typedef struct GCEHeader
{
    uint8_t block_size;
    uint8_t flags;
    uint16_t delay_time;
    uint8_t transparent_color_index;
} GCEHeader;


void decodeGIF(FrameMetadata);
void decodeGIF_args(FrameMetadata);
FrameMetadata parseGIFHeaders(uint32_t);

#endif
