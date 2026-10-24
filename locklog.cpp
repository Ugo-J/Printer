#include <chrono>
#include "locklog.hpp"

int main(){

    Lock_logger p(const_cast<char*>("Prelude Test\n"));

    // Lock_logger p;

    constexpr int P_TIMESTAMP_LENGTH = 20;

    char p_timestamp[P_TIMESTAMP_LENGTH];

    std::chrono::_V2::system_clock::time_point start;
    std::chrono::duration<double> seconds;

    for(int i = 0; i < 3; i++){

        /* start = std::chrono::high_resolution_clock::now();

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

        p.print_shift("Hello World");

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
        p.print_shift(static_cast<uint64_t>(9876543210987654321));

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Uint64 Print Shift Time: "<<seconds.count()<<"s\n";

        start = std::chrono::high_resolution_clock::now();
        p.print_shift(3.14159265358979323846);

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Double Print Shift Time: "<<seconds.count()<<"s\n"; */

        start = std::chrono::high_resolution_clock::now();

        p<<"Hello World"<<123456789<<987654321u<<(int64_t)1234567890123456789<<static_cast<uint64_t>(9876543210987654321)<<3.14159265358979323846<<Lock_logger::endl;

        seconds = std::chrono::high_resolution_clock::now() - start;
        std::cout<<"Operator<< Print Shift Time: "<<seconds.count()<<"s\n";  

    }

    p.flush();

    // p.print();

    start = std::chrono::high_resolution_clock::now();

    // Lock_logger::print_many(p);

    p.print();

    seconds = std::chrono::high_resolution_clock::now() - start;
    std::cout<<"Flush Time: "<<seconds.count()<<"s\n";    

}