#include "../include/ipc_utils.h"
#include "../include/constants.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <ctime>

// Zasoby IPC
int sem_id;
int shm_id;

void terminate_fan_processes();                                                           // wysylanie sygnalu kill do wszystkich procesow
void signal_handler(int sig);                                                             // obsluga sygnalow
void spawn_fan_process(const char *fan_program_path);                                     // generowanie procesu fana
void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon); // generowanie atrybutow dla procesu fana

int main()
{
    srand(time(NULL));
    const char *fan_program_path = "./build/fan";

    if (std::signal(SIGUSR1, signal_handler) == SIG_ERR)
        perror("[Technician] Failed to set SIGUSR1 handler");
    if (std::signal(SIGUSR2, signal_handler) == SIG_ERR)
        perror("[Technician] Failed to set SIGUSR2 handler");
    if (std::signal(SIGTERM, signal_handler) == SIG_ERR)
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

    // Tworzenie kilku procesów fan (na przykład 5)
    for (int i = 0; i < MAX_FANS; i++)
    {
        spawn_fan_process(fan_program_path);
        sleep(1); // Opcjonalnie dodajemy opóźnienie między tworzeniem procesów
    }

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
        terminate_fan_processes();
        std::cout << "[Technician] Cleaning up resources and exiting.\n";
        exit(0);
    }
}

void spawn_fan_process(const char *fan_program_path)
{
    int age, team;
    bool is_vip, has_weapon;

    // Generujemy losowe atrybuty kibica
    generate_random_fan_attributes(age, team, is_vip, has_weapon);

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("[Technician] Failed to fork");
        return;
    }

    if (pid == 0)
    {
        // Proces dziecka
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

void generate_random_fan_attributes(int &age, int &team, bool &is_vip, bool &has_weapon)
{
    age = rand() % 40 + 10;          // Wiek od 10 do 50 lat
    team = rand() % 2;               // Drużyna 0 lub 1
    is_vip = (rand() % 10 == 0);     // 10% szans na VIP-a
    has_weapon = (rand() % 20 == 0); // 5% szans na posiadanie broni
}