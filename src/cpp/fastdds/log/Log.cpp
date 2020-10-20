#include <chrono>
#include <iomanip>
#include <mutex>

#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/log/StdoutConsumer.hpp>
#include <fastdds/dds/log/Colors.hpp>
#include <iostream>

using namespace std;
namespace eprosima {
namespace fastdds {
namespace dds {

struct Log::Resources Log::resources_;

Log::Resources::Resources()
    : logging(false)
    , work(false)
    , current_loop(0)
    , filenames(false)
    , functions(true)
    , verbosity(static_cast<Log::Kind>(Log::User | Log::Error | Log::Critical | Log::Fatal))
{
    resources_.consumers.emplace_back(new StdoutConsumer);
}

Log::Resources::~Resources()
{
    Log::KillThread();
}

void Log::RegisterConsumer(
        std::unique_ptr<LogConsumer>&& consumer)
{
    std::unique_lock<std::mutex> guard(resources_.config_mutex);
    resources_.consumers.emplace_back(std::move(consumer));
}

void Log::ClearConsumers()
{
    std::unique_lock<std::mutex> working(resources_.cv_mutex);
    resources_.cv.wait(working,
            [&]()
            {
                return resources_.logs.BothEmpty();
            });
    std::unique_lock<std::mutex> guard(resources_.config_mutex);
    resources_.consumers.clear();
}

void Log::Reset()
{
    {
        std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
        resources_.category_filter.reset();
        resources_.filename_filter.reset();
        resources_.error_string_filter.reset();
        resources_.filenames = false;
        resources_.functions = true;
        resources_.consumers.clear();
        resources_.consumers.emplace_back(new StdoutConsumer);
    }
    SetVerbosity(Log::User);
}

void Log::Flush()
{
    std::unique_lock<std::mutex> guard(resources_.cv_mutex);

    if (!resources_.logging && !resources_.logging_thread)
    {
        // already killed
        return;
    }

    /*   Flush() two steps strategy:

         I must assure Log::Run swaps the queues because only swapping the queues the background content
         will be consumed (first Run() loop).

         Then, I must assure the new front queue content is consumed (second Run() loop).
     */

    int last_loop = -1;

    for (int i = 0; i < 2; ++i)
    {
        resources_.cv.wait(guard,
                [&]()
                {
                    /* I must avoid:
                     + the two calls be processed without an intermediate Run() loop (by using last_loop sequence number)
                     + deadlock by absence of Run() loop activity (by using BothEmpty() call)
                     */
                    return !resources_.logging ||
                    ( resources_.logs.Empty() &&
                    ( last_loop != resources_.current_loop || resources_.logs.BothEmpty()) );
                });

        last_loop = resources_.current_loop;

    }
}

void Log::run()
{
    std::unique_lock<std::mutex> guard(resources_.cv_mutex);

    while (resources_.logging)
    {
        resources_.cv.wait(guard,
                [&]()
                {
                    return !resources_.logging || resources_.work;
                });

        resources_.work = false;

        guard.unlock();
        {
            resources_.logs.Swap();
            while (!resources_.logs.Empty())
            {
                std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
                if (preprocess(resources_.logs.Front()))
                {
                    for (auto& consumer : resources_.consumers)
                    {
                        consumer->Consume(resources_.logs.Front());
                    }
                }

                resources_.logs.Pop();
            }
        }
        guard.lock();

        // avoid overflow
        if (++resources_.current_loop > 10000)
        {
            resources_.current_loop = 0;
        }

        resources_.cv.notify_all();
    }
}

void Log::ReportFilenames(
        bool report)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.filenames = report;
}

void Log::ReportFunctions(
        bool report)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.functions = report;
}

bool Log::preprocess(
        Log::Entry& entry)
{
    if (resources_.category_filter && !regex_search(entry.context.category, *resources_.category_filter))
    {
        return false;
    }
    if (resources_.filename_filter && !regex_search(entry.context.filename, *resources_.filename_filter))
    {
        return false;
    }
    if (resources_.error_string_filter && !regex_search(entry.message, *resources_.error_string_filter))
    {
        return false;
    }
    if (!resources_.filenames)
    {
        entry.context.filename = nullptr;
    }
    if (!resources_.functions)
    {
        entry.context.function = nullptr;
    }

    return true;
}

void Log::KillThread()
{
    {
        std::unique_lock<std::mutex> guard(resources_.cv_mutex);
        resources_.logging = false;
        resources_.work = false;
    }

    if (resources_.logging_thread)
    {
        resources_.cv.notify_all();
        // The #ifdef workaround here is due to an unsolved MSVC bug, which Microsoft has announced
        // they have no intention of solving: https://connect.microsoft.com/VisualStudio/feedback/details/747145
        // Each VS version deals with post-main deallocation of threads in a very different way.
#if !defined(_WIN32) || defined(FASTRTPS_STATIC_LINK) || _MSC_VER >= 1800
        resources_.logging_thread->join();
#endif // if !defined(_WIN32) || defined(FASTRTPS_STATIC_LINK) || _MSC_VER >= 1800
        resources_.logging_thread.reset();
    }
}

void Log::QueueLog(
        const std::string& message,
        const Log::Context& context,
        Log::Kind kind)
{
    {
        std::unique_lock<std::mutex> guard(resources_.cv_mutex);
        if (!resources_.logging && !resources_.logging_thread)
        {
            resources_.logging = true;
            resources_.logging_thread.reset(new thread(Log::run));
        }
    }

    std::string timestamp;
    get_timestamp(timestamp);
    resources_.logs.Push(Log::Entry{message, context, kind, timestamp});
    {
        std::unique_lock<std::mutex> guard(resources_.cv_mutex);
        resources_.work = true;
    }
    resources_.cv.notify_all();
}

Log::Kind Log::GetVerbosity()
{
    return resources_.verbosity;
}

void Log::SetVerbosity(
        Log::Kind kind)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.verbosity = Log::Quiet;
    while (kind > Log::Quiet)
    {
        resources_.verbosity = static_cast<Log::Kind>(resources_.verbosity.load() | kind);
        kind = static_cast<Log::Kind>(kind >> 1);
    }
}

void Log::SetVerbosityMask(
        Log::Kind kind)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.verbosity = kind;
}

void Log::SetCategoryFilter(
        const std::regex& filter)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.category_filter.reset(new std::regex(filter));
}

void Log::SetFilenameFilter(
        const std::regex& filter)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.filename_filter.reset(new std::regex(filter));
}

void Log::SetErrorStringFilter(
        const std::regex& filter)
{
    std::unique_lock<std::mutex> configGuard(resources_.config_mutex);
    resources_.error_string_filter.reset(new std::regex(filter));
}

void Log::get_timestamp(
        std::string& timestamp)
{
    std::stringstream stream;
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    tp -= std::chrono::duration_cast<std::chrono::seconds>(tp);
    auto ms = static_cast<unsigned>(tp / std::chrono::milliseconds(1));

#if defined(_WIN32)
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_c);
    stream << std::put_time(&timeinfo, "%F %T") << "." << std::setw(3) << std::setfill('0') << ms << " ";
    //#elif defined(__clang__) && !defined(std::put_time) // TODO arm64 doesn't seem to support std::put_time
    //    (void)now_c;
    //    (void)ms;
#else
    stream << std::put_time(localtime(&now_c), "%F %T") << "." << std::setw(3) << std::setfill('0') << ms << " ";
#endif // if defined(_WIN32)
    timestamp = stream.str();
}

void LogConsumer::print_timestamp(
        std::ostream& stream,
        const Log::Entry& entry,
        bool color) const
{
    std::string white = (color) ? C_B_WHITE : "";
    stream << white << entry.timestamp;
}

void LogConsumer::print_header(
        std::ostream& stream,
        const Log::Entry& entry,
        bool color) const
{
    std::string c_b_color = (!color) ? "" :
            (entry.kind == Log::Kind::Error) ? C_B_RED :
            (entry.kind == Log::Kind::Warning) ? C_B_YELLOW :
            (entry.kind == Log::Kind::Info) ? C_B_GREEN : "";

    std::string white = (color) ? C_B_WHITE : "";

    std::string kind = (entry.kind == Log::Kind::Error) ? "Error" :
            (entry.kind == Log::Kind::Warning) ? "Warning" :
            (entry.kind == Log::Kind::Info) ? "Info" : "";

    stream << c_b_color << "[" << white << entry.context.category << c_b_color << " " << kind << "] ";
}

void LogConsumer::print_context(
        std::ostream& stream,
        const Log::Entry& entry,
        bool color) const
{
    if (color)
    {
        stream << C_B_BLUE;
    }
    if (entry.context.filename)
    {
        stream << " (" << entry.context.filename;
        stream << ":" << entry.context.line << ")";
    }
    if (entry.context.function)
    {
        stream << " -> Function ";
        if (color)
        {
            stream << C_CYAN;
        }
        stream << entry.context.function;
    }
}

void LogConsumer::print_message(
        std::ostream& stream,
        const Log::Entry& entry,
        bool color) const
{
    std::string white = (color) ? C_WHITE : "";
    stream << white << entry.message;
}

void LogConsumer::print_new_line(
        std::ostream& stream,
        bool color) const
{
    std::string def = (color) ? C_DEF : "";
    stream << def << std::endl;
}

} //namespace dds
} //namespace fastdds
} //namespace eprosima
