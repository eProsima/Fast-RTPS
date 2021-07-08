// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef UTILS_SYSTEMINFO_HPP_
#define UTILS_SYSTEMINFO_HPP_

#if defined(_WIN32)
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif // if defined(_WIN32)

#include <cstdint>
#include <limits>
#include <random>

#include <fastrtps/types/TypesBase.h>
#include <utils/Host.hpp>

namespace eprosima {

using ReturnCode_t = fastrtps::types::ReturnCode_t;

/**
 * This singleton serves as a centralized point from where to obtain platform dependent system information.
 */
class SystemInfo
{
public:

    /**
     * Get the identifier of the current process.
     *
     * @return the identifier of the current process.
     */
    inline int process_id() const
    {
#if defined(__cplusplus_winrt)
        return (int)GetCurrentProcessId();
#elif defined(_WIN32)
        return (int)_getpid();
#else
        return (int)getpid();
#endif // platform selection
    }

    inline uint32_t unique_process_id() const
    {
        return unique_process_id_;
    }

    /**
     * Get the identifier of the current host.
     *
     * @return the identifier of the current host.
     */
    inline uint16_t host_id() const
    {
        return Host::instance().id();
    }

    inline uint16_t user_id() const
    {
        return user_id_;
    }

    /**
     * Get a reference to the singleton instance.
     *
     * @return reference to the singleton instance.
     */
    static const SystemInfo& instance()
    {
        static SystemInfo singleton;
        return singleton;
    }

    /**
     * Retrieve the value of a given environment variable if it exists, or nullptr.
     * The c-string which is returned in the env_value output parameter is only valid until the next time this function
     * is called, because it is a direct pointer to the static storage.
     * Modifying the string returned in env_value invokes undefined behavior.
     * If the environment variable is not set, a nullptr will be returned.
     *
     * This function is thread-safe as long as no other function modifies the host environment (in particular, POSIX
     * functions setenv, unsetenv and putenv would introduce a data race if called without synchronization.)
     *
     * \param [in] env_name the name of the environment variable
     * \param [out] env_value pointer to the value c-string
     * @return RETCODE_OK if the environment variable is set.
     * RETCODE_NO_DATA if the environment variable is unset.
     * RETCODE_BAD_PARAMETER if the provided parameters are not valid.
     */
    static ReturnCode_t get_env(
            const char* env_name,
            const char** env_value);

    /**
     * Get the effective username of the person that launched the application
     * This implementation follows the whoami implementation for POSIX
     * (https://github.com/coreutils/coreutils/blob/master/src/whoami.c)
     *
     * geteuid is always successful.
     *
     * getpwuid returns a pointer to a struct passwd if a matching entry is found and nullptr otherwise.
     * This pointer is only valid until the next time getpwuid function is called, because it is a direct
     * pointer to the static storage.
     * Modifying the pointer invokes undefined behavior.
     *
     * This function is thread-safe as long as no other function modifies the storage areas pointed within the structure
     * (in particular, POSIX functions getpwent or getpwnam might overwrite some of the storage areas.)
     *
     * \param [out] username string with the effective username.
     * @return RETCODE_OK if successful.
     * RETCODE_ERROR if the username information cannot be determined.
     */
    static ReturnCode_t get_username(
            std::string& username);

private:

    SystemInfo()
    {
        create_unique_process_id();
        get_user_id();
    }

    void get_user_id()
    {
#if defined(_WIN32)
        user_id_ = 0;
#else  // ^^^ defined(_WIN32) / !defined(_WIN32) vvv
        user_id_ = geteuid();
#endif  // !defined(_WIN32)
    }

    void create_unique_process_id()
    {
        // Generate a 4 bytes unique identifier that would be the same across all participants on the same process.
        // This will be used on the GuidPrefix of the participants, as well as on the SHM transport unicast locators.

        // Even though using the process id here might seem a nice idea, there are cases where it might not serve as
        // unique identifier of the process:
        // - One of them is when using a Kubernetes pod on which several containers with their own PID namespace are
        //   created.
        // - Another one is when a system in which a Fast DDS application is started during boot time. If the system
        //   crashes and is then re-started, it may happen that the participant may be considered an old one if the
        //   announcement lease duration did not expire.
        // In order to behave correctly in those situations, we will use the 16 least-significant bits of the PID,
        // along with a random 16 bits value. This should not be a problem, as the PID is known to be 16 bits long on
        // several systems. On those where it is longer, using the 16 least-significant ones along with a random value
        // should still give enough uniqueness for our use cases.
        int pid = process_id();

        std::random_device generator;
        std::uniform_int_distribution<uint16_t> distribution(0, (std::numeric_limits<uint16_t>::max)());
        uint16_t rand_value = distribution(generator);

        unique_process_id_ = (static_cast<uint32_t>(rand_value) << 16) | static_cast<uint32_t>(pid & 0xFFFF);
    }

    uint32_t unique_process_id_;
    uint16_t user_id_;

};

} // namespace eprosima

#endif // UTILS_SYSTEMINFO_HPP_
