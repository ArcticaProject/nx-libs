/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/


#ifndef NXLog_H
#define NXLog_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/time.h>

#include <map>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <stack>

/** Log severity level */
enum NXLogLevel
{
    NXFATAL,
    NXERROR,
    NXWARNING,
    NXINFO,
    NXDEBUG,
    NXLOG_LEVEL_COUNT
};


/**
 * Log timestamp class
 *
 * Stores the timestamp, file, function, line number and log level.
 * Acts as a manipulator on the NXLog class, telling it a new log
 * severity level. For instance:
 *
 *     nx_log << NXLogStamp(...,NXINFO)
 *
 * Tells nx_log that now NXINFO type messages are being logged. This
 * will be applied until a new NXLogStamp with a different level
 * is sent to the NXLog.
 */
class NXLogStamp
{
    private:
    std::string file_;
    std::string function_;
    size_t line_;
    NXLogLevel level_;
    struct timeval timestamp_;

    public:
    /** File where the event occurred */
    std::string file() const
    {
        return file_;
    }

    /** Function where the event occurred */
    std::string function() const
    {
        return function_;
    }

    /** Line where the event occurred */
    size_t line() const
    {
        return line_;
    }

    /** Severity level of the event */
    NXLogLevel level() const
    {
        return level_;
    }

    /** Time of the event */
    struct timeval timestamp() const
    {
        return timestamp_;
    }


    NXLogStamp(NXLogLevel _level, const char *_file = "", const char *_function = "", size_t _line = 0) : file_(_file), function_(_function), line_(_line), level_(_level)
    {
        gettimeofday(&timestamp_, NULL);
    }

};


/**
 * Log class
 *
 * Logs events to a stream, filters by file/level
 */
class NXLog
{
#ifdef INTERNAL_LOGGING_TEST
    protected:
#endif
    NXLogLevel level_;

    std::ostream *stream_;
    std::map< std::string, NXLogLevel > per_file_levels_;
    bool synchronized_;
    size_t  thread_buffer_size_;
    pthread_mutex_t output_lock_;
    pthread_key_t tls_key_;

    bool log_level_;
    bool log_time_;
    bool log_unix_time_;
    bool log_location_;
    bool log_thread_id_;

    typedef struct per_thread_data_s
    {
        NXLogLevel                      current_level;
        std::string*                    current_file;
        std::string*                    thread_name;
        std::stack<std::stringstream*>  buffer;
        NXLog*                          log_obj;
    } per_thread_data;


    static void free_thread_data(void* arg)
    {
        per_thread_data *pdt = (per_thread_data*)arg;

        if ( !pdt )
            return;

        if ( pdt->log_obj ) {
            // Ensure the buffer is flushed before thread deletion
            pdt->log_obj->flush(pdt);
        }

        delete pdt->current_file;
        delete pdt->thread_name;

        while (!pdt->buffer.empty()) {
            /*
             * get the stringstream object created in new_stack_entry()
             * from the stack and delete it after pop()
             */
            std::stringstream* tmp = pdt->buffer.top();
            (void) pdt->buffer.pop ();
            delete tmp;
        }

        delete pdt;
    }

    per_thread_data* get_data_int() const
    {
        per_thread_data *ret = NULL;

        if ( (ret = (per_thread_data*)pthread_getspecific(tls_key_)) == NULL )
        {
            ret = new per_thread_data;
            ret->current_level = NXDEBUG;
            ret->current_file = new std::string();
            ret->thread_name = new std::string();
            ret->log_obj = const_cast<NXLog*>(this);
            pthread_setspecific(tls_key_, ret);
        }

        return ret;
    }

    per_thread_data* get_data()
    {
        return get_data_int();
    }

    const per_thread_data* get_data() const
    {
        return get_data_int();
    }

    /** Convert NXLogStamp to string according to the current configuration */
    std::string stamp_to_string(const NXLogStamp& stamp) const;

    void new_stack_entry()
    {
        per_thread_data *pdt = get_data();
        pdt->buffer.push(new std::stringstream());
    }

    /**
     * Internal flush function
     *
     * When a thread is being terminated and free_thread_data gets called,
     * the TLS key gets set to NULL before the call to free_thread_data,
     * and the destructor function gets the old value.
     *
     * This means that get_data() stops working correctly, and we need
     * to be able to pass the old pointer.
     */
    virtual /* Note: this function needs to be virtual for the logging test application. Don't remove. */
    void flush(per_thread_data *pdt)
    {
         /*
          * Block all signals until we are dong printing data.
          * Ensures that a signal handler won't interrupt us
          * and overwrite the buffer data mid-print, leading
          * to confusing output.
          */
        sigset_t orig_signal_mask,
                 tmp_signal_mask;
        sigemptyset(&orig_signal_mask);

        /* Set up new mask to block all signals. */
        sigfillset(&tmp_signal_mask);

        /* Block all signals. */
        pthread_sigmask(SIG_BLOCK, &tmp_signal_mask, &orig_signal_mask);

        if (!pdt->buffer.empty ()) {
            /*
             * get the stringstream object created in new_stack_entry()
             * from the stack and delete it after pop()
             */
            std::stringstream *tmp = pdt->buffer.top();
            const std::string str = tmp->str();

            if (!str.empty())
            {
                pthread_mutex_lock(&output_lock_);
                (*stream()) << str;
                pthread_mutex_unlock(&output_lock_);
            }

            /* Remove from stack. */
            pdt->buffer.pop();

            /* free memory */
            delete tmp;
        }

        /* Restore old signal mask. */
        pthread_sigmask(SIG_SETMASK, &orig_signal_mask, NULL);
    }


    public:
    NXLog() : level_(NXWARNING), stream_(&std::cerr), synchronized_(true), thread_buffer_size_(1024),
              log_level_(false), log_time_(false), log_unix_time_(false), log_location_(false), log_thread_id_(false)
    {
        if ( pthread_key_create(&tls_key_, free_thread_data) != 0 )
        {
            std::cerr << "pthread_key_create failed" << std::endl;
            abort();
        }

    }

    ~NXLog()
    {
        per_thread_data *pdt = get_data();

        // Flush any remaining output and delete TLS data
        free_thread_data(pdt);

        pthread_key_delete(tls_key_);

        if ((stream_) && (stream_ != &std::cerr)) {
            delete stream_;
        }
    }

    /** Minimum severity level to output */
    NXLogLevel level() const
    {
        return level_;
    }

    void level(NXLogLevel _level)
    {
        level_ = _level;
    }


    /** Current severity level */
    NXLogLevel current_level() const
    {
        return get_data()->current_level;
    }

    void current_level(NXLogLevel _level)
    {
        get_data()->current_level = _level;
    }

    /** Source file from which messages are currently originating */
    std::string current_file() const
    {
        return *get_data()->current_file;
    }

    void current_file(std::string val)
    {
        *get_data()->current_file = val;
    }

    std::ostream* stream() const
    {
        return stream_;
    }

    void stream(std::ostream *_stream)
    {
        flush();
        stream_ = _stream;
    }

    bool synchronized() const {
        return synchronized_;
    }

    void synchronized(bool val) {
        synchronized_ = val;
    }

    bool log_level() const
    {
        return log_level_;
    }

    void log_level(bool val)
    {
        log_level_ = val;
    }

    bool log_time() const
    {
        return log_time_;
    }

    void log_time(bool val)
    {
        log_time_ = val;
    }

    bool log_unix_time() const
    {
        return log_unix_time_;
    }

    void log_unix_time(bool val)
    {
        log_unix_time_ = val;
    }

    bool log_location() const
    {
        return log_location_;
    }

    void log_location(bool val)
    {
        log_location_ = val;
    }

    bool log_thread_id() const
    {
        return log_thread_id_;
    }

    void log_thread_id(bool val)
    {
        log_thread_id_ = val;
    }


    void flush()
    {
        per_thread_data *pdt = get_data();
        flush(pdt);
    }

    std::string thread_name() const
    {
        return *get_data()->thread_name;
    }

    void thread_name(std::string str)
    {
        *get_data()->thread_name = str;
    }

    void thread_name(const char *str)
    {
        *get_data()->thread_name = str;
    }

    /**
     * True if a message sent to the NXLog object will be sent to the output
     *
     * This considers two things:
     *
     * If there's a per-file log level, then it is used
     * Otherwise the global log level is used.
     *
     * If the log level permits the current message to be output, then the
     * return value is true.
     */
    bool will_log() const;

    bool has_buffer() const
    {
        return (!(get_data()->buffer.empty ()));
    }


    /**
     * This catches std::flush
     */
    NXLog&  operator<<(std::ostream& (*F)(std::ostream&))
    {
        if ( will_log() )
        {
            if ( synchronized() )
            {
                /* Verbosely discard data if we don't have a buffer. */
                if (!(has_buffer()))
                {
                    std::cerr << "WARNING: no buffer available! "
                              << "Internal state error!\n" << "Log hunk will be discarded!" << std::endl;
                }
                else
                {
                    per_thread_data *pdt = get_data();
                    assert (!pdt->buffer.empty ());
                    (*pdt->buffer.top()) << F;
                    flush();
                }
            }
            else
            {
                *(stream()) << F;
            }
        }

        return *this;
    }

    template<typename T>
    friend NXLog& operator<<(NXLog& out, const T& value);

    friend NXLog& operator<< (NXLog& out, const NXLogStamp& value);
};


extern NXLog nx_log;


#define nxstamp(l)        NXLogStamp(l, __FILE__, __func__, __LINE__)
#define nxstamp_append(l) NXLogStamp(l)


#define nxdbg    nx_log << nxstamp(NXDEBUG)
#define nxinfo   nx_log << nxstamp(NXINFO)
#define nxwarn   nx_log << nxstamp(NXWARNING)
#define nxerr    nx_log << nxstamp(NXERROR)
#define nxfatal  nx_log << nxstamp(NXFATAL)

/* Append data to already existing (i.e., same-level) line. */
#define nxdbg_append    nx_log << nxstamp_append(NXDEBUG)
#define nxinfo_append   nx_log << nxstamp_append(NXINFO)
#define nxwarn_append   nx_log << nxstamp_append(NXWARNING)
#define nxerr_append    nx_log << nxstamp_append(NXERROR)
#define nxfatal_append  nx_log << nxstamp_append(NXFATAL)

NXLog& operator<< (NXLog& out, const NXLogStamp& value);


template <typename T>
bool has_newline(T &value)
{
    return false;
}

template <char*>
static bool has_newline(char *value)
{
    if (value)
    {
        return strstr(value, "\n") != NULL;
    }
    else
    {
        return false;
    }
}

template <char>
static bool has_newline(char value)
{
    return value == '\n';
}

template <std::string&>
static bool has_newline(std::string &value)
{
    return value.find_first_of("\n") != std::string::npos;
}

static size_t ss_length(std::stringstream *ss)
{
    size_t pos = ss->tellg();
    size_t ret = 0;
    ss->seekg(0, std::ios::end);
    ret = ss->tellg();
    ss->seekg(pos, std::ios::beg);
    return ret;
}

template <typename T>
NXLog& operator<<(NXLog& out, const T& value)
{
    if ( out.will_log() )
    {
        if ( out.synchronized() )
        {
            /* Verbosely discard data if we don't have a buffer. */
            if (!(out.has_buffer()))
            {
                std::cerr << "WARNING: no buffer available! "
                          << "Internal state error!\n" << "Log hunk will be discarded!" << std::endl;
            }
            else
            {
                // In synchronized mode, we buffer data until a newline, std::flush, or the buffer
                // gets full. Then we dump the whole thing at once to the output stream, synchronizing
                // with a mutex.
                NXLog::per_thread_data *pdt = out.get_data();
                assert (!pdt->buffer.empty ());
                (*pdt->buffer.top()) << value;

                if ( ss_length(pdt->buffer.top()) >= out.thread_buffer_size_ || has_newline(value) )
                    out.flush();
            }
        }
        else
        {
            // In async mode we just dump data on the output stream as-is.
            // Multithreaded code will have ugly output.
            *(out.stream()) << value;
        }

    }

    return out;
}

#endif
