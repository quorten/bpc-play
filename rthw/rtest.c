/* Very simple realtime thread test code.  */
#include <unistd.h>
#include <limits.h>
#include <sys/mman.h>
#define __USE_UNIX98 /* Required for pthread_mutexattr... */
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <time.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

unsigned tick_hit_count = 0;
unsigned tick_miss_count = 0;
unsigned tick_unknown_count = 0;
/* Set to nonzero to signal an exit condition to the thread.  */
unsigned tick_exit = 0;
pthread_mutex_t tick_mutex;

void *
thread_func(void *data)
{
  /* Do something in a 10 ms realtime loop.  */
  /* TODO: Enhance our timer sleep code to properly measure missed
     intervals.  */
  static unsigned local_miss = 0;
  static unsigned local_unknown = 0;
  while (!tick_exit) {
    int result;
    struct timespec sleep_time = { 0, 10000000 };
    struct timespec remain;
    result = pthread_mutex_trylock (&tick_mutex);
    if (result == 0) {
      if (tick_exit) {
        pthread_mutex_unlock (&tick_mutex);
        break;
      }
      tick_hit_count++;
      tick_miss_count += local_miss;
      local_miss = 0;
      tick_unknown_count += local_unknown;
      local_unknown = 0;
      pthread_mutex_unlock (&tick_mutex);
    } else if (result == EBUSY) {
      local_miss++;
    } else {
      local_unknown++;
    }
    while (sleep_time.tv_sec != 0 && sleep_time.tv_nsec != 0) {
      int result;
      remain.tv_sec = sleep_time.tv_sec;
      remain.tv_nsec = sleep_time.tv_nsec;
      result = clock_nanosleep (CLOCK_MONOTONIC, 0 /* TIMER_RELTIME */,
                                &sleep_time, &remain);
      if (result == 0)
        break;
      sleep_time.tv_sec = remain.tv_sec;
      sleep_time.tv_nsec = remain.tv_nsec;
    }
    pthread_yield ();
  }
  pthread_exit ((void *)0);
}

/* Thread ID of the realtime worker thread.  */
pthread_t rt_tid;

int
main (int argc, char *argv)
{
  mlockall (MCL_CURRENT | MCL_FUTURE);

  /* Parse command line arguments...  */

  { /* Create our realtime thread.  */
    int result;
    pthread_mutexattr_t rtmx_attr;
    struct sched_param rt_sched_param;
    pthread_attr_t rt_attr;
    pid_t pid;
    struct sched_param sp;
    void *arg = NULL; /* Pass a parameter to the thread */

    result = pthread_mutexattr_init (&rtmx_attr);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_mutexattr_init: %s\n", strerror (result));
      return 1;
    }
    result = pthread_mutexattr_setprotocol (&rtmx_attr, PTHREAD_PRIO_INHERIT);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_mutexattr_setprotocol: %s\n", strerror (result));
      return 1;
    }
    result = pthread_mutex_init (&tick_mutex, &rtmx_attr);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_mutex_init: %s\n", strerror (result));
      return 1;
    }

    rt_sched_param.sched_priority = /* SCHED_OTHER */ SCHED_RR;
    result = pthread_attr_init (&rt_attr);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_attr_init: %s\n", strerror (result));
      return 1;
    }
    result = pthread_attr_setinheritsched
      (&rt_attr, PTHREAD_INHERIT_SCHED /* PTHREAD_EXPLICIT_SCHED */);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_attr_setinheritsched: %s\n", strerror (result));
      return 1;
    }
    /* Sadly this does not work, so we just skip it.  Instead, we mess
       around with Linux-specific `sched_setscheduler()'.  */
    /* result = pthread_attr_setschedparam (&rt_attr, &rt_sched_param);
    if (result != 0) {
      /\* Handle error *\/
      printf ("pthread_attr_setschedparam: %s\n", strerror (result));
      return 1;
    } */

    pid = getpid ();
    sp.sched_priority = 20;
    if (sched_setscheduler (pid, SCHED_RR, &sp) != 0) {
      perror ("sched_setscheduler");
    }

    result = pthread_create (&rt_tid, &rt_attr, thread_func, arg);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_create: %s\n", strerror (result));
      return 1;
    }

    sp.sched_priority = 0;
    if (sched_setscheduler (pid, SCHED_OTHER, &sp) != 0) {
      perror ("sched_setscheduler");
    }

    /* result = pthread_attr_destroy (&rt_attr);
    if (result != 0) {
      /\* Handle error *\/
      printf ("pthread_attr_destroy: %s\n", strerror (result));
      return 1;
    }
    result = pthread_mutexattr_destroy (&rtmx_attr);
    if (result != 0) {
      /\* Handle error *\/
      printf ("pthread_mutexattr_destroy: %s\n", strerror (result));
      return 1;
    } */
  }

  { /* Do something useful...  */
    unsigned i;
    for (i = 0; i < 10; i++) {
      unsigned local_hit, local_miss, local_unknown;
      pthread_mutex_lock (&tick_mutex);
      local_hit = tick_hit_count;
      local_miss = tick_miss_count;
      local_unknown = tick_unknown_count;
      pthread_mutex_unlock (&tick_mutex);
      printf ("Hit count: %u, Miss count: %u, Unknown count: %u\n",
              local_hit, local_miss, local_unknown);
      sleep (1);
    }
  }

  /* Signal the thread that it's time to exit.  */
  /* pthread_kill (rt_tid, SIGINT); */
  pthread_mutex_lock (&tick_mutex);
  tick_exit = 1;
  pthread_mutex_unlock (&tick_mutex);

  { /* Wait for the thread to exit gracefully.  */
    int result;
    void *status;
    result = pthread_join (rt_tid, &status);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_join: %s\n", strerror (result));
    }
    /* If applicable, check status and act accordingly.  */
  }

  {
    int result = pthread_mutex_destroy (&tick_mutex);
    if (result != 0) {
      /* Handle error */
      printf ("pthread_mutex_destroy: %s\n", strerror (result));
    }
  }

  /* Use `pthread_exit()' to exit `main()' since we are using
     `pthread` threads elsewhere.  */
  pthread_exit ((void *)0);
}
