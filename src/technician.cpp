#include "../include/ipc_utils.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <csignal>

#define SHM_SIZE 1024 // Rozmiar pamięci współdzielonej
#define MAX_FANS 100  // Maksymalna liczba kibiców na stadionie

// Zasoby IPC
int sem_id;
int shm_id;

// Handler sygnałów
void signal_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        std::cout << "[Technician] Received signal: SIGUSR1 (Stop fans entering).\n";
        semaphore_wait(sem_id); // Wstrzymanie dostępu do stadionu
    }
    else if (sig == SIGUSR2)
    {
        std::cout << "[Technician] Received signal: SIGUSR2 (Resume fans entering).\n";
        semaphore_signal(sem_id); // Wznowienie dostępu do stadionu
    }
    else if (sig == SIGTERM)
    {
        std::cout << "[Technician] Received signal: SIGTERM (Evacuation).\n";
        // exit()? tu chyba zakonczymy dzialanie calego programu
    }
}

int main()
{
    struct sigaction sa;            // struct do sygnalow
    sa.sa_handler = signal_handler; // wskaznik na funkcje signal_handler
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask); // inicjuje maske

    if (sigaction(SIGUSR1, &sa, NULL) == -1 || sigaction(SIGUSR2, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("[Technician] Failed to set signal handlers");
        return 1;
    }
    std::cout << "[Technician] Initializing resources...\n";

    // Tworzenie semafora
    sem_id = create_semaphore(0x1234, 1);
    if (sem_id == -1)
    {
        std::cerr << "[Technician] Failed to create semaphore.\n";
        return 1;
    }

    // Tworzenie pamięci współdzielonej
    shm_id = create_shared_memory(0x1235, SHM_SIZE);
    if (shm_id == -1)
    {
        std::cerr << "[Technician] Failed to create shared memory.\n";
        remove_semaphore(sem_id);
        return 1;
    }

    // Dołączanie pamięci współdzielonej
    void *shm_ptr = attach_shared_memory(shm_id);
    if (shm_ptr == nullptr)
    {
        std::cerr << "[Technician] Failed to attach shared memory.\n";
        remove_shared_memory(shm_id);
        remove_semaphore(sem_id);
        return 1;
    }

    // Inicjalizacja danych w pamięci współdzielonej
    int *stadium_data = static_cast<int *>(shm_ptr); // rzutujemy na int-a, zeby uzywac pamieci wspoldzielonej jako tablicy int-ow
    stadium_data[0] = 0;                             // Liczba kibiców na stadionie

    std::cout << "[Technician] Resources initialized successfully.\n";

    while (true)
    {
        pause();
    } // Czeka na sygnały

    // Czyszczenie zasobów
    detach_shared_memory(shm_ptr);
    remove_shared_memory(shm_id);
    remove_semaphore(sem_id);

    std::cout << "[Technician] Resources cleaned up. Exiting.\n";
    return 0;
}
