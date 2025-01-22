#include "ipc_utils.h"

#define MAX_FANS 1000                                             // maksymalna liczba kibicow
#define SHM_SIZE sizeof(int) * (MAX_FANS + 100)                 // Rozmiar pamięci współdzielonej
const key_t SHM_KEY = generate_key("/etc/passwd", 1);           // Klucz pamięci współdzielonej
const key_t SEM_KEY = generate_key("/etc/passwd", 2);           // Klucz semafora glownego
const key_t SEM_KEY_STATION_0 = generate_key("/etc/passwd", 3); // Klucz semaforow dla stanowisk kontrolnych

// Offsety w stadium_data:
static const int OFFSET_TEAM_0 = MAX_FANS + 1;  // stationTeam[0]
static const int OFFSET_TEAM_1 = MAX_FANS + 2;  // stationTeam[1]
static const int OFFSET_TEAM_2 = MAX_FANS + 3;  // stationTeam[2]
static const int OFFSET_COUNT_0 = MAX_FANS + 4; // stationCount[0]
static const int OFFSET_COUNT_1 = MAX_FANS + 5; // stationCount[1]
static const int OFFSET_COUNT_2 = MAX_FANS + 6; // stationCount[2]
                                                // reszta offsetow jest wzgledem OFFSET_COUNT_2 i jest odpowiednio wykomentowana w kodzie