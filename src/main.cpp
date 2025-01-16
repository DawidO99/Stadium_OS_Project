#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    std::cout << "Initializing Stadium Simulation...\n";

    pid_t pid = fork();
    if (pid == 0)
    {
        execl("./build/manager", "manager", nullptr);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
        wait(nullptr);
    else
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Stadium Simulation Ended.\n";
    return 0;
}
