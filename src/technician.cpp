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

void terminate_fan_processes(); // wysylanie sygnalu do wszystkich procesow
void signal_handler(int sig);   // obsluga sygnalow

int main()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler; // Funkcja obsługująca sygnały
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask); // Blokowanie innych sygnałów podczas obsługi

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        perror("[Technician] Failed to set SIGUSR1 handler");

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
        perror("[Technician] Failed to set SIGUSR2 handler");

    if (sigaction(SIGTERM, &sa, NULL) == -1)
        perror("[Technician] Failed to set SIGTERM handler");

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
    int *stadium_data = static_cast<int *>(shm_ptr);
    stadium_data[0] = 0; // Liczba kibiców na stadionie
    for (int i = 1; i <= MAX_FANS; i++)
    {
        stadium_data[i] = 0; // Inicjalizacja przestrzeni dla PID-ów kibiców
    }

    std::cout << "[Technician] Resources initialized successfully.\n";

    while (true)
    {
        pause(); // Oczekiwanie na sygnały
    }

    // Czyszczenie zasobów (to się nie wykona, bo program kończy się w handlerze SIGTERM)
    detach_shared_memory(shm_ptr);
    remove_shared_memory(shm_id);
    remove_semaphore(sem_id);

    return 0;
}


void terminate_fan_processes()
{
    std::cout << "[Technician] Sending SIGTERM to all fan processes...\n";
    for (int i = 0; i < MAX_FANS; i++)
    {
        int fan_pid = static_cast<int *>(attach_shared_memory(shm_id))[i + 1];
        if (fan_pid > 0)
            kill(fan_pid, SIGTERM);
    }
}

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
        terminate_fan_processes(); // Wysyłanie SIGTERM do kibiców
        int *stadium_data = static_cast<int *>(attach_shared_memory(shm_id));
        stadium_data[0] = 0; // Wyzerowanie liczby kibiców
        detach_shared_memory(stadium_data);

        remove_semaphore(sem_id);
        remove_shared_memory(shm_id);

        std::cout << "[Technician] Resources cleaned up. Exiting.\n";
        exit(0);
    }
}