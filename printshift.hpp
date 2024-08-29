#include "printshiftheaders.hpp"

// we now define the printing class
class printer{

private:

    // define the constants that define the pointer type
    inline static const int CHAR = 0;
    inline static const int INTEGER = 1;
    inline static const int UNSIGNED_INTEGER = 2;
    inline static const int INT64 = 3;
    inline static const int UINT64 = 4;
    inline static const int DOUBLE = 5;

private:

    // we define the pointer struct
    struct pointer_struct{

        int type;
        void* pointer;

    };

private:

    inline static const int SIZE_OF_BUFFER = 64 * 1024; // this defines the length of the internal char buffer used for print shifting
    inline static const int SIZE_OF_POINTER_ARRAY = SIZE_OF_BUFFER/2; // this defines the length of the pointer array - we set it to SIZE_OF_BUFFER/2 so that it is at least long enough to hold the entire contents of the internal buffer without wrapping around, regardless of the data stored in the internal buffer since the smallest discrete data that can be stored in the internal buffer is a char, which is internally terminated with a null byte the highest number of independent data that can be stored in the pointer array is SIZE_OF_BUFFER/2 chars
    pointer_struct pointer_array[SIZE_OF_POINTER_ARRAY]; // the pointer_struct array
    char buffer[SIZE_OF_BUFFER]; // internal buffer used for print shifting

private: 

    std::atomic<int> write_index = 0; // this variable when updated holds the index of the pointer_array where the next write should occur, it is made atomic so once it is updated the change is visible to all threads
    void* buffer_cursor = buffer; // this cursor is used to indicate the next write location in the internal buffer
    char* end_of_buffer = &buffer[SIZE_OF_BUFFER - 1]; // this pointer holds the address of the last location in the internal buffer
    bool wrap = false; // this variable is used to indicate that print shifting has gotten to the end of the ponter array and wrapped around to the start of the array
    int p_array_index = 0; // this variable indicates the index in the pointer_array where the next entry should be entered
    int read_index = 0; // this variable holds the pointer_array index where the next read should be read from

public:

    void flush(){
    // the flush function simply sets the atomic variable write_index to the value of the non atomic variable p_array_index so everything written since the last call to flush becomes visible to every other thread

        // store the new write index
        write_index = p_array_index;

    }

    void print(){
    // the print function simply prints anything written to the internal buffer that hasn't been printed yet

        // we first get a local write_index variable
        int loc_write_index = write_index;

        while((read_index < loc_write_index) || wrap){

            if(pointer_array[read_index].type == CHAR){
            // this is a char* so we perform the necessary type casting

                std::cout<<(char*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[read_index].type == INTEGER){
            // this is a int* so we perform the necessary type casting

                std::cout<<*(int*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[read_index].type == UNSIGNED_INTEGER){
            // this is a unsigned int* so we perform the necessary type casting

                std::cout<<*(unsigned int*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[read_index].type == INT64){
            // this is a int64_t* so we perform the necessary type casting

                std::cout<<*(int64_t*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[read_index].type == UINT64){
            // this is a uint64_t* so we perform the necessary type casting

                std::cout<<*(uint64_t*)pointer_array[read_index].pointer;

            }
            else if(pointer_array[read_index].type == DOUBLE){
            // this is a double* so we perform the necessary type casting

                std::cout<<*(double*)pointer_array[read_index].pointer;

            }


            // we increment read index
            if( !((read_index + 1) < SIZE_OF_POINTER_ARRAY) ){
            // we test whether adding 1 to the current read index would cause it to wrap around the pointer array, if so we set the wrap variable to false because the thread that printshifts would have before now set it to true to indicate that it has wrapped around the pointer array
                wrap = false;

                // we fetch the updated last write value since the value of write_index the print function copied may have been set by the print_shift function when wrapping around the pointer array
                loc_write_index = write_index;

            }
                        
            // increment the read_index variable using the ternary operator
            read_index = ((read_index + 1) < SIZE_OF_POINTER_ARRAY)? ++read_index : (read_index + 1) % SIZE_OF_POINTER_ARRAY;

        }

    }

    void print_shift(const char* data, int length_of_data){
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

    }

    void print_shift(int x){
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

    }

    void print_shift(unsigned int x){
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

    }

    void print_shift(int64_t x){
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

    }

    void print_shift(uint64_t x){
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

    }

    void print_shift(double x){
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

    }

};
