#pragma once

// we now define the printing class
class Printer{

public:

    // constructor
    Printer(char* prelude_string = nullptr, int64_t pointer_array_size = 0ll, int64_t data_buffer_size = 0ll, int64_t scratchpad_size = 0ll);

    // destructor
    ~Printer();

private:

    // stdout file descriptor
    static constexpr int STDOUT = 1;

    // minimum available space in bytes for parsing numbers - this constant sets the minimum space that would be available in the scratchpad before we format a number into the scratchpad, we declare this constant as a ssize_t because it would be compared to the result of pointer arithmetic
    static constexpr ssize_t MIN_NUM_SPACE = 30;

private:

    // define the constants that define the pointer type
    inline static constexpr int CHAR = 0;
    inline static constexpr int INTEGER = 1;
    inline static constexpr int UNSIGNED_INTEGER = 2;
    inline static constexpr int INT64 = 3;
    inline static constexpr int UINT64 = 4;
    inline static constexpr int DOUBLE = 5;
    inline static constexpr int ENDL = 6;

private:

    // we define the pointer struct
    struct pointer_struct{

        // the type member defines the data type the pointer struct holds 
        int type;

        // this anonymous union member holds the data this pointer struct holds, for char* data types it holds the pointer to the char* in our data buffer
        union {
            char* pointer;
            int x_int;
            unsigned int x_uint;
            double x_double;
            int64_t x_int64;
            uint64_t x_uint64;
        };

        // the data end member holds the data write unmasked value for a char* write, thisis published to the data_read variable after the print thread has read it
        int64_t data_end = 0ll;

    };

    // we define the Printer endl tag
    static constexpr struct EndlTag {} endl{};

private:

    // this variable holds the size of the pointer array - the pointer array size has to be a power of 2 for seamless indexing. The default value is 256 * 1024 entries
    int64_t SIZE_OF_POINTER_ARRAY = 256 * 1024;

    // this defines the length of the internal char buffer used for print shifting - by default we allow 16 bytes for each pointer location so the default buffer size is 16 * SIZE_OF_POINTER_ARRAY
    int64_t SIZE_OF_BUFFER = 16 * SIZE_OF_POINTER_ARRAY;

    // we set the default size of the scratchpad used for holding data just before printing to 1MB
    int64_t SIZE_OF_SCRATCHPAD = 1024 * 1024;
    
    // the pointer_struct array pointer
    pointer_struct* pointer_array = nullptr;

    // internal buffer pointer used for print shifting
    char* buffer = nullptr;

    // scratchpad pointer
    char* scratchpad = nullptr;

    // scratchpad cursor to use to traverse the scratchpad
    char* scratchpad_cursor = nullptr;

    // we declare a pointer to point to the end of the scratchpad
    char* end_of_scratchpad = nullptr;

    // prelude string pointer - the prelude is the text printed before the print shifted data in the printer buffer, it can be used to differentiate data from different printers
    char* prelude = nullptr;

    // prelude string length - this is computed if the printer class is constructed with a prelude string
    ssize_t prelude_len = 0;

    // counter used to keep track of written bytes in the data buffer
    int64_t data_write = 0ll;

    // counter used to keep track of read bytes in the data buffer - this variable is declared as atomic so the print shift thread knows where exactly the print thread is and it doesn't overwrite data that hasn't been printed yet
    std::atomic<int64_t> data_read = 0ll;

private: 

    // this variable when updated holds the index of the pointer_array where the next write should occur, it is made atomic so once it is updated the change is visible to all threads - the flush function updates this variable with the value of the p_array_index, it is kept separately so print shifts can happen in batches and it won't be visible to the print thread till flush is called
    std::atomic<int64_t> write_index = 0ll;

    // this variable indicates the index in the pointer_array where the next entry should be entered - this is the effective location the print shift thread writes to
    int64_t p_array_index = 0ll;

    // this variable holds the pointer_array index where the next read should be read from - this is an atomic variable so the write thread can know when the pointer array is full
    std::atomic <int64_t> read_index = 0ll;

private:

    // error variables
    bool error = false;

    static constexpr int SIZE_OF_ERROR_BUFFER = 128;

    char error_buffer[SIZE_OF_ERROR_BUFFER] = {'\0'};

    // the failed writes variable is used to keep track of the writes to screen that failed due to the write system call returning an error
    int64_t failed_writes = 0ll;

    // the dropped data variable is used to keep track of the number of times data had to be dropped from print shift calls due to the pointer array or the data buffer being full
    int64_t dropped_data = 0ll;

public:

    // the flush function makes all print shifted data visible to the printing thread
    void flush();

    // the write to screen function prints all data in the scratchpad to the screen
    void write_to_screen();

    // the print function is used to print all print shifted data
    void print();

    // the free print function is used to print print shifted data from any number of printer objects
    template <typename... Printer>
    static void print_many(Printer&... p){ (p.print(), ...); }

public:

    // print shift functions

    // char* print shift function
    Printer& print_shift(const char* data);

    // int print shift function
    Printer& print_shift(int x);

    // unsigned int print shift function
    Printer& print_shift(unsigned int x);

    // int64 print shift function
    Printer& print_shift(int64_t x);

    // uint64 print shift function
    Printer& print_shift(uint64_t x);

    // double print shift function
    Printer& print_shift(double x);

    // endl print shift function
    Printer& print_shift(EndlTag endl_tag);

};

