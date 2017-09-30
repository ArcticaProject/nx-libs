#include <cstddef>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <vector>
#include <cstring>

#include "logging_test.h"

Faulty_Logger faulty_logger;
NXLog good_logger;

void print_sigmask () {
  sigset_t orig_mask;
  sigemptyset (&orig_mask);

  pthread_sigmask (SIG_SETMASK, NULL, &orig_mask);

  bool empty = true;
  for (std::size_t i = 0; i < NSIG; ++i) {
    if (sigismember (&orig_mask, i)) {
      nxdbg_good << "Signal i (" << i << ") in signal mask." << std::endl;
      empty = false;
    }
  }

  if (empty) {
    nxdbg_good << "Signal mask empty.";
  }
}

void* log_task (void* /* unused */) {
  /* print_sigmask (); */

  for (std::size_t i = 0; i < 10; ++i) {
    nxinfo << "Log message " << i << std::endl;
  }
}

void sig_handler (int signo) {
  nxinfo << "Received signal " << signo << std::endl;
}

void setup_faulty_logger () {
  faulty_logger.log_level (true);
  faulty_logger.log_unix_time (false);
  faulty_logger.log_time (true);
  faulty_logger.log_location (true);
  faulty_logger.log_thread_id (true);
  faulty_logger.level (NXDEBUG);
}

void setup_good_logger () {
  good_logger.log_level (true);
  good_logger.log_unix_time (false);
  good_logger.log_time (true);
  good_logger.log_location (true);
  good_logger.log_thread_id (true);
  good_logger.level (NXDEBUG);
}

pthread_t spawn_thread () {
  pthread_t thread_id;
  int pthread_ret;

  sigset_t block_mask, orig_mask;
  sigemptyset (&orig_mask);
  sigfillset (&block_mask);

  pthread_sigmask (SIG_BLOCK, &block_mask, &orig_mask);

  pthread_ret = pthread_create (&thread_id, NULL, log_task, NULL);

  pthread_sigmask (SIG_SETMASK, &orig_mask, NULL);

  return (thread_id);
}

void install_signal_handler () {
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (-1 == sigaction (SIGUSR1, &sa, NULL)) {
    nxerr_good << "Unable to install signal handler!" << std::endl;
  }
  else {
    nxdbg_good << "Signal handler registered successfully for SIGUSR1." << std::endl;
  }
}

void killing_process_work (pid_t parent_pid) {
  /* Seed PRNG. */
  std::srand (std::time (0));

  for (std::size_t i = 0; i < 25; ++i) {
    /* Sleep for 4 seconds + some random number up to a second. */
    std::size_t rand_add = (std::rand () % 1000000);
    usleep (4000000 + rand_add);

    /* Send SIGUSR1 to parent process. */
    nxdbg_good << "Sending SIGUSR1 (" << SIGUSR1 << ") to parent_pid (" << parent_pid << ")" << std::endl;

    if (kill (parent_pid, SIGUSR1)) {
      int saved_errno = errno;
      nxerr_good << "Failed to deliver signal to parent, aborting." << std::endl;
      nxerr_good << "Error " << saved_errno << ": " << strerror (saved_errno) << std::endl;
      exit (EXIT_FAILURE);
    }
  }

  exit (EXIT_SUCCESS);
}

void killing_process_init (int argc, char **argv) {
  /* We're in the "killing process". */
  pid_t parent_pid = getppid ();

  setup_good_logger ();

  for (std::size_t i = 0; i < argc; ++i) {
    nxdbg_good << "argv[" << i << "]: " << argv[i] << std::endl;
  }

  char *end = NULL;

  errno = 0;
  long parent_pid_check = std::strtol (argv[1], &end, 0);

  if ((errno == ERANGE) && (parent_pid_check == LONG_MAX)) {
    /* Overflow, handle gracefully. */
    parent_pid_check = 1;
  }

  if ((errno == ERANGE) && (parent_pid_check == LONG_MIN)) {
    /* Underflow, handle gracefully. */
    parent_pid_check = 1;
  }

  if (*end) {
    /* Conversion error (for inputs like "<number>X", end will point to X.) */
    parent_pid_check = 1;
  }

  if (parent_pid != parent_pid_check) {
    nxinfo_good << "Parent PID verification via first argument failed, trusting getppid ()." << std::endl;
  }

  killing_process_work (parent_pid);
}

int main (int argc, char **argv) {
  if (argc > 1) {
    killing_process_init (argc, argv);
  }
  else {
    /* That's the main process. */

    /* First, fork and create the "killing process". */
    pid_t pid = fork ();

    if (0 == pid) {
      /* Child process. */
      pid_t parent_pid = getppid ();

      /* Prepare to pass-through parent PID. */
      std::stringstream ss;
      ss << parent_pid;

      std::vector<std::string> new_argv;
      new_argv.push_back (std::string (argv[0]));
      new_argv.push_back (ss.str ());

      std::vector<char *> new_argv_c_str;
      for (std::vector<std::string>::iterator it = new_argv.begin (); it != new_argv.end (); ++it) {
        const char *elem = (*it).c_str ();
        new_argv_c_str.push_back (strndup (elem, std::strlen (elem)));
      }

      /* Add null pointer as last element. */
      new_argv_c_str.push_back (0);

      /* Relaunch, with argv[1] containing the ppid. */
      if (0 != execvp (new_argv_c_str.front (), &(new_argv_c_str.front ()))) {
        const int saved_errno = errno;
        std::cerr << "Failed to start \"killing process\"! Panic!" << std::endl;
        std::cerr << "System error: " << std::strerror (saved_errno) << std::endl;
        exit (EXIT_FAILURE);
      }
    }
    else if (0 > pid) {
      const int saved_errno = errno;
      std::cerr << "Error while forking main process! Panic!" << std::endl;
      std::cerr << "System error: " << std::strerror (saved_errno) << std::endl;
      exit (EXIT_FAILURE);
    }
    else {
      /* Main process. */
      /* Falls through to general code below. */
    }
  }

  setup_faulty_logger ();

  pthread_t thread_id = spawn_thread ();

  setup_good_logger ();

  install_signal_handler ();

  /* print_sigmask (); */

  log_task (NULL);

  pthread_join (thread_id, NULL);

  exit (EXIT_SUCCESS);
}
