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

// Zasoby IPC
int sem_id;            // semafor
int shm_id;            // pamiec dzielona
int sem_station_id[3]; // semafory dla 3 stanowisk kontroli

// Funkcje obslugujace
void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon); // generujemy atrybuty kibica
void spawn_fan_process(const char *fan_program_path);                                     // generujemy kibica z odpowiednimi atrybutami

int main()
{
    srand(time(NULL)); // do generowania atrybutow

    // inicjalizacja wszystkich mechanizmow
    std::cout << "[Technician] Initializing resources...\n";

    // semafor glowny
    sem_id = create_semaphore(SEM_KEY, 1); // tworzymy semafor o kluczu 0x1234 i wartosci 1
    // semafory dla stanowisk kontroli
    sem_station_id[0] = create_semaphore(SEM_KEY_STATION_0, 3);
    sem_station_id[1] = create_semaphore(SEM_KEY_STATION_1, 3);
    sem_station_id[2] = create_semaphore(SEM_KEY_STATION_2, 3);
    // pamiec wspoldzielona
    shm_id = create_shared_memory(SHM_KEY, SHM_SIZE); // tworzymy pamiec wspoldzielona o kluczu 0x1235 i rozmiarze SHM_SIZE -> stala w constants.h
    void *shm_ptr = attach_shared_memory(shm_id);     // dolaczamy pamiec wspoldzielona
    // Inicjalizacja danych w pamięci współdzielonej
    int *stadium_data = static_cast<int *>(shm_ptr); // rzutujemy pamiec na tablice int-ow
    stadium_data[0] = 0;                             // pod indeksem 0 trzymamy liczbe kibiców na stadionie
    for (int i = 1; i <= MAX_FANS; i++)              // pod indeksami [1, MAX_FANS] trzymamy PID-y kibicow
        stadium_data[i] = 0;                         // Inicjalizacja przestrzeni dla PID-ów kibiców

    // info o stanowiskach
    stadium_data[OFFSET_TEAM_0] = -1; // stationTeam[0]
    stadium_data[OFFSET_TEAM_1] = -1; // stationTeam[1]
    stadium_data[OFFSET_TEAM_2] = -1; // stationTeam[2]
    // info o miejsach przy danym stanowisku
    stadium_data[OFFSET_COUNT_0] = 0; // stationCount[0]
    stadium_data[OFFSET_COUNT_1] = 0; // stationCount[1]
    stadium_data[OFFSET_COUNT_2] = 0; // stationCount[2]

    std::cout << "[Technician] Resources initialized successfully.\n\n";
    // zakonczenie inicjalizacji

    // tworzymy proces kibica
    const char *fan_program_path = "./build/fan";
    for (int i = 0; i < MAX_FANS; i++)
    {
        spawn_fan_process(fan_program_path);
        sleep(1);
    }

    // Czyszczenie zasobów (to się nie wykona, bo program kończy się w handlerze SIGTERM)
    detach_shared_memory(shm_ptr);
    remove_shared_memory(shm_id);
    remove_semaphore(sem_id);
    // remove_semaphore(sem_station_id[0]);
    // remove_semaphore(sem_station_id[1]);
    // remove_semaphore(sem_station_id[2]);

    return 0;
}

void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon)
{
    age = rand() % 60 + 10;          // wiek od 10 do 70 lat
    team = rand() % 2;               // Team 0 lub Team 1
    is_vip = (rand() % 10 == 0);     // 10% szans na vipa (będzie 0.5%, mod 200)
    has_weapon = (rand() % 20 == 0); // 5% szans, ze ma bron
}

void spawn_fan_process(const char *fan_program_path) // generujemy proces kibica
{
    int age, team;           // czy pelnoletni, z jakiej druzyny
    bool is_vip, has_weapon; // czy  jest vipem, czy ma bron
    // PID sciagniemy z forka(), frustracja domyslnie na 0

    generate_random_fan_attributes(age, team, is_vip, has_weapon); // generujemy atrybuty dla kibica

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("[Technician] Failed to fork");
        return;
    }
    if (pid == 0) // proces dziecka jest git i w fan.cpp bedziemy przetwarzac wejscie
    {
        // rzutujemy nasze atrybuty na char, zeby umiescic je w argumencie wywolania execl()
        char age_str[10], team_str[10], is_vip_str[10], has_weapon_str[10];
        snprintf(age_str, sizeof(age_str), "%d", age);
        snprintf(team_str, sizeof(team_str), "%d", team);
        snprintf(is_vip_str, sizeof(is_vip_str), "%d", is_vip);
        snprintf(has_weapon_str, sizeof(has_weapon_str), "%d", has_weapon);

        execl(fan_program_path, fan_program_path, age_str, team_str, is_vip_str, has_weapon_str, NULL); // jak sie uda to wskoczymy do przetwarzania
        perror("[Technician] Failed to exec fan process");                                              // jak nie to perror
        exit(1);                                                                                        // i exit
    }
    else
        std::cout << std::endl
                  << "[Technician] Spawned fan process with PID: " << pid
                  << " (Age: " << age
                  << ", Team: " << team
                  << ", VIP: " << (is_vip ? "Yes" : "No")
                  << ", Weapon: " << (has_weapon ? "Yes" : "No") << ")\n";
}