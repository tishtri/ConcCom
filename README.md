## Peak-load benchmarks for POSIX semaphores in Linux kernel  ##

To obtain a peak-load benchmark for POSIX semaphores in Linux, we emulate the PVZ machine based peak-load benchmark described in [1] and implemented in [2], some theoretical background provided in [3].

We emulate System V Semaphores (SVS) by POSIX semaphores. Actually, we emulate a restricted set of functions - semget(), semctl(), semop() - which is used in the PVZ machine specification. In this simplified emulation, only one array of semaphores can be created, the following commands of semctl() are implemented - GETALL, SETALL, IPC\_RMID. The only correction imposed onto the PVZ machine is adding a suffix "\_posem" to all SVS functions calls. It actually switches SVS processing to our functions, specified on the top of PVZ machine. These functions issue POSIX semaphores system calls to emulate SVS functions, an array of POSIX semaphores created. 

We use a lock to implement semop(): sem\_wait(&lock) - semop() emulation - sem\_post(&lock). We read semaphores, check and apply `sops` one-by-one on an auxiliary array, initialized with the current semaphores values. If the check is satisfied, we implement `sops` via sem\_wait() and sem\_post(). If not satisfied, we repeat sem\_post(&lock) and sem\_wait(&lock) and re-check, supposing POSIX semaphores environment treats threads fairly. 

We run the System V semaphores (SVS) peak-load benchmarks on the emulated PVZ machine for threads with sparse matrix format of data as described in [2] and coded in [3] - run PTN as PVZ machine [2] using System V semaphores with semop() and threads created by pthread() [3] with sparse matrix specification of all `sops`.


### Benchmark POSIX semaphores in Linux kernel

Edit PVZ data file name in `#include` within `pvzm_posem_thread.c`

Compile and run `pvzm_posem_thread`

    >gcc -O3 -o pvzm_posem_thread pvzm_posem_thread.c -pthread
    >./pvzm_posem_thread
    or
    >./pvzm_posem_thread 1

A parameter (with the range 0-3) specifies the output detail level.


### References

1) Zaitsev, Dmitry and Naluba, Richard, PVZ Machine as a Formal System Based on Unix-Linux fork() and semop() System Calls (July 08, 2026). Preprint available at SSRN: [http://dx.doi.org/10.2139/ssrn.7606379]

2) [https://github.com/DimaZaitsev/SNCtools/bm5000x]

3) Zaitsev, D. A. (2026). Computing on semaphores of Dijkstra with place-transition nets. International Journal of Parallel, Emergent and Distributed Systems, 41(3), 291–320. [https://doi.org/10.1080/17445760.2026.2615010]

