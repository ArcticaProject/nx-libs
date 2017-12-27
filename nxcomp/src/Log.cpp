/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
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


#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

#include "Log.h"
#include "config.h"

NXLog nx_log;


bool NXLog::will_log() const
{
    std::map<std::string, NXLogLevel>::const_iterator item = per_file_levels_.find(current_file());

    if ( item != per_file_levels_.end() )
    {
        return current_level() <= item->second;
    }
    else
    {
        return current_level() <= level();
    }
}


std::string NXLog::stamp_to_string(const NXLogStamp& stamp) const
{
    std::ostringstream oss;

    static const char* level_names[] = {
        "FATAL",
        "ERROR",
        "WARN ",
        "INFO ",
        "DEBUG"
    };

    if ( log_level() )
        oss << ((stamp.level() >=0 && stamp.level() < NXLOG_LEVEL_COUNT ) ? level_names[stamp.level()] : "???") << " ";

    if ( log_time() )
    {
        struct timeval timestamp = stamp.timestamp();
        struct tm timeinfo;

        localtime_r(&timestamp.tv_sec, &timeinfo);

        if ( log_unix_time() )
        {
            oss << timestamp.tv_sec;
        }
        else
        {
            #if HAVE_STD_PUT_TIME
            oss << " " << std::put_time(&timeinfo, "%Y/%m/%d %H:%M:%S");
            #else
            oss << timestamp.tv_sec;
            #endif
        }

        oss << "." << std::setw(3) << std::setfill('0') << (int)(timestamp.tv_usec / 1000) << " ";
    }

    if ( log_location() )
        oss << stamp.file() << "/" << stamp.function() << ":" << stamp.line()  << " ";

    if ( log_thread_id() )
    {
        if ( thread_name().empty() )
            oss << getpid() << "/" << pthread_self() << " ";
        else
            oss << "[" << thread_name() << "] ";
    }

    return oss.str();
}

NXLog& operator<< (NXLog& out, const NXLogStamp& value)
{
    /*
     * If appending, the file and function names must be empty and
     * the line set to zero.
     */
    const bool looks_like_append = ((value.file().empty()) || (value.function().empty()) || (0 == value.line()));
    const bool append = ((looks_like_append) && ((value.file().empty()) && (value.function().empty()) && (0 == value.line())));

    if ((looks_like_append) && (!append))
    {
        std::cerr << "WARNING: At least one element in logstamp invalid, but this is not supposed to be an append operation. "
                  << "Internal state error!\n" << "Log line will be discarded!" << std::endl;
    }
    else if (append)
    {
        /* Appending means that the log object's internal level and the message level must match. */
        if (out.current_level() == value.level())
        {
            /* Discard, if the message is not supposed to be written out anyway. */
            if (out.will_log ())
            {
                /* And the buffer must of course be non-empty. */
                if (out.has_buffer())
                {
                    out << " (cont.) ";
                }
                else
                {
                    std::cerr << "WARNING: Append operation requested, but no queued data available. "
                              << "Internal state error!\n" << "Log line will be discarded!" << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "WARNING: Append operation requested, but internal log level not matching line level. "
                      << "Internal state error!\n" << "Log line will be discarded!" << std::endl;
        }
    }
    else
    {
        out.current_level( value.level() );
        out.current_file( value.file() );

        // Writing an NXLogStamp to the stream indicates the start of a new entry.
        // If there's any content in the buffer and we actually intend to keep that line,
        // create a new entry in the output queue.
        if ( out.synchronized() && out.will_log() )
            out.new_stack_entry();

        out << out.stamp_to_string(value);
    }

    return out;
}
