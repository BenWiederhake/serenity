#include <AK/Types.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int pipefds[2];

int main(int, char**)
{
    for (int i = 0; i < 8000; ++i) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        ASSERT(attr != 0);

        int rc = pthread_attr_setstacksize(&attr, 8 * MB);
        ASSERT(rc == 0);

        pthread_t tid;
        rc = pthread_create(
            &tid, &attr, [](void*) -> void* {
                return nullptr;
            },
            nullptr);
        // pthread_create leaks the stack (8 MB) in each call.
        ASSERT(rc == 0);
        pthread_attr_destroy(&attr);
        pthread_join(tid, nullptr);
        usleep(100000);
    }

    printf("PASS\n");
    return 0;
}
