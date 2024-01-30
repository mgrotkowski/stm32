#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define MAX_CODE_SIZE 12
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define GCT_OFFSET 0xD
#define DATA_OFFSET 0x35e
//#define DATA_OFFSET 0xc0 //do not start_with LZW_SMALLEST_CODE_SIZE only chunk_len
#define LINK_UNUSED 5911
#define LINK_END 5912
#define LZW_SMALLEST_CODE_SIZE 6


uint16_t string_ends[1 << MAX_CODE_SIZE];
uint16_t string_beginning[1 << MAX_CODE_SIZE];
uint16_t string_nodes[1 << MAX_CODE_SIZE];

uint8_t file_buffer[3*SCREEN_WIDTH * SCREEN_HEIGHT];
uint16_t temp_str_buffer[1 << MAX_CODE_SIZE];

uint32_t file_buffer_ptr = 0;

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
        file_buffer[file_buffer_ptr++] = gct[3*code];
        file_buffer[file_buffer_ptr++] = gct[3*code + 1];
        file_buffer[file_buffer_ptr++] = gct[3*code + 2];
    }

}

int main(void)
{

    //int fd = open("cube-ezgif.com-speed.gif", O_RDONLY);
    int fd = open("star_wars-ezgif.com-resize.gif", O_RDONLY);
    struct stat file_stat;
    fstat(fd, &file_stat);

    uint8_t* file = (uint8_t*) mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    uint8_t* lzw_data = &file[DATA_OFFSET];
    uint8_t* gct = &file[GCT_OFFSET];
    
    uint32_t data_ptr = 0;
    uint8_t chunk_len = lzw_data[data_ptr++];
    uint8_t chunks_read = 0;
    uint8_t num_bits = 32;


    uint16_t cc = 1 << LZW_SMALLEST_CODE_SIZE;
    uint16_t eoi = cc + 1;
    uint8_t code_size = LZW_SMALLEST_CODE_SIZE + 1; 
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


    file_buffer[file_buffer_ptr++] = gct[3*code];
    file_buffer[file_buffer_ptr++] = gct[3*code+1];
    file_buffer[file_buffer_ptr++] = gct[3*code+2];

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
            code_size = LZW_SMALLEST_CODE_SIZE + 1;
            code_mask = (0x1 << code_size) - 1;
            code_table_head = (0x1 << LZW_SMALLEST_CODE_SIZE) + 2;
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

    while (file_buffer_ptr < sizeof(file_buffer))
    {
        file_buffer[file_buffer_ptr++] = gct[0];
        file_buffer[file_buffer_ptr++] = gct[1];
        file_buffer[file_buffer_ptr++] = gct[2];
    }

    int fd2 = open("C_bytes_output_star_wars.bin", O_CREAT | O_RDWR | O_TRUNC);
    write(fd2, file_buffer, sizeof(file_buffer));
}
