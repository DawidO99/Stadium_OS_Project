#define MAX_FANS 100 // maksymalna liczba kibicow
#define SHM_SIZE 1024   // Rozmiar pamięci współdzielonej
#define SHM_KEY 0x1235  // Klucz pamięci współdzielonej
#define SEM_KEY 0x1234  // Klucz semafora glownego

// klucze semaforow dla stanowisk kontrolnych
#define SEM_KEY_STATION_0 0x2000
#define SEM_KEY_STATION_1 0x2001
#define SEM_KEY_STATION_2 0x2002

// Offsety w stadium_data:
static const int OFFSET_TEAM_0 = MAX_FANS + 1;  // stationTeam[0]
static const int OFFSET_TEAM_1 = MAX_FANS + 2;  // stationTeam[1]
static const int OFFSET_TEAM_2 = MAX_FANS + 3;  // stationTeam[2]
static const int OFFSET_COUNT_0 = MAX_FANS + 4; // stationCount[0]
static const int OFFSET_COUNT_1 = MAX_FANS + 5; // stationCount[1]
static const int OFFSET_COUNT_2 = MAX_FANS + 6; // stationCount[2]
