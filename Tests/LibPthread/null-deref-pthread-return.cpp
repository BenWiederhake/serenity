#include <pthread.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int pipefds[2];

int main(int, char**)
{
    pthread_t tid;
    int rc = pthread_create(
        &tid, nullptr, [](void*) -> void* {
            char anything;
            printf("Some thread stack var is at %p\n", &anything);
            usleep(50);
            return nullptr;
        },
        nullptr);
    if (rc) {
        printf("FAIL: pthread_create rc: %d\n", rc);
        return 1;
    }

    usleep(2000);
    printf("Joining\n");
    rc = pthread_join(tid, nullptr);
    if (rc)
        printf("Weird pthread_join rc: %d\n", rc);

    printf("PASS\n");
    return 0;
}
