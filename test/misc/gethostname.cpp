#include <unistd.h>
#include <iostream>
#include <string>

int main(int argc, char const* argv[])
{
    char hostname[1024];
    gethostname(hostname, 1024);
    std::string mystr(hostname);

    std::cout << mystr << std::endl;

    return 0;
}
