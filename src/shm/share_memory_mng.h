#ifndef _SHM_SHARE_MEMORY_MGN_H_
#define _SHM_SHARE_MEMORY_MGN_H_

#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <cstring>
#include <atomic>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <memory>

namespace shm_mng {

    #define SMALL_SEG_PER_SIZE 160
    #define BIG_SEG_PER_SIZE 320

    void writer_common_delete_shm(const char* path, int project_id);
    int writer_common_create_shm(const char* path, int project_id, int seg_size, int count);
    int reader_common_attach_shm(const char* path, int project_id);

    void common_acquire_lock(std::atomic_int *lock);
    void common_release_lock(std::atomic_int *lock);
    void common_precise_sleep();
}
#endif