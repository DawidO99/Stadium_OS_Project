#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <thread>

int sem_id;
int shm_id;
int *stadium_data;

struct fan_attr
{
    int PID = -1;            // PID procesu kibica
    int age = 18;            // Wiek (domyślnie pełnoletni)
    bool has_weapon = false; // Czy kibic ma broń
    bool is_vip = false;     // Czy jest VIP-em
    int team = -1;           // Drużyna kibica (0 lub 1)
    int frustration = 0;     // Poziom frustracji (0-5)
};

void cleanup_and_exit(int sig); // Funkcja obsługująca SIGTERM

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        std::cerr << "[Fan] Invalid arguments. Expected: age team is_vip has_weapon\n";
        return 1;
    }

    if (std::signal(SIGTERM, cleanup_and_exit) == SIG_ERR)
    {
        perror("[Fan] Failed to set SIGTERM handler");
        return 1;
    }

    sem_id = create_semaphore(SEM_KEY, 1);
    if (sem_id == -1)
    {
        perror("[Fan] Failed to attach to semaphore");
        return 1;
    }

    shm_id = create_shared_memory(SHM_KEY, sizeof(int));
    if (shm_id == -1)
    {
        perror("[Fan] Failed to attach to shared memory");
        return 1;
    }

    stadium_data = static_cast<int *>(attach_shared_memory(shm_id));
    if (stadium_data == nullptr)
    {
        perror("[Fan] Failed to attach shared memory");
        return 1;
    }

    std::cout << "[Fan] Connected to shared resources.\n";

    fan_attr attributes;
    attributes.PID = getpid();
    attributes.age = std::stoi(argv[1]);
    attributes.team = std::stoi(argv[2]);
    attributes.is_vip = std::stoi(argv[3]);
    attributes.has_weapon = std::stoi(argv[4]);

    std::cout << "[Fan] Initialized with - Age: " << attributes.age
              << ", Team: " << attributes.team
              << ", VIP: " << (attributes.is_vip ? "Yes" : "No")
              << ", Weapon: " << (attributes.has_weapon ? "Yes" : "No") << "\n";

    // Logika VIP
    if (attributes.is_vip)
    {
        semaphore_wait(sem_id);
        stadium_data[0]++;
        std::cout << "[Fan] VIP entering stadium. Current fans: " << stadium_data[0] << "\n";
        semaphore_signal(sem_id);

        while (true)
        {
            pause();
        }
    }

    if (attributes.has_weapon)
    {
        std::cout << "[Fan] Security detected a weapon. Access denied.\n";
        detach_shared_memory(stadium_data);
        exit(0); // Proces kończy się
    }

    // Logika dzieci
    if (attributes.age < 15)
    {
        std::cout << "[Fan] Child detected. Creating adult process and child thread.\n";
        attributes.age = 18;

        // Tworzymy wątek dziecka
        std::thread child_thread([]()
                                 { std::cout << "[Child] Child is enjoying the match.\n"; });

        semaphore_wait(sem_id);
        stadium_data[0] += 2;
        std::cout << "[Fan + Child] Entering stadium. Current fans: " << stadium_data[0] << "\n";
        semaphore_signal(sem_id);

        // Dołączamy wątek (czekamy na jego zakończenie)
        child_thread.join();

        while (true)
        {
            pause();
        }
    }

    // Domyślna logika kibica
    semaphore_wait(sem_id);
    stadium_data[0]++;
    std::cout << "[Fan] Regular fan entering stadium. Current fans: " << stadium_data[0] << "\n";
    semaphore_signal(sem_id);

    while (true)
    {
        pause();
    }

    return 0;
}

void cleanup_and_exit(int sig)
{
    std::cout << "[Fan] Received SIGTERM. Exiting...\n";

    if (stadium_data)
    {
        stadium_data[0]--;
    }
    detach_shared_memory(stadium_data);

    exit(0);
}