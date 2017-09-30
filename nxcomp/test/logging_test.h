#ifndef LOGGING_TEST_H
#define LOGGING_TEST_H

#include <unistd.h>

#define INTERNAL_LOGGING_TEST
#include "Log.h"

class Faulty_Logger : public NXLog {
    /* Copied from base class, inserted "fault" within critical section. */
    using NXLog::flush;
    void flush(per_thread_data *pdt)
    {
        sigset_t orig_signal_mask,
                 tmp_signal_mask;
        sigemptyset(&orig_signal_mask);

        sigfillset(&tmp_signal_mask);

        pthread_sigmask(SIG_BLOCK, &tmp_signal_mask, &orig_signal_mask);

        if (!pdt->buffer.empty ()) {
            const std::string str = pdt->buffer.top()->str();

            if (!str.empty())
            {
                pthread_mutex_lock(&output_lock_);
                usleep (3000000);
                (*stream()) << str;
                pthread_mutex_unlock(&output_lock_);
            }

            pdt->buffer.pop();
        }

        pthread_sigmask(SIG_SETMASK, &orig_signal_mask, NULL);
    }

    template<typename T>
    friend Faulty_Logger& operator<<(Faulty_Logger& out, const T& value);

    friend Faulty_Logger& operator<< (Faulty_Logger& out, const NXLogStamp& value);
};

template <typename T>
Faulty_Logger& operator<<(Faulty_Logger& out, const T& value) {
  if ( out.will_log() ) {
    if ( out.synchronized() ) {
      // In synchronized mode, we buffer data until a newline, std::flush, or the buffer
      // gets full. Then we dump the whole thing at once to the output stream, synchronizing
      // with a mutex.
      Faulty_Logger::per_thread_data *pdt = out.get_data();
      assert (!pdt->buffer.empty ());
      usleep (1000000);
      (*pdt->buffer.top()) << value;

      if ( ss_length(pdt->buffer.top()) >= out.thread_buffer_size_ || has_newline(value) )
          out.flush();
    }
    else {
        // In async mode we just dump data on the output stream as-is.
        // Multithreaded code will have ugly output.
        *(out.stream()) << value;
    }
  }

  return out;
}

Faulty_Logger& operator<< (Faulty_Logger& out, const NXLogStamp& value)
{
    out.current_level( value.level() );
    out.current_file( value.file() );

    // Writing an NXLogStamp to the stream indicates the start of a new entry.
    // If there's any content in the buffer, create a new entry in the output
    // queue.
    if ( out.synchronized() )
        out.new_stack_entry();

    out << out.stamp_to_string(value);

    return out;
}

#undef nxdbg
#undef nxinfo
#undef nxwarn
#undef nxerr
#undef nxfatal

#define nxdbg    faulty_logger << nxstamp(NXDEBUG)
#define nxinfo   faulty_logger << nxstamp(NXINFO)
#define nxwarn   faulty_logger << nxstamp(NXWARNING)
#define nxerr    faulty_logger << nxstamp(NXERROR)
#define nxfatal  faulty_logger << nxstamp(NXFATAL)

#define nxdbg_good    good_logger << nxstamp(NXDEBUG)
#define nxinfo_good   good_logger << nxstamp(NXINFO)
#define nxwarn_good   good_logger << nxstamp(NXWARNING)
#define nxerr_good    good_logger << nxstamp(NXERROR)
#define nxfatal_good  good_logger << nxstamp(NXFATAL)

/* Helper functions used by all component. */
void print_sigmask ();
void setup_faulty_logger ();
void setup_good_logger ();

/* Functions used by both main and auxiliary threads. */
void* log_task (void* /* unused */);

/* Functions used in main thread only. */
pthread_t spawn_thread ();
void install_signal_handler ();
void sig_handler (int signo);

/* Functions used by "killing" process. */
void killing_process_init (int argc, char **argv);
void killing_process_work (pid_t parent_pid);

#endif /* !defined (LOGGING_TEST_H) */
