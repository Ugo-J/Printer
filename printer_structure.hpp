#pragma once

// we now define the printing class
class Printer{

public:

    // constructor
    Printer(char* prelude_string = nullptr, int64_t pointer_array_size = 0ll, int64_t data_buffer_size = 0ll, int64_t scratchpad_size = 0ll);

    // destructor
    ~Printer();

private:

    // define the constants that define the pointer type
    inline static constexpr int CHAR = 0;
    inline static constexpr int INTEGER = 1;
    inline static constexpr int UNSIGNED_INTEGER = 2;
    inline static constexpr int INT64 = 3;
    inline static constexpr int UINT64 = 4;
    inline static constexpr int DOUBLE = 5;

private:

    // we define the pointer struct
    struct pointer_struct{ int type; void* pointer; };

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

    // prelude string pointer - the prelude is the text printed before the print shifted data in the printer buffer, it can be used to differentiate data from different printers
    char* prelude = nullptr;

private: 

    // this variable when updated holds the index of the pointer_array where the next write should occur, it is made atomic so once it is updated the change is visible to all threads
    std::atomic<int64_t> write_index = 0ll;

    // this cursor is used to indicate the next write location in the internal buffer
    void* buffer_cursor = buffer;

    // this pointer holds the address of the last location in the internal buffer
    char* end_of_buffer = &buffer[SIZE_OF_BUFFER - 1];

    // this variable is used to indicate that print shifting has gotten to the end of the ponter array and wrapped around to the start of the array
    bool wrap = false;

    // this variable indicates the index in the pointer_array where the next entry should be entered
    int p_array_index = 0;

    // this variable holds the pointer_array index where the next read should be read from
    int read_index = 0;

private:

    // error variables

    bool error = false;

    static constexpr int SIZE_OF_ERROR_BUFFER = 256;

    char error_buffer[SIZE_OF_ERROR_BUFFER] = {'\0'};

public:

    // the flush function makes all print shifted data visible to the printing thread
    void flush();

    // the print function is used to print all print shifted data
    void print();

    // the free print function is used to print print shifted data from any number of printer objects
    template <typename... Printer>
    static void print_many(Printer&... p){ (p.print(), ...); }

public:

    // print shift functions

    // char* print shift function
    Printer& print_shift(const char* data, int length_of_data);

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

};

