#include <stdint.h>
#include <string.h>
#include "globals.h"
#include "gif.h"

//#define DATA_OFFSET 0xc0 //do not start_with LZW_SMALLEST_CODE_SIZE only chunk_len
#define LINK_UNUSED 5911
#define LINK_END 5912

uint8_t imageBytes[2*128*160];
uint8_t* frame_buffer = imageBytes;

static uint16_t string_ends[1 << MAX_CODE_SIZE];
static uint16_t string_beginning[1 << MAX_CODE_SIZE];
static uint16_t string_nodes[1 << MAX_CODE_SIZE];

static uint16_t temp_str_buffer[1 << MAX_CODE_SIZE];

static uint32_t frame_buffer_ptr = 0;

typedef struct decoding_args
{                             
    uint32_t file_offset;
    uint8_t lzw_chunk_bytes_read;
    uint8_t lzw_chunk_length;
    uint8_t num_bits;
    uint32_t code_chunk;
} decoding_args;

// Read the next 4 bytes of LZW decoding data from file
static uint32_t get_next_chunk_args(uint8_t* file_ptr, decoding_args* args)

{
    uint32_t return_chunk;
    //check if we're still inside LZW chunk
    if (args->lzw_chunk_bytes_read + 4 <= args->lzw_chunk_length)
    {
        return_chunk = *((uint32_t*)&file_ptr[args->file_offset + args->lzw_chunk_bytes_read]);
        args->lzw_chunk_bytes_read+=4;
        args->num_bits = 32;
    }
    //if not read remaining bytes and go to next chunk
    else
    {
        uint8_t bytes_remaining = args->lzw_chunk_length - args->lzw_chunk_bytes_read;
        if(bytes_remaining)
        {
            return_chunk =  *((uint32_t*)&file_ptr[args->file_offset + args->lzw_chunk_bytes_read]) & ((0x1 << bytes_remaining * 8) -1);
            args->num_bits = 8 * bytes_remaining;
        }
        // we read all the chunks
        if(!args->lzw_chunk_length)
            return return_chunk;

        // go to the next chunk
        args->file_offset += args->lzw_chunk_length;
        args->lzw_chunk_length = file_ptr[args->file_offset];
        args->file_offset += 1;
        args->lzw_chunk_bytes_read = 0;
        if (!bytes_remaining)
        {
            return_chunk =  *((uint32_t*)&file_ptr[args->file_offset + args->lzw_chunk_bytes_read]);
            args->lzw_chunk_bytes_read += 4;
            args->num_bits = 32;
        }
    }

    return return_chunk;


}


// Get variable length LZW code from 4-byte storage chunk
// shift current chunk or get new one if current code 
// excedes number of available bits 
// code_bits - length in bits of LZW code to read from chunk
// code_mask - mask corresponding to code_bits length code

static uint16_t get_code_args(
                         uint16_t code_mask, 
                         uint16_t code_bits,
                         uint8_t* file_ptr,
                         decoding_args* args
)
                         
{
   uint16_t return_value;
   if (args->num_bits > code_bits)  
   {
        args->num_bits -= code_bits;
         return_value = args->code_chunk;
        args->code_chunk = args->code_chunk >> code_bits;
   }
   else if (args->num_bits == code_bits)
   {
       return_value = args->code_chunk;
       args->code_chunk = get_next_chunk_args(file_ptr, args);
   }
   else
   {
        return_value = args->code_chunk;
        uint8_t old_num_bits = args->num_bits;
        args->code_chunk = get_next_chunk_args(file_ptr, args);
        return_value = ((args->code_chunk << old_num_bits) & code_mask) | return_value;
        if (args->num_bits < code_bits - old_num_bits)
        {
             old_num_bits += args->num_bits;
             args->code_chunk = get_next_chunk_args(file_ptr, args);
             return_value =((args->code_chunk << old_num_bits ) & code_mask) | return_value; 
        }
        args->code_chunk = args->code_chunk >> (code_bits - old_num_bits);
        args->num_bits -= code_bits - old_num_bits;
   }

    return return_value & code_mask;
}

                               


static uint32_t get_next_chunk(uint8_t* file_ptr,
                               uint32_t* file_offset, 
                               uint8_t* lzw_chunk_bytes_read,
                               uint8_t* lzw_chunk_length,
                               uint8_t* num_bits)

{
    uint32_t return_chunk;
    //check if we're still inside LZW chunk
    if (*lzw_chunk_bytes_read + 4 <= *lzw_chunk_length)
    {
        return_chunk = *((uint32_t*)&file_ptr[*file_offset + *lzw_chunk_bytes_read]);
        *lzw_chunk_bytes_read+=4;
        *num_bits = 32;
    }
    //if not read remaining bytes and go to next chunk
    else
    {
        uint8_t bytes_remaining = *lzw_chunk_length - *lzw_chunk_bytes_read;
        if(bytes_remaining)
        {
            return_chunk =  *((uint32_t*)&file_ptr[*file_offset + *lzw_chunk_bytes_read]) & ((0x1 << bytes_remaining * 8) -1);
            *num_bits = 8 * bytes_remaining;
        }
        // we read all the chunks
        if(!*lzw_chunk_length)
            return return_chunk;

        // go to the next chunk
        *file_offset += *lzw_chunk_length;
        *lzw_chunk_length = file_ptr[*file_offset];
        *file_offset += 1;
        *lzw_chunk_bytes_read = 0;
        if (!bytes_remaining)
        {
            return_chunk =  *((uint32_t*)&file_ptr[*file_offset + *lzw_chunk_bytes_read]);
            *lzw_chunk_bytes_read += 4;
            *num_bits = 32;
        }
    }

    return return_chunk;


}
                               

static uint16_t get_code(uint16_t code_mask, 
                         uint16_t code_bits,
                         uint32_t* code_chunk,
                         uint8_t* chunk_len,
                         uint8_t* file_ptr,
                         uint32_t* file_offset,
                         uint8_t* lzw_chunk_bytes_read,
                         uint8_t* lzw_chunk_length,
                         uint8_t* num_bits)
                         
{
   uint16_t return_value;
   if (*num_bits > code_bits)  
   {
        *num_bits -= code_bits;
         return_value = *code_chunk;
        *code_chunk = *code_chunk >> code_bits;
   }
   else if (*num_bits == code_bits)
   {
       return_value = *code_chunk;
       *code_chunk = get_next_chunk(file_ptr, file_offset, lzw_chunk_bytes_read, lzw_chunk_length, num_bits);
   }
   else
   {
        return_value = *code_chunk;
        uint8_t old_num_bits = *num_bits;
        *code_chunk = get_next_chunk(file_ptr, file_offset, lzw_chunk_bytes_read, lzw_chunk_length, num_bits);
        return_value = ((*code_chunk << old_num_bits) & code_mask) | return_value;
        if (*num_bits < code_bits - old_num_bits)
        {
             old_num_bits += *num_bits;
             *code_chunk = get_next_chunk(file_ptr, file_offset, lzw_chunk_bytes_read, lzw_chunk_length, num_bits);
             return_value =((*code_chunk << old_num_bits ) & code_mask) | return_value; 
        }
        *code_chunk = *code_chunk >> (code_bits - old_num_bits);
        *num_bits -= code_bits - old_num_bits;
   }

    return return_value & code_mask;
}


static void decode_pixels(uint16_t code, uint8_t* gct)
{
    uint16_t* s = temp_str_buffer;
    while(code < LINK_END)
    {
        *s++ = string_ends[code];
        code = string_nodes[code];
    }

    while(s != temp_str_buffer)
    {
        uint16_t code = *--s;
        uint16_t ret_color = (gct[3*code + 2] >> 3 | ((uint16_t)gct[3*code + 1] >> 2) << 5 | ((uint16_t) gct[3*code] >> 3) << 11); 
        // Store MSB first 
        frame_buffer[frame_buffer_ptr++] = (ret_color >> 8) & 0xff; 
        frame_buffer[frame_buffer_ptr++] = ret_color & 0xff;
    }

}

void decodeGIF_args(FrameMetadata metadata)
{
    uint8_t* lzw_data = &gifBytes[metadata.data_offset];
    uint8_t* gct = &gifBytes[metadata.color_table_offset];

    decoding_args args;

    args.file_offset = 0;
    args.lzw_chunk_length= lzw_data[args.file_offset++];
    args.lzw_chunk_bytes_read = 0;
    args.num_bits = 32;
    
    uint16_t cc = 1 << metadata.lzw_smallest_code_size;
    uint16_t eoi = cc + 1;
    uint8_t  code_size = metadata.lzw_smallest_code_size + 1; 
    uint16_t code_mask = (0x1 << code_size) - 1;
    uint16_t code_table_head = cc+2;
    uint16_t max_new_items = 1 << code_size;

    for(uint16_t i = 0; i < cc; i++)
    {
        string_nodes[i] = LINK_END;
        string_beginning[i] = i;
        string_ends[i] = i;
    }

    args.code_chunk = get_next_chunk_args(lzw_data, &args);
    uint16_t code;
    memset(&string_nodes[cc], LINK_END, (unsigned long)(4096 - cc)*sizeof(uint16_t));
    code = get_code_args(code_mask, code_size, lzw_data, &args);

    if (code == cc)
        code = get_code_args(code_mask, code_size, lzw_data, &args);

    // RGB 888 -> RGB 565 conversion
    uint16_t ret_color = (gct[3*code + 2] >> 3 | ((uint16_t)gct[3*code + 1] >> 2) << 5 | ((uint16_t) gct[3*code] >> 3) << 11); 
    // Store MSB first 
    frame_buffer[frame_buffer_ptr++] = (ret_color >> 8) & 0xff; 
    frame_buffer[frame_buffer_ptr++] = ret_color & 0xff;

    uint16_t prev_code = code;

    while(code != eoi)
    {
        code = get_code_args(code_mask, code_size, lzw_data, &args);

        if(code == eoi)
                break;

        if (code < code_table_head)
        {
            if (code == cc)
            {

                 memset(&string_nodes[cc], LINK_END, (unsigned long)(4096 - cc)*sizeof(uint16_t));
                 code_size = metadata.lzw_smallest_code_size + 1;
                 code_mask = (0x1 << code_size) - 1;
                 code_table_head = (0x1 << metadata.lzw_smallest_code_size) + 2;
                 code = get_code_args(code_mask, code_size, lzw_data, &args);
                 prev_code = code;
                 max_new_items = 1 << code_size;
            }
            else
            {
                string_beginning[code_table_head] = string_beginning[prev_code];
                string_ends[code_table_head] = string_beginning[code];
                string_nodes[code_table_head] = prev_code;
                prev_code = code;
                code_table_head++;
                decode_pixels(code, gct);
            }
        }

        else if (code == code_table_head)
        {
            string_beginning[code_table_head] = string_beginning[prev_code];
            string_ends[code] = string_beginning[prev_code];
            string_nodes[code] = prev_code;
            prev_code = code;
            code_table_head++;
            decode_pixels(code, gct);
        }

        if (code_table_head == max_new_items && code_size < 12)
        {
            code_size++;
            max_new_items = 1 << code_size;
            code_mask = (0x1 << code_size) - 1;
        }


    }

    uint16_t gct_first_color = (gct[2] >> 3 | ((uint16_t)gct[1] >> 2) << 5 | ((uint16_t) gct[0] >> 3) << 11); 
    while (frame_buffer_ptr < (SCREEN_WIDTH - metadata.x_offset) * (SCREEN_HEIGHT - metadata.y_offset))
    {
        frame_buffer[frame_buffer_ptr++] = (gct_first_color >> 8) & 0xff;
        frame_buffer[frame_buffer_ptr++] = gct_first_color & 0xff;
    }
    frame_buffer_ptr = 0;

}


void decodeGIF(FrameMetadata meta_data)
{

    uint8_t* lzw_data = &gifBytes[meta_data.data_offset];
    uint8_t* gct = &gifBytes[meta_data.color_table_offset];

    uint32_t data_ptr = 0;
    uint8_t chunk_len = lzw_data[data_ptr++];
    uint8_t chunks_read = 0;
    uint8_t num_bits = 32;


    uint16_t cc = 1 << meta_data.lzw_smallest_code_size;
    uint16_t eoi = cc + 1;
    uint8_t code_size = meta_data.lzw_smallest_code_size + 1; 
    uint16_t code_mask = (0x1 << code_size) - 1;
    uint16_t code_table_head = cc+2;
    uint16_t max_new_items = 1 << code_size;

    for(uint16_t i = 0; i < cc; i++)
    {
        string_nodes[i] = LINK_END;
        string_beginning[i] = i;
        string_ends[i] = i;
    }

    uint32_t code_chunk = get_next_chunk(lzw_data, 
                                         &data_ptr, 
                                         &chunks_read, 
                                         &chunk_len, 
                                         &num_bits);
    uint16_t code;


    memset(&string_nodes[cc], LINK_END, (unsigned long)(4096 - cc)*sizeof(uint16_t));
    code = get_code(code_mask, 
                    code_size, 
                    &code_chunk, 
                    &chunk_len,
                    lzw_data,
                    &data_ptr,
                    &chunks_read,
                    &chunk_len,
                    &num_bits); 
    if (code == cc)
    {
        code = get_code(code_mask, 
                    code_size, 
                    &code_chunk, 
                    &chunk_len,
                    lzw_data,
                    &data_ptr,
                    &chunks_read,
                    &chunk_len,
                    &num_bits); 
    }

    // RGB 888 -> RGB 565 conversion
    uint16_t ret_color = (gct[3*code + 2] >> 3 | ((uint16_t)gct[3*code + 1] >> 2) << 5 | ((uint16_t) gct[3*code] >> 3) << 11); 
    // Store MSB first 
    frame_buffer[frame_buffer_ptr++] = (ret_color >> 8) & 0xff; 
    frame_buffer[frame_buffer_ptr++] = ret_color & 0xff;

    uint16_t prev_code = code;

    while(code != eoi)
    {
        code = get_code(code_mask, 
                       code_size, 
                       &code_chunk, 
                       &chunk_len,
                       lzw_data,
                       &data_ptr,
                       &chunks_read,
                       &chunk_len,
                       &num_bits); 
        if(code == eoi)
        {
                break;
        }
        if (code < code_table_head)
        {
        if (code == cc)
        {

            memset(&string_nodes[cc], LINK_END, (unsigned long)(4096 - cc)*sizeof(uint16_t));
            code_size = meta_data.lzw_smallest_code_size + 1;
            code_mask = (0x1 << code_size) - 1;
            code_table_head = (0x1 << meta_data.lzw_smallest_code_size) + 2;
            code = get_code(code_mask, 
                                 code_size, 
                                 &code_chunk, 
                                 &chunk_len,
                                 lzw_data,
                                 &data_ptr,
                                 &chunks_read,
                                 &chunk_len,
                                 &num_bits); 

            prev_code = code;
            max_new_items = 1 << code_size;
        }
        else
        {
            string_beginning[code_table_head] = string_beginning[prev_code];
            string_ends[code_table_head] = string_beginning[code];
            string_nodes[code_table_head] = prev_code;
            prev_code = code;
            code_table_head++;
            decode_pixels(code, gct);
        }
        }

        else if (code == code_table_head)
        {
            string_beginning[code_table_head] = string_beginning[prev_code];
            string_ends[code] = string_beginning[prev_code];
            string_nodes[code] = prev_code;
            prev_code = code;
            code_table_head++;
            decode_pixels(code, gct);
        }

        if (code_table_head == max_new_items && code_size < 12)
        {
            code_size++;
            max_new_items = 1 << code_size;
            code_mask = (0x1 << code_size) - 1;
        }


    }

    uint16_t gct_first_color = (gct[2] >> 3 | ((uint16_t)gct[1] >> 2) << 5 | ((uint16_t) gct[0] >> 3) << 11); 
    while (frame_buffer_ptr < (SCREEN_WIDTH - meta_data.x_offset) * (SCREEN_HEIGHT - meta_data.y_offset))
    {
        frame_buffer[frame_buffer_ptr++] = (gct_first_color >> 8) & 0xff;
        frame_buffer[frame_buffer_ptr++] = gct_first_color & 0xff;
    }
    frame_buffer_ptr = 0;

}

FrameMetadata parseGIFHeaders(uint32_t GCE_off)
{
   //skip first two bytes
   GCEHeader *gch = (GCEHeader*) &gifBytes[GCE_off + 2];
   //skip first byte
   ImageDescriptorHeader* idh = (ImageDescriptorHeader*) &gifBytes[GCE_off + 9];

   uint32_t color_table_offset;
   uint32_t data_offset = GCE_off + 19;

   if (idh->flags >> 7 & 0x1)
    {
        color_table_offset = GCE_off + 18;
        data_offset += 3*(1 << ((idh->flags & 0x7) + 1)) ;
    }
   else
        color_table_offset = 0xD;
        
   
   FrameMetadata return_frame = {
                                 .x_offset = idh->x_offset,
                                 .y_offset = idh->y_offset,
                                 .width = idh->height,
                                 .height = idh->width,
                                 .lzw_smallest_code_size = gifBytes[data_offset - 1],
                                 .data_offset = data_offset,
                                 .color_table_offset = color_table_offset
    };

    return return_frame;
                                  
}

