#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <thread>
#include <semaphore.h>

// zasoby IPC
int sem_id; // semafor glowny
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
    sem_id = create_semaphore(SEM_KEY, 0); // otwieramy semafor glowny
    // pamiec dzielona
    shm_id = create_shared_memory(SHM_KEY, SHM_SIZE);
    int *stadium_data = static_cast<int *>(attach_shared_memory(shm_id));

    // std::cout << "[Fan] Connected to shared resources.\n";

    fan_attr attributes;
    stadium_data[stadium_data[0] + 1] = getpid(); // zapisujemy PID procesu w pamieci wspoldzielonej
    attributes.PID = stadium_data[stadium_data[0] + 1];
    attributes.age = std::stoi(argv[1]);
    attributes.team = std::stoi(argv[2]);
    attributes.is_vip = std::stoi(argv[3]);
    attributes.has_weapon = std::stoi(argv[4]);
    std::cout << "[Fan] Initialized with - Age: " << attributes.age
              << ", PID" << attributes.PID
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
            std::cout << "[Fan + Child] PID : " << attributes.PID << ", VIP entering stadium. Current fans: " << stadium_data[0] << "\n";
            semaphore_signal(sem_id);
            has_child = false;
        }
        else
        {
            semaphore_wait(sem_id);
            stadium_data[0]++;
            std::cout << "[Fan] PID : " << attributes.PID << ", VIP entering stadium. Current fans: " << stadium_data[0] << "\n";
            semaphore_signal(sem_id);
        }
    }

    // obsluga stanowisk
    // Otwieramy semafory stanowiskowe (deskryptory do już utworzonych semaforów)
    int sem_stations[3];
    sem_stations[0] = create_semaphore(SEM_KEY_STATION_0, 3);
    sem_stations[1] = create_semaphore(SEM_KEY_STATION_1, 3);
    sem_stations[2] = create_semaphore(SEM_KEY_STATION_2, 3);
    // inicjalizacja zmiennych pomocniczych
    bool control_passed = false;

    while (!control_passed) // dopoki nie przejdzie kontroli
    {
        for (int i = 0; i < 3; i++) // dla kazdego stanowiska
        {
            if (semaphore_trywait(sem_stations[i]) == 0) // jezeli udalo sie zajac semafor dla stacji i, blokujemy
            {
                semaphore_wait(sem_id);                               // blokujemy semafor glowny
                int current_team = stadium_data[OFFSET_TEAM_0 + i];   // zespol przy stanowisku
                int current_count = stadium_data[OFFSET_COUNT_0 + i]; // ile osob

                if (current_team == -1 || current_team == attributes.team) // jezeli pusto lub zespol naszego procesu
                {
                    if (current_team == -1) // jezeli pusto to przypisujemy zespol naszego procesu
                        stadium_data[OFFSET_TEAM_0 + i] = attributes.team;
                    stadium_data[OFFSET_COUNT_0 + i] = current_count + 1; // zwiekszamy liczbe osob na stanowisku
                    semaphore_signal(sem_id);                             // odblokowujemy semafor glowny
                    std::cout << "[Fan] PID : " << attributes.PID << " entered station " << i << " for control. Team: " << attributes.team << "\n";

                    std::this_thread::sleep_for(std::chrono::seconds(2)); // symulacja kontroli
                    if (attributes.has_weapon)
                    {
                        std::cout << "[Fan] PID : " << attributes.PID << ", weapon detected. Fan is led away by security." << std::endl;
                        exit(0); // jak kibic ma bron to go wyprowadzamy ze stadionu
                    }
                    // po pozytywnej kontroli opuszczamy stanowisko
                    semaphore_wait(sem_id);                    // blokujemy semafor glowny
                    stadium_data[OFFSET_COUNT_0 + i]--;        // kibic schodzi ze stanowiska kontroli
                    if (stadium_data[OFFSET_COUNT_0 + i] == 0) // jezeli po zejsciu stanowisko jest puste
                        stadium_data[OFFSET_TEAM_0 + i] = -1;  // to znowu nie nalezy do zadnej druzyny
                    semaphore_signal(sem_id);                  // odblokowujemy semafor glowny

                    // semaphore_signal(sem_stations[i]); // odblokowujemy semafor stanowiska
                    semaphore_signal(sem_stations[i]);

                    control_passed = true;
                    break;
                }
                else // jezeli stanowisko jest zajete przez inna druzyne to zwalniamy slot
                {
                    semaphore_signal(sem_id);
                    semaphore_signal(sem_stations[i]);
                    // semaphore_signal(sem_stations[i]);
                }
            }
        }
    }
    if (!control_passed) // jezeli nie udalo sie przejsc, frustracja rosnie
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        attributes.frustration++;
        if (attributes.frustration > 5)
        {
            std::cout << "[Fan] PID : " << attributes.PID << ", I'm frustrated and leaving...\n";
            exit(0);
        }
    }
    std::cout << "[Fan] PID : " << attributes.PID << ", Passed control area.\n"; // koniec kontroli, wpuszczamy na stadion

    // teraz zostal nam zwykly kibic
    semaphore_wait(sem_id);
    if (has_child)
    {
        stadium_data[0] += 2;
        std::cout << "[Fan + Child] PID : " << attributes.PID << ", Regular fans entering stadium. Current fans: " << stadium_data[0] << "\n";
        has_child = false;
    }
    else
    {
        stadium_data[0]++;
        std::cout << "[Fan] PID : " << attributes.PID << ", Regular fan entering stadium. Current fans: " << stadium_data[0] << "\n";
    }
    semaphore_signal(sem_id);
}