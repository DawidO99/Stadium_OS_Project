#include "../../include/ipc_utils.h"
#include <stdio.h>
#include <errno.h>

// Semafory
int create_semaphore(key_t key, int initial_value)
{
    int sem_id = semget(key, 1, IPC_CREAT | 0600); // minimalne prawa dostepu
    if (sem_id == -1)
    {
        perror("Failed to create semaphore");
        return -1;
    }
    if (semctl(sem_id, 0, SETVAL, initial_value) == -1)
    {
        perror("Failed to initialize semaphore");
        return -1;
    }
    return sem_id;
}

void remove_semaphore(int sem_id)
{
    if (semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("Failed to remove semaphore");
    }
}

void semaphore_wait(int sem_id)
{
    struct sembuf op = {0, -1, 0};
    if (semop(sem_id, &op, 1) == -1)
    {
        perror("Failed to wait on semaphore");
    }
}

void semaphore_signal(int sem_id)
{
    struct sembuf op = {0, 1, 0};
    if (semop(sem_id, &op, 1) == -1)
    {
        perror("Failed to signal semaphore");
    }
}

// Pamięć współdzielona
int create_shared_memory(key_t key, size_t size)
{
    int shm_id = shmget(key, size, IPC_CREAT | 0600);
    if (shm_id == -1)
    {
        perror("Failed to create shared memory");
        return -1;
    }
    return shm_id;
}

void *attach_shared_memory(int shm_id)
{
    void *shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1)
    {
        perror("Failed to attach shared memory");
        return NULL;
    }
    return shm_ptr;
}

void detach_shared_memory(void *shm_ptr)
{
    if (shmdt(shm_ptr) == -1)
    {
        perror("Failed to detach shared memory");
    }
}

void remove_shared_memory(int shm_id)
{
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove shared memory");
    }
}

// Kolejki komunikatów
int create_message_queue(key_t key)
{
    int msg_id = msgget(key, IPC_CREAT | 0600);
    if (msg_id == -1)
    {
        perror("Failed to create message queue");
        return -1;
    }
    return msg_id;
}

void remove_message_queue(int msg_id)
{
    if (msgctl(msg_id, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove message queue");
    }
}
