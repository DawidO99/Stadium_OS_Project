
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
#include <sys/wait.h>

// Zasoby IPC
int sem_id;                  // Semafor główny
int shm_id;                  // Pamięć dzielona
int sem_station_id;          // Zbiór semaforów dla stacji kontroli
int *stadium_data = nullptr; // pamiec dzielona zrzutowana na tablice intow

void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon); // generowanie atrybutow dla kibicow
void spawn_fan_process(const char *fan_program_path);                                     // generowanie kibicow forkiem
void signal_handler(int sig);                                                             // handler dla sygnalow

int main()
{
    srand(time(NULL)); // Do generowania atrybutów

    std::cout << "[Technician] Enter the number of fans trying to enter the stadium : " << std::endl;
    int n = -1;
    std::cin >> n; //uzytkownik podaje "z palca" liczbe procesow do wygenerowania
    while(n>250000 || n==-1)
    {
        std::cout << "[Technician] Allowed process limit exceeded. Please try again" << std::endl;
        std::cin >> n;  //na torusie ulimit = 257104, ale zaokraglimy do 250k
    }
        

    std::cout << "[Technician] Initializing resources...\n";

    std::signal(SIGALRM, SIG_IGN); // Ignorowanie SIGALRM

    // Rejestracja handlera sygnałów
    std::signal(SIGUSR1, signal_handler);
    std::signal(SIGUSR2, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Tworzenie semaforów
    sem_id = create_semaphore(SEM_KEY, 1, 1);                   // Semafor główny
    sem_station_id = create_semaphore(SEM_KEY_STATION_0, 3, 3); // Zbiór semaforów dla 3 stacji kontroli

    // Tworzenie pamięci współdzielonej
    shm_id = create_shared_memory(SHM_KEY, SHM_SIZE);
    void *shm_ptr = attach_shared_memory(shm_id);

    // Inicjalizacja pamięci współdzielonej
    stadium_data = static_cast<int *>(shm_ptr);
    stadium_data[0] = 0; // Liczba kibiców na stadionie
    for (int i = 1; i <= MAX_FANS; i++)
        stadium_data[i] = 0; // PID-y kibiców
    stadium_data[OFFSET_TEAM_0] = -1; // StationTeam[0]
    stadium_data[OFFSET_TEAM_1] = -1; // StationTeam[1]
    stadium_data[OFFSET_TEAM_2] = -1; // StationTeam[2]
    stadium_data[OFFSET_COUNT_0] = 0; // StationCount[0]
    stadium_data[OFFSET_COUNT_1] = 0; // StationCount[1]
    stadium_data[OFFSET_COUNT_2] = 0; // StationCount[2]

    stadium_data[OFFSET_COUNT_2 + 1] = getpid(); // zapisanie PID-u technician dla kierownika (manager)
    stadium_data[OFFSET_COUNT_2 + 2] = 1;        // flaga do sygnalow
    //OFFSET_COUNT_2 + 3 jest wypelniony w manager.cpp
    stadium_data[OFFSET_COUNT_2 + 4] = 0; // liczba dzieci na stadionie

    std::cout << "[Technician] Resources initialized successfully.\n";

    // Tworzenie procesów kibiców
    const char *fan_program_path = "./build/fan";
    bool stop=false;
    for (int i = 0; i < n && !stop; i++)
    {
        while (stadium_data[OFFSET_COUNT_2 + 2] != 1)
        {
            std::cout << "[Technician] Fans cannot enter. Please wait" << std::endl;
            //sleep(2);
            if(stadium_data[0]>=MAX_FANS)
            {
                stop=true;
                break;
            }      
        }
        if (stop) break; // Kończy pętlę for
        spawn_fan_process(fan_program_path);
        //sleep(1); // Opóźnienie między generowaniem kibiców
    }

    std::cout << "[Technician] All fans entered the stadium." << std::endl;
    kill(getpid(), SIGTERM);

    return 0;
}

void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon)
{
    age = rand() % 60 + 10;          // Wiek od 10 do 70 lat
    team = rand() % 2;               // Drużyna 0 lub 1
    is_vip = (rand() % 200 == 0);     // 0.5% szans na VIP-a
    has_weapon = (rand() % 20 == 0); // 5% szans na broń
}

void spawn_fan_process(const char *fan_program_path)
{
    int age, team;
    bool is_vip, has_weapon;
    generate_random_fan_attributes(age, team, is_vip, has_weapon);
    // zbieranie procesow zombie
    std::thread([]
                {
    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG); // Zbieramy zakończone procesy
    } })
        .detach(); // Uruchamiamy wątek jako niezależny

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

void signal_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        semaphore_wait(sem_id, 0);
        std::cout << "[Technician] Received SIGUSR1: Stopping fan entry.\n";
        stadium_data[OFFSET_COUNT_2 + 2] = 0; // Wyłącz generowanie kibiców
        semaphore_signal(sem_id, 0);
    }
    else if (sig == SIGUSR2)
    {
        semaphore_wait(sem_id, 0);
        std::cout << "[Technician] Received SIGUSR2: Resuming fan entry.\n";
        stadium_data[OFFSET_COUNT_2 + 2] = 1; // Włącz generowanie kibiców
        semaphore_signal(sem_id, 0);
    }
    else if (sig == SIGTERM)
    {
        semaphore_wait(sem_id, 0);
        stadium_data[OFFSET_COUNT_2 + 2] = 0; // Wyłącz generowanie kibiców
        semaphore_signal(sem_id, 0);
        for (int i = 0; i < 3; ++i)
            semaphore_wait(sem_station_id, i); // Zablokowanie semaforów dla stanowisk kontroli

        std::cout << "[Technician] Received SIGTERM: Evacuating fans.\n";
        semaphore_wait(sem_id, 0);
        for (int i = 1; i <= MAX_FANS; i++)
        {
            if (stadium_data[i] > 0)
            {
                std::cout << "[Technician] Fan with PID : " << stadium_data[i] << " is leaving the stadium" << std::endl;
                kill(stadium_data[i], SIGTERM); // Wysyłanie SIGTERM do fanów
                stadium_data[0]--;
                stadium_data[i] = 0;
            }
        }
        stadium_data[0] -= stadium_data[OFFSET_COUNT_2 + 4]; // wiemy, ze procesy z dziecmi wyszly, ale te dzieci tez trzeba odjac
        std::cout << "[Technician] Number of fans in the stadium : " << stadium_data[0] << std::endl;
        semaphore_signal(sem_id, 0);

        // Oczekiwanie na zakończenie procesów potomnych
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, 0)) > 0) {}
        if (pid == -1 && errno == ECHILD) 
            perror("[Technician] No more fan processes to wait for");
    
        semaphore_wait(sem_id, 0);
        if (stadium_data[0] == 0)
        {
            pid_t manager_pid = stadium_data[OFFSET_COUNT_2 + 3];
            if (kill(manager_pid, SIGALRM) == 0)
                std::cout << "[Technician] Informed manager: Stadium is empty.\n";
            else
                perror("[Technician] Failed to inform manager");
        }
        semaphore_signal(sem_id, 0);
        std::cout << "[Technician] Cleaning up shared resources and semaphores...\n";
        detach_shared_memory(stadium_data); // Odłącz pamięć współdzieloną
        remove_shared_memory(shm_id);       // Usuń pamięć współdzieloną
        remove_semaphore(sem_id);           // Usuń semafor główny
        remove_semaphore(sem_station_id);   // Usuń semafory dla stanowisk
        std::cout << "[Technician] All resources cleaned up. Exiting.\n";
        std::cout << "[Technician] All fans evacuated. Exiting.\n";
        exit(0);
    }
}