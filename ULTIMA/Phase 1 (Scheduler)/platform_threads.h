#ifndef ULTIMA_PLATFORM_THREADS_H
#define ULTIMA_PLATFORM_THREADS_H

#include <pthread.h>
#include <ncurses.h>

#if defined(_WIN32) && !defined(ULTIMA_USE_NATIVE_PTHREADS)
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
#else
#include <pthread.h>
#endif

#endif // ULTIMA_PLATFORM_THREADS_H
