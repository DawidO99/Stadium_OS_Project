#include "../include/ipc_utils.h"
#include <stdio.h>

int main()
{
    // Tworzenie semafora
    int sem_id = create_semaphore(0x1234, 1);
    if (sem_id == -1)
        return 1; // Błąd w semaforze
    printf("Semaphore created with ID: %d\n", sem_id);

    // Test semafora
    semaphore_wait(sem_id);
    printf("Semaphore wait successful.\n");
    semaphore_signal(sem_id);
    printf("Semaphore signal successful.\n");

    // Usuwanie semafora
    remove_semaphore(sem_id);
    printf("Semaphore removed.\n");

    // Tworzenie pamięci współdzielonej
    int shm_id = create_shared_memory(0x1235, 1024);
    if (shm_id == -1)
        return 1; // Błąd w pamięci współdzielonej
    printf("Shared memory created with ID: %d\n", shm_id);

    void *shm_ptr = attach_shared_memory(shm_id);
    if (shm_ptr == NULL)
        return 1; // Błąd w dołączaniu pamięci
    printf("Shared memory attached at address: %p\n", shm_ptr);

    detach_shared_memory(shm_ptr);
    printf("Shared memory detached.\n");

    remove_shared_memory(shm_id);
    printf("Shared memory removed.\n");

    return 0;
}
