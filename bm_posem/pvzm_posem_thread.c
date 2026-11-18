// Stress load benchmark for POSIX semaphores in Linux via
// System V semaphores (restricted) emulated by POSIX semaphores 
// Restrictions: semget(), semctl(), semop() partially implemented
// KISS principle applied
// Only one array of semaphores can be created
// Following commands of semctl() are implemented: GETALL, SETALL, IPC_RMID
// We use a lock to implement semop(): sem_wait(&lock)
// We read semaphores and check and apply sops one-by-one
// If satisfied, we implements sops via sem_wait() and sem_post()
// If not satisfied, we repeat sem_post(&lock) and sem_wait(&lock) and check,
// supposing POSIX semaphores environment treats threads fairly

// We run System V semaphores (SVS) stress load benchmarks on
// PVZ machine for threads with sparse matrix format of data
// https://github.com/DimaZaitsev/SNCtools/bm5000x
// preprint

// Run PTN as PVZ machine (https://doi.org/10.1080/17445760.2026.2615010)
// using System V semaphores with semop() and threads created by pthread()
// v.3.0 alpha - sparse matrix specification of all sops

// Compile-build and run
// Edit the pvzm_data.h filename in pvzm_posem_thread.c
// $ gcc -O3 -o pvzm_posem_thread pvzm_posem_thread.c -pthread
// $ ./pvzm_posem_thread 2

// Useful commands:
// observe futex system calls
// $ strace -f -e trace=futex ./pvzm_posem_thread 1
// observe thread/process creation system calls
// $  strace -f -e trace=clone,clone3 ./pvzm_posem_thread 1

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <semaphore.h>

#include <limits.h>
#include <sys/time.h>


// !!!!!!!!!!!!!!!!!!!! include PVZ machine data .h file name here !!!!!!!!!!!!!!!!!!!!
#include "gm16x16-b6.h"


int pls_sem;
int t_arg[N]; // memory for thread arguments
char err_buf[256];
int deb=0;


// ***** emulate SVS by POSIX semaphores (POsem)

sem_t * posem = NULL;
unsigned short * curs;
int nsems;
sem_t lock;

int semget_posem (key_t __key, int __nsems, int __semflg) { // __key and __semflg are ignored

  if( posem != NULL) {
    errno=ENOMEM;
    return(-1);
  }
  
  nsems=__nsems;
  posem = (sem_t *) malloc(nsems * sizeof(sem_t));
  curs = (unsigned short *) malloc(nsems * sizeof(unsigned short *));
  
  if(posem == NULL || curs == NULL) {
    errno=ENOMEM;
    return(-1);
  }  
  else {
    sem_init(&lock, 0, 1);
if(deb>2) printf("=== semget_posem \n");
    //sem_init(&change, 0, 1);
    return(0);
  }
  
} // semget_posem


int semctl_posem (int __semid, int __semnum, int __cmd, ...) { // __semid and __semnum are ignored
  va_list args;
  unsigned short * svs;
  int i, val;
  
  if( __cmd == GETALL ) {
    //sem_wait(&lock);
    va_start(args,__cmd);
    svs = va_arg(args,unsigned short *);
    va_end(args);
    for(i=0;i<nsems;i++) { sem_getvalue(posem+i, &val ); svs[i] = val;}
if(deb>2) printf("=== semctl_posem: GETALL\n");
    //sem_post(&lock);
    return(0);
    
  } else if( __cmd == SETALL ) {
     sem_wait(&lock);
     va_start(args,__cmd);
     svs = va_arg(args,unsigned short *);
     va_end(args);
     for(i=0;i<nsems;i++) { val=svs[i]; sem_init(posem+i, 0, val); }
if(deb>2) printf("=== semctl_posem: SETALL\n");  
     sem_post(&lock);
     return(0);
     
  } else if( __cmd == IPC_RMID ) {
     sem_wait(&lock);
     free(posem);
     posem=NULL;
if(deb>2) printf("=== semctl_posem: IPC_RMID\n");
     sem_post(&lock);
     return(0);
     
  } else {
    errno=EINVAL;
    return(-1);
  }
  
} // semctl_posem


int semop_posem (int __semid, struct sembuf *__sops, size_t __nsops){ // __semid is ignored 
  int all, sopsi, i, semi;

  do {
    sem_wait(&lock);
if(deb>2) printf("=== semop_posem: got lock \n"); fflush(stdout); 
if(deb>2) for(i=0;i<__nsops;i++) printf("num=%d, op=%d\n",__sops[i].sem_num,__sops[i].sem_op);
    // check P and Z
    semctl_posem (pls_sem, 0, GETALL, curs);
if(deb>2) printf("=== semop_posem: got vals \n"); fflush(stdout);
if(deb>2) for(i=0;i<nsems;i++) printf("curs[%d]=%d\n",i,curs[i]);
    all=1;
    for(sopsi=0; sopsi < __nsops; sopsi++) {
      semi = __sops[sopsi].sem_num;
      if( __sops[sopsi].sem_op < 0  && curs[semi] < - __sops[sopsi].sem_op ||
          __sops[sopsi].sem_op == 0 && curs[semi] != 0 ) {
        all=0;
        break;
      }
    } // for
    curs[semi] += __sops[sopsi].sem_op;  
if(deb>2) printf("=== changed: curs[%d]=%d\n",semi,curs[semi]);
    if(!all) {
      sem_post(&lock);
if(deb>2) printf("=== semop_posem: released lock \n"); fflush(stdout);
      //sleep(1);
    }
  } while (!all);
   
   // implement P and V
if(deb>2) printf("=== semop_posem: implement \n"); fflush(stdout);
   for(sopsi=0; sopsi < __nsops; sopsi++) {
      if(__sops[sopsi].sem_op < 0) {
if(deb>2) printf("=== semop_posem: implement P(%d) \n",__sops[sopsi].sem_num); fflush(stdout);
        for(i = 0; i < -__sops[sopsi].sem_op; i++) sem_wait(&posem[__sops[sopsi].sem_num]);
      } else if(__sops[sopsi].sem_op == 0) {
if(deb>2) printf("=== semop_posem: implement Z(%d) \n",__sops[sopsi].sem_num); fflush(stdout);
        continue;
      } else { // > 0
if(deb>2) printf("=== semop_posem: implement V(%d) \n",__sops[sopsi].sem_num); fflush(stdout);
        for(i = 0; i < __sops[sopsi].sem_op; i++) sem_post(&posem[__sops[sopsi].sem_num]);
      } 
  } // for
  
  //sem_post(&change); 
  sem_post(&lock);
if(deb>2) printf("=== semop_posem: post lock \n"); fflush(stdout);   
  return(0);
  
} // semop_posem

// ***** ENF OF emulate SVS by POSIX sem
// @ 2026, Tatiana Shmeleva tatianar.shmeleva@gmail.com

// ***** Emulated PVZ machine, 
// Inserted from: https://github.com/DimaZaitsev/SNCtools/bm5000x
// suffix "_posem" added to SVS function calls

double seconds()
{
    struct timeval tp;
    struct timezone tzp;
    int i = gettimeofday(&tp, &tzp);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}

void prn_mu() { // print marking (semaphores)
    // Get marking
    if (semctl_posem (pls_sem, 0, GETALL, s) == -1) {
        perror ("semctl GETALL mu"); exit (1);
    }
    printf("mu: ");
    for(int p=0;p<M;p++) printf("%d ",s[p]);
    printf("\n");
    fflush(stdout);
} // end of prn_mu()

void prn_mu_t(int t) { // print marking (semaphores)
    // Get marking
    if (semctl_posem (pls_sem, 0, GETALL, s) == -1) {
        perror ("semctl GETALL mu"); exit (1);
    }
    printf("t%d => mu: ", t);
    for(int p=0;p<M;p++) printf("%d ",s[p]);
    printf("\n");
    fflush(stdout);
} // end of prn_mu_t()

void *transition (void *arg)
{
  int t = *((int *) arg);

  for(;;) {

    // Group P/V operation on places (semaphores)

    if (semop_posem (pls_sem, sops+sops_idx[t], sops_idx[t+1]-sops_idx[t]) == -1) {
      sprintf(err_buf,"semop: Group P/V/Z operation on places t_pls[%d]",t);
      perror (err_buf); exit (1);
    }

    if (deb == 2) printf("t%d ", t ); else
    if (deb > 2) prn_mu_t(t);
    fflush(stdout);

  } // for(;;)

} // end of transition()

int main (int argc, char **argv)
{
    pthread_t tid_trs [N];
    int t, r;
    double t1, dt;
    
    if(argc>1) deb=atoi(argv[1]);

    //  semafore array as the current place marking
    // create a set of M semaphores
    if ((pls_sem = semget_posem (IPC_PRIVATE, M, 0660 | IPC_CREAT)) == -1) {
        perror ("semget"); exit (1);
    }
    // assign initial marking
    if (semctl_posem (pls_sem, 0, SETALL, s) == -1) {
        perror ("semctl SETALL"); exit (1);
    }
    if(deb>0) { printf("init "); prn_mu(); }

    // create N transition threads
    for (t = 0; t < N; t++) {
        t_arg[t] = t;
        if ((r = pthread_create (&tid_trs [t], NULL, transition, (void *) &t_arg[t] )) != 0) {
            fprintf (stderr, "Error = %d (%s)\n", r, strerror (r)); exit (1);
        }
    }
    
    t1=seconds();
    // check termination condition
    if (semop_posem (pls_sem, fin_sops, fin_sops_idx[1]-fin_sops_idx[0]) == -1) {
      perror ("semop: P0 on fin_pls"); exit (1);
    }
    dt=seconds()-t1;
    
    sleep(1); 
    
    if(deb>0) { printf("\nfin  "); prn_mu(); }
    
    printf("%f ",dt);
    if(deb>0) printf("\n");
        
    // cancel threads
    for (t = 0; t < N; t++) {
      if ((r = pthread_cancel (tid_trs [t])) != 0) {
        fprintf (stderr, "Error = %d (%s)\n", r, strerror (r)); exit (1);
      }
    }
    
    // delete set of semaphores
    if (semctl_posem (pls_sem, 0, IPC_RMID) == -1) {
        perror ("semctl IPC_RMID"); exit (1);
    }

    exit (0);
} // end of main()

// ***** END OF Emulated PVZ machine


