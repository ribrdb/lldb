#include <unistd.h>
int
infloop ()
{
    int loop = 1;
    while (loop > 0) {
        if (loop > 10) {
            sleep(1);
            loop = 1;
        }
        loop++; // Set break point at this line.
    }
    return loop;
}
