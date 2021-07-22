#ifndef _RTPS_FLOWCONTROL_FLOWCONTROLLERFACTORY_HPP_
#define _RTPS_FLOWCONTROL_FLOWCONTROLLERFACTORY_HPP_

#include <fastdds/rtps/flowcontrol/FlowControllerDescriptor.hpp>
#include <fastdds/rtps/attributes/WriterAttributes.h>
#include "FlowController.hpp"

#include <string>
#include <map>

namespace eprosima {

namespace fastrtps {
namespace rtps {
class RTPSParticipantImpl;
} // namespace rtps
} // namespace fastrtps

namespace fastdds {
namespace rtps {

class FlowController;

/*!
 * Factory of flow controllers.
 *
 * @note Non-safe thread
 */
class FlowControllerFactory
{
public:

    /*!
     * Initialize the factory.
     * In charge of creating default flow controllers.
     * Call always before use it.
     *
     * @param participant Pointer to the participant owner of this object.
     */
    void init(
            fastrtps::rtps::RTPSParticipantImpl* participant);

    /*!
     * Registers a new flow controller.
     * This function should be used by the participant.
     *
     * @param flow_controller_descr FlowController descriptor.
     */
    void register_flow_controller (
            const FlowControllerDescriptor& flow_controller_descr);

    /*!
     * Get a FlowController given its name.
     *
     * @param flow_controller_name Name of the interested FlowController.
     * @return Pointer to the FlowController. nullptr if no registered FlowController with that name.
     */
    FlowController* retrieve_flow_controller(
            const std::string& flow_controller_name,
            const fastrtps::rtps::WriterAttributes& writer_attributes);

private:

    fastrtps::rtps::RTPSParticipantImpl* participant_ = nullptr;

    //! Stores the created flow controllers.
    std::map<std::string, std::unique_ptr<FlowController>> flow_controllers_;

};

} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _RTPS_FLOWCONTROL_FLOWCONTROLLERFACTORY_HPP_
