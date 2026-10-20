#pragma once

#include "printer_headers.hpp"
#include "printer_structure.hpp"

// constructor
Printer::Printer(char* prelude_string, int64_t pointer_array_size, int64_t data_buffer_size, int64_t scratchpad_size){

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

            // we update our data buffer size if it is > our current data buffer size
            if(data_buffer_size > SIZE_OF_BUFFER){

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

                // we update our buffer cursor
                buffer_cursor = buffer;

                // we update our end of buffer variable
                end_of_buffer = &buffer[SIZE_OF_BUFFER - 1];

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
Printer::~Printer(){

    // we delete our pointer array if allocated
    if(pointer_array != nullptr) delete [] pointer_array;

    // we delete our data buffer if allocated
    if(buffer != nullptr) delete [] buffer;

    // we delete our scratchpad if allocated
    if(scratchpad != nullptr) delete [] scratchpad;

    // we delete our prelude if allocated
    if(prelude != nullptr) delete [] prelude;

}

void Printer::flush(){
// the flush function simply sets the atomic variable write_index to the value of the non atomic variable p_array_index so everything written since the last call to flush becomes visible to every other thread

    // we use the release memory order to ensure that all writes to the internal buffer and pointer array are visible to other threads before the write_index is updated
    write_index.store(p_array_index, std::memory_order_release);

}

void Printer::write_to_screen(){

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

void Printer::print(){
// the print function simply writes anything written to the internal buffer that hasn't been printed yet to our scratchpad

    // only continue if no error
    if(!error){

        // we first get a local write_index variable
        int64_t loc_write_index = write_index.load(std::memory_order_acquire);

        // now we format all available data into our scratchpad so we call write just minimally starting from our prelude

        // we copy our prelude into our scratchpad if our prelude is not null and we are at the start of our scratchpad
        if(prelude != nullptr){

            if(scratchpad_cursor == scratchpad){

                memcpy(scratchpad_cursor, prelude, prelude_len);

                // we increment our scratchpad cursor by our prelude length
                scratchpad_cursor += prelude_len;

            }

        }

        // now we loop through our pointer array formatting the available data
        while(read_index < loc_write_index){

            // we fetch our pointer read array index
            int64_t r_array_index = read_index & (SIZE_OF_POINTER_ARRAY - 1);

            if(pointer_array[r_array_index].type == CHAR){
            // this is a char* so we check the length to see if we can copy this into our scratchpad without wrapping

                // we compute the length of this char*
                ssize_t len = strlen((char*)pointer_array[r_array_index].pointer);

                if(len <= static_cast<ssize_t>(end_of_scratchpad - scratchpad_cursor + 1)){

                    // getting here the scratchpad has enough free space to copy this char* into it
                    memcpy(scratchpad_cursor, pointer_array[r_array_index].pointer, len);

                    // we increment the scratchpad cursor by the char* length
                    scratchpad_cursor += len;

                }
                else{

                    // getting here we don't have enough space in our scratchpad to hold this char* so we print the content of our scratchpad to screen and continue

                    // our write to screen function resets our scratchpad cursor internally before returning so we don't need to reset our scratchpad cursor again
                    write_to_screen();

                    // now we check if the char* len is > our scratchpad size in which case we have to print the char* directly to screen as it can't be contained in the scratchpad
                    if(len > SIZE_OF_SCRATCHPAD){

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

            }
            else if(pointer_array[r_array_index].type == INTEGER){
            // this is a int* so we perform the necessary type casting

                std::cout<<*(int*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[r_array_index].type == UNSIGNED_INTEGER){
            // this is a unsigned int* so we perform the necessary type casting

                std::cout<<*(unsigned int*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[r_array_index].type == INT64){
            // this is a int64_t* so we perform the necessary type casting

                std::cout<<*(int64_t*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[r_array_index].type == UINT64){
            // this is a uint64_t* so we perform the necessary type casting

                std::cout<<*(uint64_t*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[r_array_index].type == DOUBLE){
            // this is a double* so we perform the necessary type casting

                std::cout<<*(double*)pointer_array[read_index].pointer;

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

            // we increment read index
            read_index++;

        }

        // we print our data if our scratchpad is full
        if(scratchpad_cursor >= end_of_scratchpad){

            // we write our scratchpad data to screen
            write_to_screen();

        }

    }

}

Printer& Printer::print_shift(const char* data, int length_of_data){
// print_shift function for printshifting strings

    // we first check if the string length can be contained in the available space before the end of the internal buffer

    if(length_of_data + 1 < end_of_buffer - (char*)buffer_cursor){
    // getting here the data can be contained in the free available space before the end of the buffer

        // copy the data to the internal buffer
        memcpy(buffer_cursor, data, length_of_data);

        // we store the location and type of this entry in the pointer array
        pointer_array[p_array_index].type = CHAR;

        pointer_array[p_array_index].pointer = buffer_cursor;

        // we move the cursor to point to the next free location
        buffer_cursor = (char*)buffer_cursor + length_of_data;

        // we store the null byte at this location
        *(char*)buffer_cursor = '\0';

        // we move the cursor to point to the next free location
        buffer_cursor = (char*)buffer_cursor + sizeof(char);

        // we now increment p_array_index
        if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
        // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
            wrap = true;

            // we wrap the p_array_index around the pointer array
            p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

            // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new value
            flush();

        }
        else
            // we increment our p_array_index
            p_array_index++;

    }
    else if(length_of_data + 1 < SIZE_OF_BUFFER){
    // getting here the data cannot be contained in the remaining available space in the internal buffer but the length of data is less than the size of the internal buffer

        // first we flush whatever data has been written to the internal buffer but not flushed yet
        flush();

        // now we set the buffer_cursor to the start location of the internal buffer
        buffer_cursor = buffer;

        // copy the data to the internal buffer
        memcpy(buffer_cursor, data, length_of_data);

        // we store the location and type of this entry in the pointer array
        pointer_array[p_array_index].type = CHAR;

        pointer_array[p_array_index].pointer = buffer_cursor;

        // we move the cursor to point to the next free location
        buffer_cursor = (char*)buffer_cursor + length_of_data;

        // we store the null byte at this location
        *(char*)buffer_cursor = '\0';

        // we move the cursor to point to the next free location
        buffer_cursor = (char*)buffer_cursor + sizeof(char);

        // we now increment p_array_index
        if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
        // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
            wrap = true;
                    
            // we wrap the p_array_index around the pointer array
            p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

            // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new value
            flush();

        }
        else
            // we increment our p_array_index
            p_array_index++;


    }
    else{
    // getting here the length of data is larger than the size of the entire internal buffer so we write to the internal buffer in batches calling flush after every write

        // first we flush whatever data has been written to the internal buffer but not flushed yet
        flush();

        // now we set the buffer_cursor to the start location of the internal buffer
        buffer_cursor = buffer;

        // we null terminate the end of the buffer before going into the loop
        buffer[SIZE_OF_BUFFER - 1] = '\0';

        // we use a local variable to keep track of how much data we have left to print
        int length_of_remaining_data = length_of_data;

        while(SIZE_OF_BUFFER < length_of_remaining_data + 1){
        // we keep looping till the length of data remaining can fit completely into the data buffer

            // copy a batch of data to the internal buffer
            memcpy(buffer_cursor, data, SIZE_OF_BUFFER - 1); // we leave a space for the null byte so we do not overwrite it

            // we store the location and type of this entry in the pointer array
            pointer_array[p_array_index].type = CHAR;

            pointer_array[p_array_index].pointer = buffer_cursor;

            // here buffer_cursor would still be pointing to the start location of the internal buffer

            // we now increment p_array_index
            if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
            // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
                wrap = true;
                        
                // we wrap the p_array_index around the pointer array
                p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

                // we don't call flush here since flush is called after every write to the internal buffer

            }
            else
                // we increment our p_array_index
                p_array_index++;

            // we flush the written data
            flush();

            // we update the length of remaining data
            length_of_remaining_data -= SIZE_OF_BUFFER - 1;

        }

        // getting here the remaining data can now fit into the internal buffer

        // buffer_cursor would still be pointing to the start of the internal buffer

        // we first check that the length of the remaining data is > 0
        if(length_of_remaining_data > 0){

            // copy the data to the internal buffer
            memcpy(buffer_cursor, data, length_of_remaining_data);

            // we store the location and type of this entry in the pointer array
            pointer_array[p_array_index].type = CHAR;

            pointer_array[p_array_index].pointer = buffer_cursor;

            // we move the cursor to point to the next free location
            buffer_cursor = (char*)buffer_cursor + length_of_data;

            // we store the null byte at this location
            *(char*)buffer_cursor = '\0';

            // we move the cursor to point to the next free location
            buffer_cursor = (char*)buffer_cursor + sizeof(char);

            // we now increment p_array_index
            if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
            // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
                wrap = true;
                        
                // we wrap the p_array_index around the pointer array
                p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

                // we don't call flush here because flush is called after the write to the internal buffer

            }
            else
                // we increment our p_array_index
                p_array_index++;

            // we flush the written data
            flush();

        }

    }

    return *this;

}

Printer& Printer::print_shift(int x){
// function for printshifting integers

    size_t free_space = end_of_buffer - (char*)buffer_cursor;

    // we attempt to align the buffer cursor
    
    // we check if the returned value is null which would mean that the size of the remaining space in the internal buffer is not enough to hold an int
    if(std::align(alignof(int), sizeof(int), buffer_cursor, free_space) == nullptr){
    // buffer_cursor is null so we set buffer_cursor to the start of the internal buffer and call align again

        // we first flush any unprinted data
        flush();

        // we store the size of the internal buffer in the free space variable
        free_space = (size_t)SIZE_OF_BUFFER;

        buffer_cursor = buffer;

        // we align the buffer cursor
        std::align(alignof(int), sizeof(int), buffer_cursor, free_space);

        // now align is bound to succeed

    }

    // store the integer in the internal buffer
    *(int*)buffer_cursor = x;

    // we store the location and type of this entry in the pointer array
    pointer_array[p_array_index].type = INTEGER;

    pointer_array[p_array_index].pointer = buffer_cursor;

    // we move the cursor to point to the next free location
    buffer_cursor = (char*)buffer_cursor + sizeof(int);

    // we now increment p_array_index
    if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
    // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
        wrap = true;

        // we wrap the p_array_index around the pointer array
        p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

        // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new value
        flush();

    }
    else
        // we increment our p_array_index
        p_array_index++;


    return *this;

}

Printer& Printer::print_shift(unsigned int x){
// function for printshifting integers

    size_t free_space = end_of_buffer - (char*)buffer_cursor;

    // we attempt to align the buffer cursor
    
    // we check if the returned value is null which would mean that the size of the remaining space in the internal buffer is not enough to hold an unsigned int
    if(std::align(alignof(unsigned int), sizeof(unsigned int), buffer_cursor, free_space) == nullptr){
    // buffer_cursor is null so we set buffer_cursor to the start of the internal buffer and call align again

        // we first flush any unprinted data
        flush();

        // we store the size of the internal buffer in the free space variable
        free_space = (size_t)SIZE_OF_BUFFER;

        buffer_cursor = buffer;

        // we align the buffer cursor
        std::align(alignof(unsigned int), sizeof(unsigned int), buffer_cursor, free_space);

        // now align is bound to succeed

    }

    // store the unsigned integer in the internal buffer
    *(unsigned int*)buffer_cursor = x;

    // we store the location and type of this entry in the pointer array
    pointer_array[p_array_index].type = UNSIGNED_INTEGER;

    pointer_array[p_array_index].pointer = buffer_cursor;

    // we move the cursor to point to the next free location
    buffer_cursor = (char*)buffer_cursor + sizeof(unsigned int);

    // we now increment p_array_index
    if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
    // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
        wrap = true;
                
        // we wrap the p_array_index around the pointer array
        p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

        // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new value
        flush();

    }
    else
        // we increment our p_array_index
        p_array_index++;


    return *this;

}

Printer& Printer::print_shift(int64_t x){
// function for printshifting integers

    size_t free_space = end_of_buffer - (char*)buffer_cursor;

    // we attempt to align the buffer cursor
    
    // we check if the returned value is null which would mean that the size of the remaining space in the internal buffer is not enough to hold an int64_t
    if(std::align(alignof(int64_t), sizeof(int64_t), buffer_cursor, free_space) == nullptr){
    // buffer_cursor is null so we set buffer_cursor to the start of the internal buffer and call align again

        // we first flush any unprinted data
        flush();

        // we store the size of the internal buffer in the free space variable
        free_space = (size_t)SIZE_OF_BUFFER;

        buffer_cursor = buffer;

        // we align the buffer cursor
        std::align(alignof(int64_t), sizeof(int64_t), buffer_cursor, free_space);

        // now align is bound to succeed

    }

    // store the int64_t in the internal buffer
    *(int64_t*)buffer_cursor = x;

    // we store the location and type of this entry in the pointer array
    pointer_array[p_array_index].type = INT64;

    pointer_array[p_array_index].pointer = buffer_cursor;

    // we move the cursor to point to the next free location
    buffer_cursor = (char*)buffer_cursor + sizeof(int64_t);

    // we now increment p_array_index
    if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
    // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
        wrap = true;
                
        // we wrap the p_array_index around the pointer array
        p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

        // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new status
        flush();

    }
    else
        // we increment our p_array_index
        p_array_index++;


    return *this;

}

Printer& Printer::print_shift(uint64_t x){
// function for printshifting integers

    size_t free_space = end_of_buffer - (char*)buffer_cursor;

    // we attempt to align the buffer cursor
    
    // we check if the returned value is null which would mean that the size of the remaining space in the internal buffer is not enough to hold an uint64_t
    if(std::align(alignof(uint64_t), sizeof(uint64_t), buffer_cursor, free_space) == nullptr){
    // buffer_cursor is null so we set buffer_cursor to the start of the internal buffer and call align again

        // we first flush any unprinted data
        flush();

        // we store the size of the internal buffer in the free space variable
        free_space = (size_t)SIZE_OF_BUFFER;

        buffer_cursor = buffer;

        // we align the buffer cursor
        std::align(alignof(uint64_t), sizeof(uint64_t), buffer_cursor, free_space);

        // now align is bound to succeed

    }

    // store the uint64_t in the internal buffer
    *(uint64_t*)buffer_cursor = x;

    // we store the location and type of this entry in the pointer array
    pointer_array[p_array_index].type = UINT64;

    pointer_array[p_array_index].pointer = buffer_cursor;

    // we move the cursor to point to the next free location
    buffer_cursor = (char*)buffer_cursor + sizeof(uint64_t);

    // we now increment p_array_index
    if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
    // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
        wrap = true;
                
        // we wrap the p_array_index around the pointer array
        p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

        // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new status
        flush();

    }
    else
        // we increment our p_array_index
        p_array_index++;


    return *this;

}

Printer& Printer::print_shift(double x){
// function for printshifting integers

    size_t free_space = end_of_buffer - (char*)buffer_cursor;

    // we attempt to align the buffer cursor
    
    // we check if the returned value is null which would mean that the size of the remaining space in the internal buffer is not enough to hold an double
    if(std::align(alignof(double), sizeof(double), buffer_cursor, free_space) == nullptr){
    // buffer_cursor is null so we set buffer_cursor to the start of the internal buffer and call align again

        // we first flush any unprinted data
        flush();

        // we store the size of the internal buffer in the free space variable
        free_space = (size_t)SIZE_OF_BUFFER;

        buffer_cursor = buffer;

        // we align the buffer cursor
        std::align(alignof(double), sizeof(double), buffer_cursor, free_space);

        // now align is bound to succeed

    }

    // store the double in the internal buffer
    *(double*)buffer_cursor = x;

    // we store the location and type of this entry in the pointer array
    pointer_array[p_array_index].type = DOUBLE;

    pointer_array[p_array_index].pointer = buffer_cursor;

    // we move the cursor to point to the next free location
    buffer_cursor = (char*)buffer_cursor + sizeof(double);

    // we now increment p_array_index
    if( !((p_array_index + 1) < SIZE_OF_POINTER_ARRAY) ){
    // we test whether adding 1 to the current pointer array index would cause it to wrap around the pointer array, if so we set the wrap variable to true
        wrap = true;
                
        // we wrap the p_array_index around the pointer array
        p_array_index = (p_array_index + 1) % SIZE_OF_POINTER_ARRAY;

        // we flush now so that everything written since the last flush would become visible to the printing thread after the wrap around including the wrap variable's new status
        flush();

    }
    else
        // we increment our p_array_index
        p_array_index++;

    
    return *this;

}
