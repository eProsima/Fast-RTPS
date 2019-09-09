// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/**
 * @file eClock.h
 *
 */

#ifndef ECLOCK_H_
#define ECLOCK_H_

#include "../rtps/common/Time_t.h"

namespace eprosima {
namespace fastrtps {

/**
 * Class eClock used to obtain the time and to sleep some processes.
 * Time measured since 1970.
 * @ingroup UTILITIES_MODULE
 */
class RTPS_DllAPI eClock
{

public:

    eClock();

	virtual ~eClock();

	/**
	* Fill a Time_t with the current time
	* @param now Pointer to a RTPS Time_t instance to fill with the current time
	* @return true on success
	*/
	bool setTimeNow(rtps::Time_t* now);

	/**
	* Fill a Time_t with the current time
	* @param now Pointer to a Time_t instance to fill with the current time
	* @return true on success
	*/
	bool setTimeNow(fastrtps::Time_t* now);

	/**
	* Method to start measuring an interval in us.
	*/
	void intervalStart();

	/**
	* Method to finish measuring an interval in us.
	* @return Time of the interval in us
	*/
	uint64_t intervalEnd();

	/**
	* Put the current thread to sleep.
	* @param milliseconds Time to sleep
	*/
	static void my_sleep(uint32_t milliseconds);

private:
    
    uint64_t interval_start;
};

} /* namespace rtps */
} /* namespace eprosima */

#endif /* CLOCK_H_ */
