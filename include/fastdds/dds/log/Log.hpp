// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef _FASTDDS_DDS_LOG_LOG_HPP_
#define _FASTDDS_DDS_LOG_LOG_HPP_

#include <fastrtps/utils/DBQueue.h>
#include <fastrtps/fastrtps_dll.h>
#include <thread>
#include <sstream>
#include <atomic>
#include <regex>

/**
 * eProsima log layer. Logging categories and verbosities can be specified dynamically at runtime. However, even on a category
 * not covered by the current verbosity level, there is some overhead on calling a log macro. For maximum performance, you can
 * opt out of logging any particular level by defining the following symbols:
 *
 * * define LOG_NO_ERROR
 * * define LOG_NO_WARNING
 * * define LOG_NO_INFO
 *
 * Additionally. the lowest level (Info) is disabled by default on release branches.
 */

// Logging API:

//! Logs an info message. Disable it through Log::SetVerbosity, define LOG_NO_INFO, or being in a release branch
#define logInfo(cat, msg) logInfo_(cat, msg)
//! Logs a warning. Disable reporting through Log::SetVerbosity or define LOG_NO_WARNING
#define logWarning(cat, msg) logWarning_(cat, msg)
//! Logs an error. Disable reporting through define LOG_NO_ERROR
#define logError(cat, msg) logError_(cat, msg)

namespace eprosima {
namespace fastdds {
namespace dds {

class LogConsumer;

/**
 * Logging utilities.
 * Logging is accessed through the three macros above, and configuration on the log output
 * can be achieved through static methods on the class. Logging at various levels can be
 * disabled dynamically (through the Verbosity level) or statically (through the LOG_NO_[VERB]
 * macros) for maximum performance.
 * @ingroup COMMON_MODULE
 */
class Log
{
public:

    /**
     * Types of log entry.
     * * Error: Maximum priority. Can only be disabled statically through LOG_NO_ERROR.
     * * Warning: Medium priority.  Can be disabled statically and dynamically.
     * * Info: Low priority. Useful for debugging. Disabled by default on release branches.
     */
    enum Kind
    {
        Error,
        Warning,
        Info,
    };

    /**
     * Registers an user defined consumer to route log output.
     * There is a default stdout consumer active as default.
     * @param consumer r-value to a consumer unique_ptr. It will be invalidated after the call.
     */
    RTPS_DllAPI static void RegisterConsumer(
            std::unique_ptr<LogConsumer>&& consumer);

    //! Removes all registered consumers, including the default stdout.
    RTPS_DllAPI static void ClearConsumers();

    //! Enables the reporting of filenames in log entries. Disabled by default.
    RTPS_DllAPI static void ReportFilenames(
            bool);

    //! Enables the reporting of function names in log entries. Enabled by default when supported.
    RTPS_DllAPI static void ReportFunctions(
            bool);

    //! Sets the verbosity level, allowing for messages equal or under that priority to be logged.
    RTPS_DllAPI static void SetVerbosity(
            Log::Kind);

    //! Returns the current verbosity level.
    RTPS_DllAPI static Log::Kind GetVerbosity();

    //! Sets a filter that will pattern-match against log categories, dropping any unmatched categories.
    RTPS_DllAPI static void SetCategoryFilter(
            const std::regex&);

    //! Sets a filter that will pattern-match against filenames, dropping any unmatched categories.
    RTPS_DllAPI static void SetFilenameFilter(
            const std::regex&);

    //! Sets a filter that will pattern-match against the provided error string, dropping any unmatched categories.
    RTPS_DllAPI static void SetErrorStringFilter(
            const std::regex&);

    //! Returns the logging engine to configuration defaults.
    RTPS_DllAPI static void Reset();

    //! Waits until no more log info is availabel
    RTPS_DllAPI static void Flush();

    //! Stops the logging thread. It will re-launch on the next call to a successful log macro.
    RTPS_DllAPI static void KillThread();

    // Note: In VS2013, if you're linking this class statically, you will have to call KillThread before leaving
    // main, due to an unsolved MSVC bug.

    struct Context
    {
        const char* filename;
        int line;
        const char* function;
        const char* category;
    };

    struct Entry
    {
        std::string message;
        Log::Context context;
        Log::Kind kind;
        std::string timestamp;
    };

    /**
     * Not recommended to call this method directly! Use the following macros:
     *  * logInfo(cat, msg);
     *  * logWarning(cat, msg);
     *  * logError(cat, msg);
     */
    RTPS_DllAPI static void QueueLog(
            const std::string& message,
            const Log::Context&,
            Log::Kind);

private:

    struct Resources
    {
        fastrtps::DBQueue<Entry> logs;
        std::vector<std::unique_ptr<LogConsumer>> consumers;
        std::unique_ptr<std::thread> logging_thread;

        // Condition variable segment.
        std::condition_variable cv;
        std::mutex cv_mutex;
        bool logging;
        bool work;
        int current_loop;

        // Context configuration.
        std::mutex config_mutex;
        bool filenames;
        bool functions;
        std::unique_ptr<std::regex> category_filter;
        std::unique_ptr<std::regex> filename_filter;
        std::unique_ptr<std::regex> error_string_filter;

        std::atomic<Log::Kind> verbosity;

        Resources();

        ~Resources();
    };

    static struct Resources resources_;

    // Applies transformations to the entries compliant with the options selected (such as
    // erasure of certain context information, or filtering by category. Returns false
    // if the log entry is blacklisted.
    static bool preprocess(
            Entry&);

    static void run();

    static void get_timestamp(
            std::string&);
};

/**
 * Consumes a log entry to output it somewhere.
 */
class LogConsumer
{
public:

    virtual ~LogConsumer() = default;

    virtual void Consume(
            const Log::Entry&) = 0;

protected:

    void print_timestamp(
            std::ostream& stream,
            const Log::Entry&,
            bool color) const;

    void print_header(
            std::ostream& stream,
            const Log::Entry&,
            bool color) const;

    void print_context(
            std::ostream& stream,
            const Log::Entry&,
            bool color) const;

    void print_message(
            std::ostream& stream,
            const Log::Entry&,
            bool color) const;

    void print_new_line(
            std::ostream& stream,
            bool color) const;
};

#if defined(WIN32)
#define __func__ __FUNCTION__
#endif // if defined(WIN32)

// Name of variables inside macros must be unique, or it could produce an error with external variables
#ifndef LOG_NO_ERROR
#define logError_(cat, msg)                                                                                            \
    do{                                                                                                                \
        using namespace eprosima::fastdds::dds;                                                                        \
        std::stringstream fastdds_log_ss_tmp__;                                                                        \
        fastdds_log_ss_tmp__ << msg;                                                                                   \
        Log::QueueLog(fastdds_log_ss_tmp__.str(), Log::Context{__FILE__, __LINE__, __func__, #cat}, Log::Kind::Error); \
    }while (0)
#elif (defined(__INTERNALDEBUG) || defined(_INTERNALDEBUG))
#define logError_(cat, msg)                                     \
    do{                                                         \
        auto fastdds_log_lambda_tmp__ = [&]()                   \
                {                                               \
                    std::stringstream fastdds_log_ss_tmp__;     \
                    fastdds_log_ss_tmp__ << msg;                \
                };                                              \
        (void)fastdds_log_lambda_tmp__;                         \
    }while (0)
#else
#define logError_(cat, msg)
#endif // ifndef LOG_NO_ERROR

#ifndef LOG_NO_WARNING
#define logWarning_(cat, msg)                                                                                       \
    do{                                                                                                             \
        using namespace eprosima::fastdds::dds;                                                                     \
        if (Log::GetVerbosity() >= Log::Kind::Warning)                                                              \
        {                                                                                                           \
            std::stringstream fastdds_log_ss_tmp__;                                                                 \
            fastdds_log_ss_tmp__ << msg;                                                                            \
            Log::QueueLog(                                                                                          \
                fastdds_log_ss_tmp__.str(), Log::Context{__FILE__, __LINE__, __func__, #cat}, Log::Kind::Warning);  \
        }                                                                                                           \
    }while (0)
#elif (defined(__INTERNALDEBUG) || defined(_INTERNALDEBUG))
#define logWarning_(cat, msg)                                   \
    do{                                                         \
        auto fastdds_log_lambda_tmp__ = [&]()                   \
                {                                               \
                    std::stringstream fastdds_log_ss_tmp__;     \
                    fastdds_log_ss_tmp__ << msg;                \
                };                                              \
        (void)fastdds_log_lambda_tmp__;                         \
    }while (0)
#else
#define logWarning_(cat, msg)
#endif // ifndef LOG_NO_WARNING

#if (defined(__INTERNALDEBUG) || defined(_INTERNALDEBUG)) && (defined(_DEBUG) || defined(__DEBUG)) &&   \
    (!defined(LOG_NO_INFO))
#define logInfo_(cat, msg)                                                                              \
    do{                                                                                                 \
        using namespace eprosima::fastdds::dds;                                                         \
        if (Log::GetVerbosity() >= Log::Kind::Info)                                                     \
        {                                                                                               \
            std::stringstream fastdds_log_ss_tmp__;                                                     \
            fastdds_log_ss_tmp__ << msg;                                                                \
            Log::QueueLog(fastdds_log_ss_tmp__.str(), Log::Context{__FILE__, __LINE__, __func__, #cat}, \
                    Log::Kind::Info);                                                                   \
        }                                                                                               \
    }while (0)
#elif (defined(__INTERNALDEBUG) || defined(_INTERNALDEBUG))
#define logInfo_(cat, msg)                                  \
    do{                                                     \
        auto fastdds_log_lambda_tmp__ = [&]()               \
                {                                           \
                    std::stringstream fastdds_log_ss_tmp__; \
                    fastdds_log_ss_tmp__ << msg;            \
                };                                          \
        (void)fastdds_log_lambda_tmp__;                     \
    }while (0)
#else
#define logInfo_(cat, msg)
#endif // if (defined(__INTERNALDEBUG) || defined(_INTERNALDEBUG)) && (defined(_DEBUG) || defined(__DEBUG)) && (!defined(LOG_NO_INFO))

} // namespace dds
} // namespace fastdds
} // namespace eprosima

#endif // ifndef _FASTDDS_DDS_LOG_LOG_HPP_
