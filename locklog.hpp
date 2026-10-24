#pragma once

#include "locklog_headers.hpp"
#include "locklog_structure.hpp"

// constructor
inline Lock_logger::Lock_logger(const char* prelude_string, int64_t pointer_array_size, int64_t data_buffer_size, int64_t scratchpad_size){

    // we allocate and store our prelude if not null
    if(prelude_string != nullptr){

        // we fetch our prelude string length
        prelude_len = strlen(prelude_string);

        // we allocate the char array to store our prelude string - we add 1 to our prelude_string length to account for the null terminating byte
        prelude = new(std::nothrow) char[prelude_len + 1];

        // we check that we successfully allocated memory for our prelude
        if(prelude != nullptr){

            // we copy our prelude string to our prelude buffer
            strcpy(prelude, prelude_string);

        }
        else{

            // getting here our prelude array allocation failed
            strcpy(error_buffer, "Error allocating prelude array.");

            error = true;

        }

    }

    // only continue if no error
    if(!error){

        // we only update our pointer array size if it is > our current pointer array size and is a power of 2 else we fall back to our default pointer array size
        if(pointer_array_size > SIZE_OF_POINTER_ARRAY && ((pointer_array_size & (pointer_array_size - 1)) == 0ll)){

            // we update our SIZE OF POINTER ARRAY with the supplied pointer array size variable
            SIZE_OF_POINTER_ARRAY = pointer_array_size;

            // we update our data buffer by maintaining its 1:16 ration with our pointer array
            SIZE_OF_BUFFER = 16 * SIZE_OF_POINTER_ARRAY;

        }

        // we allocate our pointer array
        pointer_array = new(std::nothrow) pointer_struct[SIZE_OF_POINTER_ARRAY];

        // we check that we successfully allocated memory for our pointer array
        if(pointer_array == nullptr){

            // getting here our pointer array allocation failed
            strcpy(error_buffer, "Error allocating pointer array.");

            error = true;

        }

        // only continue if no error
        if(!error){

            // we update our data buffer size if it is > our current data buffer size and is a power of 2 else we fall back to our default buffer size
            if(data_buffer_size > SIZE_OF_BUFFER && ((data_buffer_size & (data_buffer_size - 1)) == 0ll)){

                SIZE_OF_BUFFER = data_buffer_size;

            }

            // we allocate our data buffer
            buffer = new(std::nothrow) char[SIZE_OF_BUFFER];

            // we check that we successfully allocated memory for our data buffer
            if(buffer == nullptr){

                // getting here our data buffer allocation failed
                strcpy(error_buffer, "Error allocating data buffer.");

                error = true;

            }

            // only continue if no error
            if(!error){

                // we update our scratchpad size if it is > our current scratchpad size
                if(scratchpad_size > SIZE_OF_SCRATCHPAD){

                    SIZE_OF_SCRATCHPAD = scratchpad_size;

                }

                // we check that the prelude len if an is < the scratchpad size else we set the error flag
                if(prelude != nullptr && prelude_len > SIZE_OF_SCRATCHPAD){

                    // getting here our prelude len is > our scratchpad size so we set our error flag
                    strcpy(error_buffer, "Prelude length larger scratchpad size.");

                    error = true;

                }

                // only continue if no error
                if(!error){                

                    // we allocate our scratch pad
                    scratchpad = new(std::nothrow) char[SIZE_OF_SCRATCHPAD];

                    // we check that we successfully allocated memory for our scratchpad
                    if(scratchpad == nullptr){

                        // getting here our scratchpad allocation failed
                        strcpy(error_buffer, "Error allocating scratchpad.");

                        error = true;

                    }

                    // only continue if no error
                    if(!error){

                        // we set our scratchpad cursor to point to the scratchpad
                        scratchpad_cursor = scratchpad;

                        // we set the end of scratchpad pointer to the end of our declared scratchpad
                        end_of_scratchpad = &scratchpad[SIZE_OF_SCRATCHPAD - 1];

                    }

                }

            }

        }

    }

}

// destructor
inline Lock_logger::~Lock_logger(){

    // we write to screen any unwritten data in the scratchpad if we have data other than the prelude in the scratchpad
    if(scratchpad_cursor - scratchpad > prelude_len) write_to_screen();

    // we delete our pointer array if allocated
    if(pointer_array != nullptr) delete [] pointer_array;

    // we delete our data buffer if allocated
    if(buffer != nullptr) delete [] buffer;

    // we delete our scratchpad if allocated
    if(scratchpad != nullptr) delete [] scratchpad;

    // we delete our prelude if allocated
    if(prelude != nullptr) delete [] prelude;

}

inline void Lock_logger::flush(){
// the flush function simply sets the atomic variable write_index to the value of the non atomic variable p_array_index so everything written since the last call to flush becomes visible to every other thread

    // we use the release memory order to ensure that all writes to the internal buffer and pointer array are visible to other threads before the write_index is updated
    write_index.store(p_array_index, std::memory_order_release);

}

inline void Lock_logger::write_to_screen(){

    if(!error){

        ssize_t data_to_print = scratchpad_cursor - scratchpad;

        // we set scratchpad cursor back to cursor so we can use it to traverse our data to print
        scratchpad_cursor = scratchpad;

        while(data_to_print > 0){

            ssize_t data_printed = write(STDOUT, scratchpad_cursor, data_to_print);

            if(data_printed < 0){

                // if our write was interrupted we simply continue
                if(errno == EINTR) continue;

                // getting here we got an error not caused by an interrupt so we increment our failed writes variable and break from our print loop
                failed_writes++;

                break;

            }

            // we increment our scratchpad cursor
            scratchpad_cursor += data_printed;

            // we decrement our data to print length
            data_to_print -= data_printed;

        }

        // now we set our scratchpad cursor back to our scratchpad for our next print
        scratchpad_cursor = scratchpad;

    }

}

inline void Lock_logger::print(){
// the print function simply writes anything written to the internal buffer that hasn't been printed yet to our scratchpad

    // only continue if no error
    if(!error){

        // we first get a local copy of our write_index variable
        int64_t loc_write_index = write_index.load(std::memory_order_acquire);

        // we get a local copy of our read index variable - we load our read index usint std::memory order relaxed because this is the same thread that writes to it
        int64_t loc_read_index = read_index.load(std::memory_order_relaxed);

        // now we format all available data into our scratchpad so we call write just minimally starting from our prelude

        // now we loop through our pointer array formatting the available data
        while(loc_read_index < loc_write_index){

            // we copy our prelude into our scratchpad if our prelude is not null and we are at the start of our scratchpad
            if(prelude != nullptr){

                if(scratchpad_cursor == scratchpad){

                    memcpy(scratchpad_cursor, prelude, prelude_len);

                    // we increment our scratchpad cursor by our prelude length
                    scratchpad_cursor += prelude_len;

                }

            }

            // we fetch our pointer read array index
            int64_t r_array_index = loc_read_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the available space in our scratchpad
            ssize_t available_space = static_cast<ssize_t>(end_of_scratchpad - scratchpad_cursor + 1);

            if(pointer_array[r_array_index].type == CHAR){
            // this is a char* so we check the length to see if we can copy this into our scratchpad without wrapping

                // we compute the length of this char*
                ssize_t len = strlen((char*)pointer_array[r_array_index].pointer);

                if(len <= available_space){

                    // getting here the scratchpad has enough free space to copy this char* into it
                    memcpy(scratchpad_cursor, pointer_array[r_array_index].pointer, len);

                    // we increment the scratchpad cursor by the char* length
                    scratchpad_cursor += len;

                }
                else{

                    // getting here we don't have enough space in our scratchpad to hold this char* so we print the content of our scratchpad to screen and continue

                    // our write to screen function resets our scratchpad cursor internally before returning so we don't need to reset our scratchpad cursor again
                    write_to_screen();

                    // now we check if the char* len is > our scratchpad size - prelude_len in which case we have to print the char* directly to screen as it can't be contained in the scratchpad
                    if(len > SIZE_OF_SCRATCHPAD - prelude_len){

                        // we cast our pointer array pointer into a char and store it in a local variable
                        char* loc_data = (char*)pointer_array[r_array_index].pointer;

                        while(len > 0){

                            ssize_t data_printed = write(STDOUT, loc_data, len);

                            if(data_printed < 0){

                                // if our write was interrupted we simply continue
                                if(errno == EINTR) continue;
                                
                                // getting here we got an error not caused by an interrupt so we increment our failed writes variable and break from our print loop
                                failed_writes++;

                                break;

                            }

                            // we increment our loc data pointer
                            loc_data += data_printed;

                            // we decrement our char* len
                            len -= data_printed;

                        }

                    }
                    else{

                        // getting here the char* len can fit into the scratchpad so we just continue
                        continue;

                    }

                }

                // now we update our data read varible with the data end value at this pointer location
                data_read.store(pointer_array[r_array_index].data_end, std::memory_order_release);

            }
            else if(pointer_array[r_array_index].type == INTEGER){
            // this is a int* so we perform the necessary type casting

                // we check if our MIN_NUM_SPACE is <= our available space
                if(MIN_NUM_SPACE <= available_space){

                    scratchpad_cursor = std::to_chars(scratchpad_cursor, end_of_scratchpad + 1, pointer_array[r_array_index].x_int).ptr;

                }
                else{

                    // getting here we have < our min num space in our scratchpad so we write to screen and continue
                    write_to_screen();

                    continue;

                }

            }
            else if(pointer_array[r_array_index].type == UNSIGNED_INTEGER){
            // this is a unsigned int* so we perform the necessary type casting

                // we check if our MIN_NUM_SPACE is <= our available space
                if(MIN_NUM_SPACE <= available_space){

                    scratchpad_cursor = std::to_chars(scratchpad_cursor, end_of_scratchpad + 1, pointer_array[r_array_index].x_uint).ptr;

                }
                else{

                    // getting here we have < our min num space in our scratchpad so we write to screen and continue
                    write_to_screen();

                    continue;

                }

            }
            else if(pointer_array[r_array_index].type == INT64){
            // this is a int64_t* so we perform the necessary type casting

                // we check if our MIN_NUM_SPACE is <= our available space
                if(MIN_NUM_SPACE <= available_space){

                    scratchpad_cursor = std::to_chars(scratchpad_cursor, end_of_scratchpad + 1, pointer_array[r_array_index].x_int64).ptr;

                }
                else{

                    // getting here we have < our min num space in our scratchpad so we write to screen and continue
                    write_to_screen();

                    continue;

                }

            }
            else if(pointer_array[r_array_index].type == UINT64){
            // this is a uint64_t* so we perform the necessary type casting

                // we check if our MIN_NUM_SPACE is <= our available space
                if(MIN_NUM_SPACE <= available_space){

                    scratchpad_cursor = std::to_chars(scratchpad_cursor, end_of_scratchpad + 1, pointer_array[r_array_index].x_uint64).ptr;

                }
                else{

                    // getting here we have < our min num space in our scratchpad so we write to screen and continue
                    write_to_screen();

                    continue;

                }

            }
            else if(pointer_array[r_array_index].type == DOUBLE){
            // this is a double* so we perform the necessary type casting

                // we check if our MIN_NUM_SPACE is <= our available space
                if(MIN_NUM_SPACE <= available_space){

                    scratchpad_cursor = std::to_chars(scratchpad_cursor, end_of_scratchpad + 1, pointer_array[r_array_index].x_double).ptr;

                }
                else{

                    // getting here we have < our min num space in our scratchpad so we write to screen and continue
                    write_to_screen();

                    continue;

                }

            }
            else if(pointer_array[r_array_index].type == ENDL){
            // this is an ENDL write so we add a new line to our scratchpad and call our write to screen function

                if(scratchpad_cursor <= end_of_scratchpad){

                    // we write a new line to the location pointed at by our scratchpad cursor
                    *scratchpad_cursor++ = '\n';

                    // now we call our write to screen function
                    write_to_screen();

                }
                else{

                    // getting here we don't have enough space in our scratchpad to add our new line character so we call our write to screen function and then call write system call directly on the new line character so we can print it without touching our scratchpad cursor
                    write_to_screen();

                    // we call write system call
                    ssize_t data_printed = 0;

                    do{ data_printed = write(STDOUT, "\n", 1); } while(data_printed < 0 && errno == EINTR);
                    if(data_printed < 0) failed_writes++;

                }

            }

            // we increment loc read index and store it in our atomic read index variable with std::memory_order_release so it is visible to our write thread
            read_index.store(++loc_read_index, std::memory_order_release);

        }

        // we print our data if our scratchpad is full
        if(scratchpad_cursor > end_of_scratchpad){

            // we write our scratchpad data to screen
            write_to_screen();

        }

    }

}

inline Lock_logger& Lock_logger::print_shift(const char* data){
// print_shift function for printshifting strings

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // we check if the supplied parameter is a nullptr and return if it is
            if(data == nullptr) return *this;

            // getting here we have free space in our pointer array so we compute the length of this string
            int64_t len = strlen(data);

            // we compute the used space in our data buffer
            int64_t used_space = data_write - data_read.load(std::memory_order_acquire);

            // if the free space (data array size - used space) is < len + 1 we flush data if unflushed, drop this data, increment our dropped data and return - we add 1 to the string length to account for the null byte
            if(SIZE_OF_BUFFER - used_space < len + 1){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the required space for this string - we compute this differently from the value the free space is compared to above because if our required space isn't contiguous we would update it in place
            int64_t required_space = len + 1;

            // we fetch our write array index into the data buffer which is our unmasked data_write
            int64_t w_buffer_index = data_write & (SIZE_OF_BUFFER - 1);

            // we now check if the contiguous space from our write buffer index to the end of our data buffer is sufficient to hold this char* string - if it isn't we add the contiguous space to our required space and set our buffer index to 0
            if(w_buffer_index + required_space > SIZE_OF_BUFFER){

                required_space += SIZE_OF_BUFFER - w_buffer_index;
                w_buffer_index = 0ll;

            }

            // now we check if our used space + required space > SIZE_OF_BUFFER, if it is then we don't have sufficient contiguous space for this char* string so we we flush data if unflushed, drop the data, increment our dropped data counter and return
            if(used_space + required_space > SIZE_OF_BUFFER){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // getting here we can enter this data into our buffer

            // we compute our destination pointer from our write buffer index
            char* p_dest = buffer + w_buffer_index;

            // we store the location and type of this entry in the pointer array
            pointer_array[w_array_index].type = CHAR;

            pointer_array[w_array_index].pointer = p_dest;

            // we copy our data to the internal buffer
            memcpy(p_dest, data, len);

            // we increment our p_dest pointer by our data len so we can null terminate our data
            p_dest += len;

            // we null terminate our data
            *p_dest++ = '\0';

            // we now update our data write counter by adding our required space to it
            data_write += required_space;

            // we set our data end variable in our pointer array entry to our updated data_write variable
            pointer_array[w_array_index].data_end = data_write;

            // finally we increment our p_array_index variable
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }

    }

    return *this;

}

inline Lock_logger& Lock_logger::print_shift(const std::string_view data){
// print_shift function for printshifting std::string_view objects

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array so we length the length of this string_view
            int64_t len = data.size();

            // we compute the used space in our data buffer
            int64_t used_space = data_write - data_read.load(std::memory_order_acquire);

            // if the free space (data array size - used space) is < len + 1 we flush data if unflushed, drop this data, increment our dropped data and return - we add 1 to the string length to account for the null byte
            if(SIZE_OF_BUFFER - used_space < len + 1){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the required space for this string - we compute this differently from the value the free space is compared to above because if our required space isn't contiguous we would update it in place
            int64_t required_space = len + 1;

            // we fetch our write array index into the data buffer which is our unmasked data_write
            int64_t w_buffer_index = data_write & (SIZE_OF_BUFFER - 1);

            // we now check if the contiguous space from our write buffer index to the end of our data buffer is sufficient to hold this char* string - if it isn't we add the contiguous space to our required space and set our buffer index to 0
            if(w_buffer_index + required_space > SIZE_OF_BUFFER){

                required_space += SIZE_OF_BUFFER - w_buffer_index;
                w_buffer_index = 0ll;

            }

            // now we check if our used space + required space > SIZE_OF_BUFFER, if it is then we don't have sufficient contiguous space for this char* string so we we flush data if unflushed, drop the data, increment our dropped data counter and return
            if(used_space + required_space > SIZE_OF_BUFFER){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // getting here we can enter this data into our buffer

            // we compute our destination pointer from our write buffer index
            char* p_dest = buffer + w_buffer_index;

            // we store the location and type of this entry in the pointer array
            pointer_array[w_array_index].type = CHAR;

            pointer_array[w_array_index].pointer = p_dest;

            // we copy our data to the internal buffer
            memcpy(p_dest, data.data(), len);

            // we increment our p_dest pointer by our data len so we can null terminate our data
            p_dest += len;

            // we null terminate our data
            *p_dest++ = '\0';

            // we now update our data write counter by adding our required space to it
            data_write += required_space;

            // we set our data end variable in our pointer array entry to our updated data_write variable
            pointer_array[w_array_index].data_end = data_write;

            // finally we increment our p_array_index variable
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }

    }

    return *this;

}

inline Lock_logger& Lock_logger::print_shift(const std::string& data){
// print_shift function for printshifting std::string objects

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array so we length the length of this string_view
            int64_t len = data.size();

            // we compute the used space in our data buffer
            int64_t used_space = data_write - data_read.load(std::memory_order_acquire);

            // if the free space (data array size - used space) is < len + 1 we flush data if unflushed, drop this data, increment our dropped data and return - we add 1 to the string length to account for the null byte
            if(SIZE_OF_BUFFER - used_space < len + 1){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the required space for this string - we compute this differently from the value the free space is compared to above because if our required space isn't contiguous we would update it in place
            int64_t required_space = len + 1;

            // we fetch our write array index into the data buffer which is our unmasked data_write
            int64_t w_buffer_index = data_write & (SIZE_OF_BUFFER - 1);

            // we now check if the contiguous space from our write buffer index to the end of our data buffer is sufficient to hold this char* string - if it isn't we add the contiguous space to our required space and set our buffer index to 0
            if(w_buffer_index + required_space > SIZE_OF_BUFFER){

                required_space += SIZE_OF_BUFFER - w_buffer_index;
                w_buffer_index = 0ll;

            }

            // now we check if our used space + required space > SIZE_OF_BUFFER, if it is then we don't have sufficient contiguous space for this char* string so we we flush data if unflushed, drop the data, increment our dropped data counter and return
            if(used_space + required_space > SIZE_OF_BUFFER){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // getting here we can enter this data into our buffer

            // we compute our destination pointer from our write buffer index
            char* p_dest = buffer + w_buffer_index;

            // we store the location and type of this entry in the pointer array
            pointer_array[w_array_index].type = CHAR;

            pointer_array[w_array_index].pointer = p_dest;

            // we copy our data to the internal buffer
            memcpy(p_dest, data.data(), len);

            // we increment our p_dest pointer by our data len so we can null terminate our data
            p_dest += len;

            // we null terminate our data
            *p_dest++ = '\0';

            // we now update our data write counter by adding our required space to it
            data_write += required_space;

            // we set our data end variable in our pointer array entry to our updated data_write variable
            pointer_array[w_array_index].data_end = data_write;

            // finally we increment our p_array_index variable
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }

    }

    return *this;

}

inline Lock_logger& Lock_logger::print_shift(int x){
// function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = INTEGER;

            pointer_array[w_array_index].x_int = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::print_shift(unsigned int x){
// function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = UNSIGNED_INTEGER;

            pointer_array[w_array_index].x_uint = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::print_shift(int64_t x){
// function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = INT64;

            pointer_array[w_array_index].x_int64 = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::print_shift(uint64_t x){
// function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = UINT64;

            pointer_array[w_array_index].x_uint64 = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::print_shift(double x){
// function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = DOUBLE;

            pointer_array[w_array_index].x_double = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::print_shift(EndlTag endl_tag){

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = ENDL;

            // for the ENDL type we flush any data written to the pointer array so they become visible to the print thread - the endl type does not have a corresponding value to be written to the pointer array
            flush();

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::operator<<(const char* data){
// operator<< function for printshifting strings

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // we check if the supplied parameter is a nullptr and return if it is
            if(data == nullptr) return *this;

            // getting here we have free space in our pointer array so we compute the length of this string
            int64_t len = strlen(data);

            // we compute the used space in our data buffer
            int64_t used_space = data_write - data_read.load(std::memory_order_acquire);

            // if the free space (data array size - used space) is < len + 1 we flush data if unflushed, drop this data, increment our dropped data and return - we add 1 to the string length to account for the null byte
            if(SIZE_OF_BUFFER - used_space < len + 1){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the required space for this string - we compute this differently from the value the free space is compared to above because if our required space isn't contiguous we would update it in place
            int64_t required_space = len + 1;

            // we fetch our write array index into the data buffer which is our unmasked data_write
            int64_t w_buffer_index = data_write & (SIZE_OF_BUFFER - 1);

            // we now check if the contiguous space from our write buffer index to the end of our data buffer is sufficient to hold this char* string - if it isn't we add the contiguous space to our required space and set our buffer index to 0
            if(w_buffer_index + required_space > SIZE_OF_BUFFER){

                required_space += SIZE_OF_BUFFER - w_buffer_index;
                w_buffer_index = 0ll;

            }

            // now we check if our used space + required space > SIZE_OF_BUFFER, if it is then we don't have sufficient contiguous space for this char* string so we we flush data if unflushed, drop the data, increment our dropped data counter and return
            if(used_space + required_space > SIZE_OF_BUFFER){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // getting here we can enter this data into our buffer

            // we compute our destination pointer from our write buffer index
            char* p_dest = buffer + w_buffer_index;

            // we store the location and type of this entry in the pointer array
            pointer_array[w_array_index].type = CHAR;

            pointer_array[w_array_index].pointer = p_dest;

            // we copy our data to the internal buffer
            memcpy(p_dest, data, len);

            // we increment our p_dest pointer by our data len so we can null terminate our data
            p_dest += len;

            // we null terminate our data
            *p_dest++ = '\0';

            // we now update our data write counter by adding our required space to it
            data_write += required_space;

            // we set our data end variable in our pointer array entry to our updated data_write variable
            pointer_array[w_array_index].data_end = data_write;

            // finally we increment our p_array_index variable
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }

    }

    return *this;

}

inline Lock_logger& Lock_logger::operator<<(const std::string_view data){
// operator<< function for printshifting std::string_view objects

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array so we length the length of this string_view
            int64_t len = data.size();

            // we compute the used space in our data buffer
            int64_t used_space = data_write - data_read.load(std::memory_order_acquire);

            // if the free space (data array size - used space) is < len + 1 we flush data if unflushed, drop this data, increment our dropped data and return - we add 1 to the string length to account for the null byte
            if(SIZE_OF_BUFFER - used_space < len + 1){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the required space for this string - we compute this differently from the value the free space is compared to above because if our required space isn't contiguous we would update it in place
            int64_t required_space = len + 1;

            // we fetch our write array index into the data buffer which is our unmasked data_write
            int64_t w_buffer_index = data_write & (SIZE_OF_BUFFER - 1);

            // we now check if the contiguous space from our write buffer index to the end of our data buffer is sufficient to hold this char* string - if it isn't we add the contiguous space to our required space and set our buffer index to 0
            if(w_buffer_index + required_space > SIZE_OF_BUFFER){

                required_space += SIZE_OF_BUFFER - w_buffer_index;
                w_buffer_index = 0ll;

            }

            // now we check if our used space + required space > SIZE_OF_BUFFER, if it is then we don't have sufficient contiguous space for this char* string so we we flush data if unflushed, drop the data, increment our dropped data counter and return
            if(used_space + required_space > SIZE_OF_BUFFER){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // getting here we can enter this data into our buffer

            // we compute our destination pointer from our write buffer index
            char* p_dest = buffer + w_buffer_index;

            // we store the location and type of this entry in the pointer array
            pointer_array[w_array_index].type = CHAR;

            pointer_array[w_array_index].pointer = p_dest;

            // we copy our data to the internal buffer
            memcpy(p_dest, data.data(), len);

            // we increment our p_dest pointer by our data len so we can null terminate our data
            p_dest += len;

            // we null terminate our data
            *p_dest++ = '\0';

            // we now update our data write counter by adding our required space to it
            data_write += required_space;

            // we set our data end variable in our pointer array entry to our updated data_write variable
            pointer_array[w_array_index].data_end = data_write;

            // finally we increment our p_array_index variable
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }

    }

    return *this;

}

inline Lock_logger& Lock_logger::operator<<(const std::string& data){
// operator<< function for printshifting std::string objects

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array so we length the length of this string_view
            int64_t len = data.size();

            // we compute the used space in our data buffer
            int64_t used_space = data_write - data_read.load(std::memory_order_acquire);

            // if the free space (data array size - used space) is < len + 1 we flush data if unflushed, drop this data, increment our dropped data and return - we add 1 to the string length to account for the null byte
            if(SIZE_OF_BUFFER - used_space < len + 1){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we compute the required space for this string - we compute this differently from the value the free space is compared to above because if our required space isn't contiguous we would update it in place
            int64_t required_space = len + 1;

            // we fetch our write array index into the data buffer which is our unmasked data_write
            int64_t w_buffer_index = data_write & (SIZE_OF_BUFFER - 1);

            // we now check if the contiguous space from our write buffer index to the end of our data buffer is sufficient to hold this char* string - if it isn't we add the contiguous space to our required space and set our buffer index to 0
            if(w_buffer_index + required_space > SIZE_OF_BUFFER){

                required_space += SIZE_OF_BUFFER - w_buffer_index;
                w_buffer_index = 0ll;

            }

            // now we check if our used space + required space > SIZE_OF_BUFFER, if it is then we don't have sufficient contiguous space for this char* string so we we flush data if unflushed, drop the data, increment our dropped data counter and return
            if(used_space + required_space > SIZE_OF_BUFFER){

                // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
                if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

                dropped_data++;
                return *this;

            }

            // getting here we can enter this data into our buffer

            // we compute our destination pointer from our write buffer index
            char* p_dest = buffer + w_buffer_index;

            // we store the location and type of this entry in the pointer array
            pointer_array[w_array_index].type = CHAR;

            pointer_array[w_array_index].pointer = p_dest;

            // we copy our data to the internal buffer
            memcpy(p_dest, data.data(), len);

            // we increment our p_dest pointer by our data len so we can null terminate our data
            p_dest += len;

            // we null terminate our data
            *p_dest++ = '\0';

            // we now update our data write counter by adding our required space to it
            data_write += required_space;

            // we set our data end variable in our pointer array entry to our updated data_write variable
            pointer_array[w_array_index].data_end = data_write;

            // finally we increment our p_array_index variable
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }

    }

    return *this;

}

inline Lock_logger& Lock_logger::operator<<(int x){
// operator<< function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = INTEGER;

            pointer_array[w_array_index].x_int = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::operator<<(unsigned int x){
// operator<< function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = UNSIGNED_INTEGER;

            pointer_array[w_array_index].x_uint = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::operator<<(int64_t x){
// operator<< function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = INT64;

            pointer_array[w_array_index].x_int64 = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::operator<<(uint64_t x){
// operator<< function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = UINT64;

            pointer_array[w_array_index].x_uint64 = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::operator<<(double x){
// operator<< function for printshifting integers

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = DOUBLE;

            pointer_array[w_array_index].x_double = x;

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}

inline Lock_logger& Lock_logger::operator<<(EndlTag endl_tag){

    // only continue if no error
    if(!error){

        // we check if we still have free space in our pointer array - since our p_array_index and read index are unmasked counters we simply check the difference between them
        if(p_array_index - read_index.load(std::memory_order_acquire) < SIZE_OF_POINTER_ARRAY){

            // getting here we have free space in our pointer array

            // we unmask our pointer array index
            int64_t w_array_index = p_array_index & (SIZE_OF_POINTER_ARRAY - 1);

            // we store the type of this entry and the data value
            pointer_array[w_array_index].type = ENDL;

            // for the ENDL type we flush any data written to the pointer array so they become visible to the print thread - the endl type does not have a corresponding value to be written to the pointer array
            flush();

            // we increment our p_array_index
            p_array_index++;

        }
        else{

            // getting here our pointer array is full so we first check if the data in the pointer array has been flushed if it hasn't then we flush it to make it visible to the print thread, we then ignore this data and increment our dropped data counter

            // we load our write index with std::memory_order_relaxed because it is written to by the same thread calling the print shift function
            if(p_array_index != write_index.load(std::memory_order_relaxed)) flush();

            // we increment our dropped data counter
            dropped_data++;

        }
        
    }
    
    return *this;

}
