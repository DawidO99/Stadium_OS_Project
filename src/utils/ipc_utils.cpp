#include "../../include/ipc_utils.h"
#include <stdio.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Tworzenie zbioru semaforów
int create_semaphore(key_t key, int num_semaphores, int initial_value)
{
    int sem_id = semget(key, num_semaphores, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1)
    {
        if (errno == EEXIST)
        {
            sem_id = semget(key, num_semaphores, 0);
            if (sem_id == -1)
            {
                perror("Failed to open existing semaphore set");
                return -1;
            }
            return sem_id;
        }
        else
        {
            perror("Failed to create semaphore set");
            return -1;
        }
    }

    // Inicjalizacja semaforów w zestawie
    for (int i = 0; i < num_semaphores; ++i)
    {
        if (semctl(sem_id, i, SETVAL, initial_value) == -1)
        {
            perror("Failed to initialize semaphore in set");
            remove_semaphore(sem_id);
            return -1;
        }
    }
    return sem_id;
}

// Usuwanie zbioru semaforów
void remove_semaphore(int sem_id)
{
    if (semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("Failed to remove semaphore set");
    }
}

// Operacje na semaforach w zestawie
void semaphore_wait(int sem_id, int sem_num)
{
    struct sembuf op = {sem_num, -1, SEM_UNDO};
    if (semop(sem_id, &op, 1) == -1)
    {
        perror("Failed to wait on semaphore");
    }
}

void semaphore_signal(int sem_id, int sem_num)
{
    struct sembuf op = {sem_num, 1, SEM_UNDO};
    if (semop(sem_id, &op, 1) == -1)
    {
        perror("Failed to signal semaphore");
    }
}

int semaphore_trywait(int sem_id, int sem_num)
{
    struct sembuf op = {sem_num, -1, IPC_NOWAIT | SEM_UNDO};
    int ret;
    if ((ret = semop(sem_id, &op, 1)) == -1)
    {
        if (errno == EAGAIN)
        {
            fprintf(stderr, "Semaphore %d is unavailable (non-blocking trywait failed).\n", sem_num);
            return 0;
        }
        else
        {
            perror("Failed to perform semaphore trywait");
            return -1;
        }
    }
    return ret;
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

// ganerowanie klucza
key_t generate_key(const char *path, int id)
{
    key_t key = ftok(path, id);
    if (key == -1)
    {
        perror("[KeyGen] Failed to generate key using ftok");
        exit(EXIT_FAILURE);
    }
    return key;
}