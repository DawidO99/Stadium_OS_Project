#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <semaphore.h>
#include <thread>

// Zasoby IPC
int sem_id;         // Semafor główny
int shm_id;         // Pamięć dzielona
int sem_station_id; // Zbiór semaforów dla stacji kontroli

void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon);
void spawn_fan_process(const char *fan_program_path);

int main()
{
    srand(time(NULL)); // Do generowania atrybutów

    std::cout << "[Technician] Initializing resources...\n";

    // Tworzenie semaforów
    sem_id = create_semaphore(SEM_KEY, 1, 1);                   // Semafor główny
    sem_station_id = create_semaphore(SEM_KEY_STATION_0, 3, 3); // Zbiór semaforów dla 3 stacji kontroli

    // Tworzenie pamięci współdzielonej
    shm_id = create_shared_memory(SHM_KEY, SHM_SIZE);
    void *shm_ptr = attach_shared_memory(shm_id);

    // Inicjalizacja pamięci współdzielonej
    int *stadium_data = static_cast<int *>(shm_ptr);
    stadium_data[0] = 0; // Liczba kibiców na stadionie
    for (int i = 1; i <= MAX_FANS; i++)
    {
        stadium_data[i] = 0; // PID-y kibiców
    }
    stadium_data[OFFSET_TEAM_0] = -1; // StationTeam[0]
    stadium_data[OFFSET_TEAM_1] = -1; // StationTeam[1]
    stadium_data[OFFSET_TEAM_2] = -1; // StationTeam[2]
    stadium_data[OFFSET_COUNT_0] = 0; // StationCount[0]
    stadium_data[OFFSET_COUNT_1] = 0; // StationCount[1]
    stadium_data[OFFSET_COUNT_2] = 0; // StationCount[2]

    std::cout << "[Technician] Resources initialized successfully.\n";

    // Tworzenie procesów kibiców
    const char *fan_program_path = "./build/fan";
    for (int i = 0; i < MAX_FANS; i++)
    {
        spawn_fan_process(fan_program_path);
        sleep(1);
    }

    // Czyszczenie zasobów
    detach_shared_memory(shm_ptr);
    stadium_data[0] = 0; // Zerujemy liczbę kibiców
    for (int i = 1; i <= MAX_FANS; ++i)
        stadium_data[i] = 0; // Czyszczenie PID-ów
    for (int i = 0; i < 3; ++i)
    {
        stadium_data[OFFSET_TEAM_0 + i] = -1;
        stadium_data[OFFSET_COUNT_0 + i] = 0;
    }
    remove_shared_memory(shm_id);
    remove_semaphore(sem_id);
    remove_semaphore(sem_station_id);

    return 0;
}

void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon)
{
    age = rand() % 60 + 10;          // Wiek od 10 do 70 lat
    team = rand() % 2;               // Drużyna 0 lub 1
    is_vip = (rand() % 10 == 0);     // 10% szans na VIP-a
    has_weapon = (rand() % 20 == 0); // 5% szans na broń
}

void spawn_fan_process(const char *fan_program_path)
{
    int age, team;
    bool is_vip, has_weapon;
    generate_random_fan_attributes(age, team, is_vip, has_weapon);

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("[Technician] Failed to fork");
        return;
    }
    if (pid == 0)
    {
        char age_str[10], team_str[10], is_vip_str[10], has_weapon_str[10];
        snprintf(age_str, sizeof(age_str), "%d", age);
        snprintf(team_str, sizeof(team_str), "%d", team);
        snprintf(is_vip_str, sizeof(is_vip_str), "%d", is_vip);
        snprintf(has_weapon_str, sizeof(has_weapon_str), "%d", has_weapon);

        execl(fan_program_path, fan_program_path, age_str, team_str, is_vip_str, has_weapon_str, NULL);
        perror("[Technician] Failed to exec fan process");
        exit(1);
    }
    else
    {
        std::cout << "[Technician] Spawned fan process with PID: " << pid
                  << " (Age: " << age
                  << ", Team: " << team
                  << ", VIP: " << (is_vip ? "Yes" : "No")
                  << ", Weapon: " << (has_weapon ? "Yes" : "No") << ")\n";
    }
}
