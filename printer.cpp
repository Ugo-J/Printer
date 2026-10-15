#include <chrono>
#include <NanoLogCpp17.h>
#include <array>
#include "printer.hpp"

using namespace NanoLog::LogLevels;

int main(){

    printer p;

    constexpr int P_TIMESTAMP_LENGTH = 20;

    char p_timestamp[P_TIMESTAMP_LENGTH];

    NanoLog::setLogFile("nanolog_output.log");

    std::chrono::_V2::system_clock::time_point start;
    std::chrono::duration<double> seconds;

    for(int i = 0; i < 3; i++){

        start = std::chrono::high_resolution_clock::now();

        char* ptr = std::to_chars(p_timestamp, p_timestamp + P_TIMESTAMP_LENGTH, 1000).ptr;
        // null terminate the result
        *ptr = '\0';

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"To Chars Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();

        std::cout<<1000<<std::endl;

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Flush Print Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();

        NANO_LOG(NOTICE, "Hello World! This is an integer %d and a double %lf\r\n", 1, 2.0);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Nanolog Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();

        p.print_shift("Hello World", 11);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"String Print Shift Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();

        p.print_shift(123456789);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Int Print Shift Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();

        p.print_shift(987654321u);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Unsigned Int Print Shift Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();

        p.print_shift((int64_t)1234567890123456789);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Int64 Print Shift Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();
        p.print_shift((uint64_t)9876543210987654321);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Uint64 Print Shift Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();
        p.print_shift(3.14159265358979323846);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Double Print Shift Time: "<<seconds.count()<<"s\n";

    }

    p.flush();

    p.print();

}