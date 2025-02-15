#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stddef.h>

// Semafory
int create_semaphore(key_t key, int num_semaphores, int initial_value);
void remove_semaphore(int sem_id);
void semaphore_wait(int sem_id, int sem_num);   // Procedura P - opuszczenie semafora (blokuje się, jeśli s==0) - "s--"
void semaphore_signal(int sem_id, int sem_num); // Procedura V - podniesienie semafora (odblokowuje dostep) - "s++"
int semaphore_trywait(int sem_id, int sem_num); // Procedura P z IPC_NOWAIT

// Pamięć współdzielona
int create_shared_memory(key_t key, size_t size);
void *attach_shared_memory(int shm_id);   // dolaczenie
void detach_shared_memory(void *shm_ptr); // odlaczenie
void remove_shared_memory(int shm_id);

// generowanie klucza
key_t generate_key(const char *path, int id);

#endif
