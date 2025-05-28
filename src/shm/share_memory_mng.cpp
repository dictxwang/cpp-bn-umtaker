#include "share_memory_mng.h"
#include <iostream>
#include <errno.h>

namespace shm_mng {

    void writer_common_delete_shm(const char* path, int project_id) {
        int shm_id;
        key_t key;
        struct shmid_ds buf;

        key = ftok(path, project_id);
        if (key == -1) {
            return;
        }

        shm_id = shmget(key, 0, 0);
        if (shm_id == -1) {
            return;
        }

        // delete the share memory
        shmctl(shm_id, IPC_RMID, &buf);
    }

    int writer_common_create_shm(const char* path, int project_id, int seg_size, int count) {
        
        writer_common_delete_shm(path, project_id);

        int shm_id;
        key_t key;

        key = ftok(path, project_id);
        if (key == -1) {
            std::cerr << "ftok failed with errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
            return -1;
        }

        //创建共享内存
        shm_id = shmget(key, seg_size*count, IPC_CREAT | IPC_EXCL | 0666);
        if (shm_id == -1) {
            std::cerr << "shmget failed with errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
            return -2;
        }

        // detach
        // if (shmdt(tickers) == -1) {
        //     return -3;
        // }
        return shm_id;
    }

    int reader_common_attach_shm(const char* path, int project_id) {
        int shm_id;
        key_t key;

        key = ftok(path, project_id);
        if (key == -1) {
            std::cerr << "ftok failed with errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
            return -1;
        }

        shm_id = shmget(key, 0, 0);
        if (shm_id == -1) {
            std::cerr << "shmget failed with errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
            return -2;
        }

        return shm_id;
    }

    void common_acquire_lock(std::atomic_int *lock) {
        while (atomic_exchange_explicit(lock, 1, std::memory_order_acquire) != 0) {
            common_precise_sleep(); // Just to avoid burning too much CPU in a busy loop
        }
    }

    void common_release_lock(std::atomic_int *lock) {
        atomic_store_explicit(lock, 0, std::memory_order_release);  // Release the lock
    }

    void common_precise_sleep() {
        struct timespec req = {0};
        req.tv_sec = 0; // 0 seconds
        req.tv_nsec = 1; // 1 nanoseconds

        nanosleep(&req, NULL);
    }
}