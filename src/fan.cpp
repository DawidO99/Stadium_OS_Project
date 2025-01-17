#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <thread>

// zasoby IPC
int sem_id; // semafor
int shm_id; // pamiec dzielona

bool has_child = false;

struct fan_attr // struct z atrybutami kibicow
{
    int PID = -1;            // PID procesu kibica
    int age = 18;            // Wiek (domyślnie pełnoletni)
    bool has_weapon = false; // Czy kibic ma broń
    bool is_vip = false;     // Czy jest VIP-em
    int team = -1;           // Drużyna kibica (0 lub 1)
    int frustration = 0;     // Poziom frustracji (0-5), domyslnie 0
};

int main(int argc, char *argv[])
{
    if (argc < 5) // sprawdzamy czy argumenty kibica sa przekazane poprawnie
    {
        perror("[Fan] Invalid arguments. Expected: age team is_vip has_weapon\n");
        return 1;
    }

    // semafor
    sem_id = create_semaphore(SEM_KEY, 1); // tworzymy semafor o kluczu 0x1234 i wartosci 1, czyli w sumie ten sam co w technician.cpp
    // pamiec dzielona
    shm_id = create_shared_memory(SHM_KEY, SHM_SIZE);
    int *stadium_data = static_cast<int *>(attach_shared_memory(shm_id));

    std::cout << "[Fan] Connected to shared resources.\n";

    fan_attr attributes;
    stadium_data[stadium_data[0] + 1] = getpid(); //zapisujemy PID procesu w pamieci wspoldzielonej
    attributes.PID = stadium_data[stadium_data[0] + 1];
    attributes.age = std::stoi(argv[1]);
    attributes.team = std::stoi(argv[2]);
    attributes.is_vip = std::stoi(argv[3]);
    attributes.has_weapon = std::stoi(argv[4]);

    std::cout << "[Fan] Initialized with - Age: " << attributes.age
              << ", Team: " << attributes.team
              << ", VIP: " << (attributes.is_vip ? "Yes" : "No")
              << ", Weapon: " << (attributes.has_weapon ? "Yes" : "No") << "\n";

    semaphore_wait(sem_id);          // blokujemy semafor, zeby sprawdzic czy sa miejsca na stadionie
    if (stadium_data[0] >= MAX_FANS) // 1. czy jest wolne miejsce
    {
        std::cout << "[Fan] Stadium is full. Number of Fans : " << stadium_data[0] << "." << std::endl;
        exit(0); // jak nie ma miejsc to kibic nie wchodzi, wiec go wyrzucamy (pozniej bedzie signal)
    }
    semaphore_signal(sem_id); // mozemy odblokowac semafor

    if (attributes.age < 15) // 2. czy jest dzieckiem
    {
        semaphore_wait(sem_id); // blokujemy semafor
        if (stadium_data[0] >= MAX_FANS - 1)
        {
            std::cout << "[Fan] Stadium is full. Number of Fans : " << stadium_data[0] << "." << std::endl;
            exit(0); // jak nie ma miejsc to kibic nie wchodzi, wiec go wyrzucamy (pozniej bedzie signal)
        }
        semaphore_signal(sem_id); // mozemy odblokowac

        has_child = true;
        std::cout << "[Fan] Child detected. Looking for an adult..\n";
        attributes.age = rand() % 52 + 18; // ustawiamy proces na pelnoletni
        std::thread child([]()
                          { std::cout << "[Child] Child found his parent.\n"; }); // generujemy mu watek dziecko, mamy teraz relacje rodzic - dziecko
        child.join();                                                             // dolaczamy watek
    }

    if (attributes.is_vip) // 3. czy jest vipem - jak tak to wchodzi
    {
        if (has_child)
        {
            semaphore_wait(sem_id);
            stadium_data[0] += 2;
            std::cout << "[Fan + Child] VIP entering stadium. Current fans: " << stadium_data[0] << "\n";
            semaphore_signal(sem_id);
            has_child = false;
        }
        else
        {
            semaphore_wait(sem_id);
            stadium_data[0]++;
            std::cout << "[Fan + Child] VIP entering stadium. Current fans: " << stadium_data[0] << "\n";
            semaphore_signal(sem_id);
        }
    }

    // jaki zespol
    // obsluga stanowsik - czy jestt miejsce, jak nie to frustracja
}