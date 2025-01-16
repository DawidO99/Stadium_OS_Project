#include "../include/ipc_utils.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <cstdlib>

#define SHM_KEY 0x1235 // Klucz pamięci współdzielonej
#define SEM_KEY 0x1234 // Klucz semafora

int sem_id;
int shm_id;
int *stadium_data;

void cleanup_and_exit(int sig); // Obsluga sygnalu SIGTERM

int main()
{
    struct sigaction sa;
    sa.sa_handler = cleanup_and_exit; // Funkcja obsługująca sygnały
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask); // Blokowanie innych sygnałów podczas obsługi

    if (sigaction(SIGTERM, &sa, NULL) == -1)
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

    semaphore_wait(sem_id);
    stadium_data[0]++;
    std::cout << "[Fan] Entering stadium. Current fans: " << stadium_data[0] << "\n";
    semaphore_signal(sem_id);

    while (true)
    {
        pause(); // Oczekiwanie na sygnały
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