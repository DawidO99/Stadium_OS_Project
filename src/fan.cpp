// fan.cpp - Zaktualizowana wersja
#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <thread>
#include <semaphore.h>

// Zasoby IPC
int sem_id; // Semafor główny
int shm_id; // Pamięć dzielona

bool has_child = false;

struct fan_attr
{                            // Struktura z atrybutami kibiców
    int PID = -1;            // PID procesu kibica
    int age = 18;            // Wiek (domyślnie pełnoletni)
    bool has_weapon = false; // Czy kibic ma broń
    bool is_vip = false;     // Czy jest VIP-em
    int team = -1;           // Drużyna kibica (0 lub 1)
    int frustration = 0;     // Poziom frustracji (0-5), domyślnie 0
};

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        perror("[Fan] Invalid arguments. Expected: age team is_vip has_weapon\n");
        return 1;
    }

    // Semafory i pamięć dzielona
    sem_id = create_semaphore(SEM_KEY, 1, 0); // Otwieramy semafor główny
    shm_id = create_shared_memory(SHM_KEY, SHM_SIZE);
    int *stadium_data = static_cast<int *>(attach_shared_memory(shm_id));

    fan_attr attributes;
    stadium_data[stadium_data[0] + 1] = getpid(); // Zapisujemy PID procesu w pamięci współdzielonej
    attributes.PID = stadium_data[stadium_data[0] + 1];
    attributes.age = std::stoi(argv[1]);
    attributes.team = std::stoi(argv[2]);
    attributes.is_vip = std::stoi(argv[3]);
    attributes.has_weapon = std::stoi(argv[4]);

    std::cout << "[Fan] Initialized with - Age: " << attributes.age
              << ", PID: " << attributes.PID
              << ", Team: " << attributes.team
              << ", VIP: " << (attributes.is_vip ? "Yes" : "No")
              << ", Weapon: " << (attributes.has_weapon ? "Yes" : "No") << "\n";

    semaphore_wait(sem_id, 0); // Blokujemy semafor, żeby sprawdzić miejsca na stadionie
    if (stadium_data[0] >= MAX_FANS)
    {
        std::cout << "[Fan] Stadium is full. Number of Fans: " << stadium_data[0] << ".\n";
        exit(0);
    }
    semaphore_signal(sem_id, 0); // Odblokowujemy semafor

    if (attributes.age < 15)
    {
        has_child = true;
        std::cout << "[Fan] Child detected. Assigning an adult.\n";
        attributes.age = rand() % 52 + 18; // Ustawiamy na pełnoletni wiek
        std::thread child([]()
                          { std::cout << "[Child] Child assigned to an adult.\n"; });
        child.join();
    }

    if (attributes.is_vip)
    {
        semaphore_wait(sem_id, 0); // blokujemy semafor
        if (stadium_data[0] >= MAX_FANS - 1)
        {
            std::cout << "[Fan] Stadium is full. Number of Fans : " << stadium_data[0] << "." << std::endl;
            exit(0); // jak nie ma miejsc to kibic nie wchodzi, wiec go wyrzucamy (pozniej bedzie signal)
        }
        semaphore_signal(sem_id, 0); // mozemy odblokowacs
        semaphore_wait(sem_id, 0);
        stadium_data[0] += has_child ? 2 : 1;
        std::cout << (has_child ? "[Fan + Child]" : "[Fan]")
                  << " VIP entering stadium. Current fans: " << stadium_data[0] << "\n";
        semaphore_signal(sem_id, 0);
        has_child = false;
        exit(0);
    }

    // Obsługa stanowisk kontroli (zbiór semaforów)
    int sem_station_id = create_semaphore(SEM_KEY_STATION_0, 3, 3); // Zbiór semaforów dla stacji kontroli
    bool control_passed = false;

    while (!control_passed)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (semaphore_trywait(sem_station_id, i) == 0)
            {
                semaphore_wait(sem_id, 0);
                int current_team = stadium_data[OFFSET_TEAM_0 + i];
                int current_count = stadium_data[OFFSET_COUNT_0 + i];

                if ((current_team == -1 || current_team == attributes.team) && current_count < 3)
                {
                    if (current_team == -1)
                    {
                        stadium_data[OFFSET_TEAM_0 + i] = attributes.team;
                    }
                    stadium_data[OFFSET_COUNT_0 + i] = current_count + 1;
                    semaphore_signal(sem_id, 0);

                    std::cout << "[Fan] PID: " << attributes.PID
                              << " entered station " << i
                              << " for control.\n";
                    std::this_thread::sleep_for(std::chrono::seconds(2));

                    if (attributes.has_weapon)
                    {
                        std::cout << "[Fan] Weapon detected. Security intervening.\n";
                        exit(0);
                    }

                    // Zwolnienie stacji
                    semaphore_wait(sem_id, 0);
                    stadium_data[OFFSET_COUNT_0 + i]--;
                    if (stadium_data[OFFSET_COUNT_0 + i] == 0)
                    {
                        stadium_data[OFFSET_TEAM_0 + i] = -1;
                    }
                    semaphore_signal(sem_id, 0);
                    semaphore_signal(sem_station_id, i);
                    control_passed = true;
                    break;
                }
                semaphore_signal(sem_id, 0);
                semaphore_signal(sem_station_id, i);
            }
        }

        if (!control_passed)
        {
            attributes.frustration++;
            if (attributes.frustration > 5)
            {
                // Zwolnij zajęty semafor, jeśli kibic zdążył go zajmować
                for (int i = 0; i < 3; ++i)
                    semaphore_signal(sem_station_id, i);
                std::cout << "[Fan] Frustrated and leaving...\n";
                exit(0);
            }
            std::cout << "[Fan] Waiting before retrying...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Dodanie opóźnienia
        }
    }

    semaphore_wait(sem_id, 0);
    stadium_data[0] += has_child ? 2 : 1;
    std::cout << (has_child ? "[Fan + Child]" : "[Fan]")
              << " Regular fan entering stadium. Current fans: " << stadium_data[0] << "\n";
    semaphore_signal(sem_id, 0);
    return 0;
}