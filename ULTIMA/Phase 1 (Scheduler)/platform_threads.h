/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#ifndef ULTIMA_PLATFORM_THREADS_H
#define ULTIMA_PLATFORM_THREADS_H

#if defined(__has_include)
#if __has_include(<pthread.h>)
#include <pthread.h>
#define ULTIMA_HAS_PTHREAD_HEADER 1
#else
#include <mutex>

typedef struct {
    std::mutex native_mutex;
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER {}

inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
    mutex->native_mutex.lock();
    return 0;
}

inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    mutex->native_mutex.unlock();
    return 0;
}

inline int pthread_mutex_destroy(pthread_mutex_t *) {
    return 0;
}
#endif
#else
#include <pthread.h>
#define ULTIMA_HAS_PTHREAD_HEADER 1
#endif

#endif // ULTIMA_PLATFORM_THREADS_H
