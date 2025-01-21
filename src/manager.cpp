#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <csignal>

// Funkcje pomocnicze
int get_technician_pid(int *stadium_data); // sciagamy pid praxownika techninczego do wysylania sygnalow
void print_menu();

int main()
{
    // Otwórz pamięć współdzieloną
    int shm_id = create_shared_memory(SHM_KEY, SHM_SIZE);
    int *stadium_data = static_cast<int *>(attach_shared_memory(shm_id));

    if (stadium_data == nullptr)
    {
        std::cerr << "[Manager] Failed to attach to shared memory.\n";
        return 1;
    }
    stadium_data[OFFSET_COUNT_2 + 3] = getpid(); // pobieramy pid Managera, zeby kierownik mogl poinformowac o zakonczeniu ewakuacji

    // Pobierz PID Technician
    int technician_pid = get_technician_pid(stadium_data);
    if (technician_pid <= 0)
    {
        std::cerr << "[Manager] Invalid Technician PID.\n";
        detach_shared_memory(stadium_data);
        return 1;
    }

    std::cout << "[Manager] Connected to Technician with PID: " << technician_pid << "\n";

    // Menu sygnalow
    int option = -1;
    while (option != 0)
    {
        print_menu();
        std::cin >> option;

        switch (option)
        {
        case 1: // Wstrzymanie wpuszczania kibicow
            if (kill(technician_pid, SIGUSR1) == 0)
            {
                std::cout << "[Manager] Sent SIGUSR1 to Technician.\n";
            }
            else
            {
                perror("[Manager] Failed to send SIGUSR1");
            }
            break;

        case 2: // Wznowienie wpuszczania kibicow
            if (kill(technician_pid, SIGUSR2) == 0)
            {
                std::cout << "[Manager] Sent SIGUSR2 to Technician.\n";
            }
            else
            {
                perror("[Manager] Failed to send SIGUSR2");
            }
            break;

        case 3: // Ewakuacja/Zakonczenie programu
            if (kill(technician_pid, SIGTERM) == 0)
            {
                std::cout << "[Manager] Sent SIGTERM to Technician.\n";
            }
            else
            {
                perror("[Manager] Failed to send SIGTERM");
            }
            break;

        case 0: // Wyjscie z managera
            std::cout << "[Manager] Exiting...\n";
            break;

        default:
            std::cout << "[Manager] Invalid option. Please try again.\n";
        }
    }

    // Odłącz pamięć współdzieloną
    detach_shared_memory(stadium_data);
    return 0;
}

// Funkcja pobierająca PID Technician z pamięci współdzielonej
int get_technician_pid(int *stadium_data)
{
    return stadium_data[OFFSET_COUNT_2 + 1]; // PID Technician na końcu tablicy
}

void print_menu()
{
    std::cout << "=== Stadium Manager ===\n";
    std::cout << "1. Stop fan entry (SIGUSR1)\n";
    std::cout << "2. Resume fan entry (SIGUSR2)\n";
    std::cout << "3. Evacuate stadium (SIGTERM)\n";
    std::cout << "0. Exit Manager\n";
    std::cout << "Select an option: ";
}