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

/*! 
 * @file tcp_idl.1.cpp
 * This source file contains the definition of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace { char dummy; }
#endif

#include "TCPControlMessage.h"

#include <fastcdr/Cdr.h>

#include <fastcdr/exceptions/BadParamException.h>
using namespace eprosima::fastcdr::exception;

#include <utility>


ConnectionRequest_t::ConnectionRequest_t() : m_vendorId(c_VendorId_eProsima);
{
}

ConnectionRequest_t::~ConnectionRequest_t()
{
}

ConnectionRequest_t::ConnectionRequest_t(const ConnectionRequest_t &x)
{
    m_protocolVersion = x.m_protocolVersion;
    m_vendorId = x.m_vendorId;
    m_transportLocator = x.m_transportLocator;
}

ConnectionRequest_t::ConnectionRequest_t(ConnectionRequest_t &&x)
{
    m_protocolVersion = x.m_protocolVersion;
    m_vendorId = x.m_vendorId;
    m_transportLocator = x.m_transportLocator;
}

ConnectionRequest_t& ConnectionRequest_t::operator=(const ConnectionRequest_t &x)
{
    m_protocolVersion = x.m_protocolVersion;
    m_vendorId = x.m_vendorId;
    m_transportLocator = x.m_transportLocator;
    
    return *this;
}

ConnectionRequest_t& ConnectionRequest_t::operator=(ConnectionRequest_t &&x)
{
    m_protocolVersion = x.m_protocolVersion;
    m_vendorId = x.m_vendorId;
    m_transportLocator = x.m_transportLocator;
    
    return *this;
}

size_t ConnectionRequest_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t ConnectionRequest_t::getCdrSerializedSize(const ConnectionRequest_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void ConnectionRequest_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_protocolVersion;
    scdr << m_vendorId;
    scdr << m_transportLocator;
}

void ConnectionRequest_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_protocolVersion;
    dcdr >> m_vendorId;
    dcdr >> m_transportLocator;
}

size_t ConnectionRequest_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
    return current_align;
}

bool ConnectionRequest_t::isKeyDefined()
{
    return false;
}

void ConnectionRequest_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
	 
	 
}
OpenLogicalPortRequest_t::OpenLogicalPortRequest_t()
{
    m_logicalPort = 0;
}

OpenLogicalPortRequest_t::~OpenLogicalPortRequest_t()
{
}

OpenLogicalPortRequest_t::OpenLogicalPortRequest_t(const OpenLogicalPortRequest_t &x)
{
    m_logicalPort = x.m_logicalPort;
}

OpenLogicalPortRequest_t::OpenLogicalPortRequest_t(OpenLogicalPortRequest_t &&x)
{
    m_logicalPort = x.m_logicalPort;
}

OpenLogicalPortRequest_t& OpenLogicalPortRequest_t::operator=(const OpenLogicalPortRequest_t &x)
{
    m_logicalPort = x.m_logicalPort;
    
    return *this;
}

OpenLogicalPortRequest_t& OpenLogicalPortRequest_t::operator=(OpenLogicalPortRequest_t &&x)
{
    m_logicalPort = x.m_logicalPort;
    
    return *this;
}

size_t OpenLogicalPortRequest_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t OpenLogicalPortRequest_t::getCdrSerializedSize(const OpenLogicalPortRequest_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void OpenLogicalPortRequest_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_logicalPort;
}

void OpenLogicalPortRequest_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_logicalPort;
}

size_t OpenLogicalPortRequest_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool OpenLogicalPortRequest_t::isKeyDefined()
{
    return false;
}

void OpenLogicalPortRequest_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}
CheckLogicalPortsRequest_t::CheckLogicalPortsRequest_t()
{
}

CheckLogicalPortsRequest_t::~CheckLogicalPortsRequest_t()
{
}

CheckLogicalPortsRequest_t::CheckLogicalPortsRequest_t(const CheckLogicalPortsRequest_t &x)
{
    m_logicalPortsRange = x.m_logicalPortsRange;
}

CheckLogicalPortsRequest_t::CheckLogicalPortsRequest_t(CheckLogicalPortsRequest_t &&x)
{
    m_logicalPortsRange = std::move(x.m_logicalPortsRange);
}

CheckLogicalPortsRequest_t& CheckLogicalPortsRequest_t::operator=(const CheckLogicalPortsRequest_t &x)
{
    m_logicalPortsRange = x.m_logicalPortsRange;
    
    return *this;
}

CheckLogicalPortsRequest_t& CheckLogicalPortsRequest_t::operator=(CheckLogicalPortsRequest_t &&x)
{
    m_logicalPortsRange = std::move(x.m_logicalPortsRange);
    
    return *this;
}

size_t CheckLogicalPortsRequest_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += (100 * 2) + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t CheckLogicalPortsRequest_t::getCdrSerializedSize(const CheckLogicalPortsRequest_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += (data.logicalPortsRange().size() * 2) + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void CheckLogicalPortsRequest_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_logicalPortsRange;
}

void CheckLogicalPortsRequest_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_logicalPortsRange;
}

size_t CheckLogicalPortsRequest_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool CheckLogicalPortsRequest_t::isKeyDefined()
{
    return false;
}

void CheckLogicalPortsRequest_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}
KeepAliveRequest_t::KeepAliveRequest_t()
{
}

KeepAliveRequest_t::~KeepAliveRequest_t()
{
}

KeepAliveRequest_t::KeepAliveRequest_t(const KeepAliveRequest_t &x)
{
    m_locator = x.m_locator;
}

KeepAliveRequest_t::KeepAliveRequest_t(KeepAliveRequest_t &&x)
{
    m_locator = x.m_locator;
}

KeepAliveRequest_t& KeepAliveRequest_t::operator=(const KeepAliveRequest_t &x)
{
    m_locator = x.m_locator;
    
    return *this;
}

KeepAliveRequest_t& KeepAliveRequest_t::operator=(KeepAliveRequest_t &&x)
{
    m_locator = x.m_locator;
    
    return *this;
}

size_t KeepAliveRequest_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t KeepAliveRequest_t::getCdrSerializedSize(const KeepAliveRequest_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void KeepAliveRequest_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_locator;
}

void KeepAliveRequest_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_locator;
}

size_t KeepAliveRequest_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool KeepAliveRequest_t::isKeyDefined()
{
    return false;
}

void KeepAliveRequest_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}
LogicalPortIsClosedRequest_t::LogicalPortIsClosedRequest_t()
{
    m_logicalPort = 0;
}

LogicalPortIsClosedRequest_t::~LogicalPortIsClosedRequest_t()
{
}

LogicalPortIsClosedRequest_t::LogicalPortIsClosedRequest_t(const LogicalPortIsClosedRequest_t &x)
{
    m_logicalPort = x.m_logicalPort;
}

LogicalPortIsClosedRequest_t::LogicalPortIsClosedRequest_t(LogicalPortIsClosedRequest_t &&x)
{
    m_logicalPort = x.m_logicalPort;
}

LogicalPortIsClosedRequest_t& LogicalPortIsClosedRequest_t::operator=(const LogicalPortIsClosedRequest_t &x)
{
    m_logicalPort = x.m_logicalPort;
    
    return *this;
}

LogicalPortIsClosedRequest_t& LogicalPortIsClosedRequest_t::operator=(LogicalPortIsClosedRequest_t &&x)
{
    m_logicalPort = x.m_logicalPort;
    
    return *this;
}

size_t LogicalPortIsClosedRequest_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t LogicalPortIsClosedRequest_t::getCdrSerializedSize(const LogicalPortIsClosedRequest_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void LogicalPortIsClosedRequest_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_logicalPort;
}

void LogicalPortIsClosedRequest_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_logicalPort;
}

size_t LogicalPortIsClosedRequest_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool LogicalPortIsClosedRequest_t::isKeyDefined()
{
    return false;
}

void LogicalPortIsClosedRequest_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}
RequestData::RequestData()
{
    m__d = ::BIND_CONNECTION;





}

RequestData::~RequestData()
{
}

RequestData::RequestData(const RequestData &x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_connectionRequest = x.m_connectionRequest;
        break;
        case ::OPEN_LOGICAL_PORT:
        m_openLogicalPortRequest = x.m_openLogicalPortRequest;
        break;
        case ::CHECK_LOGICAL_PORT:
        m_checkLogicalPortsRequest = x.m_checkLogicalPortsRequest;
        break;
        case ::KEEP_ALIVE:
        m_keepAliveRequest = x.m_keepAliveRequest;
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        m_logicalPortIsClosedRequest = x.m_logicalPortIsClosedRequest;
        break;
        default:
        break;
    }
}

RequestData::RequestData(RequestData &&x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_connectionRequest = std::move(x.m_connectionRequest);
        break;
        case ::OPEN_LOGICAL_PORT:
        m_openLogicalPortRequest = std::move(x.m_openLogicalPortRequest);
        break;
        case ::CHECK_LOGICAL_PORT:
        m_checkLogicalPortsRequest = std::move(x.m_checkLogicalPortsRequest);
        break;
        case ::KEEP_ALIVE:
        m_keepAliveRequest = std::move(x.m_keepAliveRequest);
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        m_logicalPortIsClosedRequest = std::move(x.m_logicalPortIsClosedRequest);
        break;
        default:
        break;
    }
}

RequestData& RequestData::operator=(const RequestData &x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_connectionRequest = x.m_connectionRequest;
        break;
        case ::OPEN_LOGICAL_PORT:
        m_openLogicalPortRequest = x.m_openLogicalPortRequest;
        break;
        case ::CHECK_LOGICAL_PORT:
        m_checkLogicalPortsRequest = x.m_checkLogicalPortsRequest;
        break;
        case ::KEEP_ALIVE:
        m_keepAliveRequest = x.m_keepAliveRequest;
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        m_logicalPortIsClosedRequest = x.m_logicalPortIsClosedRequest;
        break;
        default:
        break;
    }
    
    return *this;
}

RequestData& RequestData::operator=(RequestData &&x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_connectionRequest = std::move(x.m_connectionRequest);
        break;
        case ::OPEN_LOGICAL_PORT:
        m_openLogicalPortRequest = std::move(x.m_openLogicalPortRequest);
        break;
        case ::CHECK_LOGICAL_PORT:
        m_checkLogicalPortsRequest = std::move(x.m_checkLogicalPortsRequest);
        break;
        case ::KEEP_ALIVE:
        m_keepAliveRequest = std::move(x.m_keepAliveRequest);
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        m_logicalPortIsClosedRequest = std::move(x.m_logicalPortIsClosedRequest);
        break;
        default:
        break;
    }
    
    return *this;
}

void RequestData::_d(TCPCommonKind __d)
{
    bool b = false;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        switch(__d)
        {
            case ::BIND_CONNECTION:
            b = true;
            break;
            default:
            break;
        }
        break;
        case ::OPEN_LOGICAL_PORT:
        switch(__d)
        {
            case ::OPEN_LOGICAL_PORT:
            b = true;
            break;
            default:
            break;
        }
        break;
        case ::CHECK_LOGICAL_PORT:
        switch(__d)
        {
            case ::CHECK_LOGICAL_PORT:
            b = true;
            break;
            default:
            break;
        }
        break;
        case ::KEEP_ALIVE:
        switch(__d)
        {
            case ::KEEP_ALIVE:
            b = true;
            break;
            default:
            break;
        }
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        switch(__d)
        {
            case ::LOGICAL_PORT_IS_CLOSED:
            b = true;
            break;
            default:
            break;
        }
        break;
    }
    
    if(!b) throw BadParamException("Discriminator doesn't correspond with the selected union member");
    
    m__d = __d;
}

TCPCommonKind RequestData::_d() const
{
    return m__d;
}

TCPCommonKind& RequestData::_d()
{
    return m__d;
}

void RequestData::connectionRequest(const ConnectionRequest_t &_connectionRequest)
{
    m_connectionRequest = _connectionRequest;
    m__d = ::BIND_CONNECTION;
}

void RequestData::connectionRequest(ConnectionRequest_t &&_connectionRequest)
{
    m_connectionRequest = std::move(_connectionRequest);
    m__d = ::BIND_CONNECTION;
}

const ConnectionRequest_t& RequestData::connectionRequest() const
{
    bool b = false;
        
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_connectionRequest;
}

ConnectionRequest_t& RequestData::connectionRequest()
{
    bool b = false;
        
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_connectionRequest;
}
void RequestData::openLogicalPortRequest(const OpenLogicalPortRequest_t &_openLogicalPortRequest)
{
    m_openLogicalPortRequest = _openLogicalPortRequest;
    m__d = ::OPEN_LOGICAL_PORT;
}

void RequestData::openLogicalPortRequest(OpenLogicalPortRequest_t &&_openLogicalPortRequest)
{
    m_openLogicalPortRequest = std::move(_openLogicalPortRequest);
    m__d = ::OPEN_LOGICAL_PORT;
}

const OpenLogicalPortRequest_t& RequestData::openLogicalPortRequest() const
{
    bool b = false;
        
    switch(m__d)
    {
        case ::OPEN_LOGICAL_PORT:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_openLogicalPortRequest;
}

OpenLogicalPortRequest_t& RequestData::openLogicalPortRequest()
{
    bool b = false;
        
    switch(m__d)
    {
        case ::OPEN_LOGICAL_PORT:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_openLogicalPortRequest;
}
void RequestData::checkLogicalPortsRequest(const CheckLogicalPortsRequest_t &_checkLogicalPortsRequest)
{
    m_checkLogicalPortsRequest = _checkLogicalPortsRequest;
    m__d = ::CHECK_LOGICAL_PORT;
}

void RequestData::checkLogicalPortsRequest(CheckLogicalPortsRequest_t &&_checkLogicalPortsRequest)
{
    m_checkLogicalPortsRequest = std::move(_checkLogicalPortsRequest);
    m__d = ::CHECK_LOGICAL_PORT;
}

const CheckLogicalPortsRequest_t& RequestData::checkLogicalPortsRequest() const
{
    bool b = false;
        
    switch(m__d)
    {
        case ::CHECK_LOGICAL_PORT:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_checkLogicalPortsRequest;
}

CheckLogicalPortsRequest_t& RequestData::checkLogicalPortsRequest()
{
    bool b = false;
        
    switch(m__d)
    {
        case ::CHECK_LOGICAL_PORT:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_checkLogicalPortsRequest;
}
void RequestData::keepAliveRequest(const KeepAliveRequest_t &_keepAliveRequest)
{
    m_keepAliveRequest = _keepAliveRequest;
    m__d = ::KEEP_ALIVE;
}

void RequestData::keepAliveRequest(KeepAliveRequest_t &&_keepAliveRequest)
{
    m_keepAliveRequest = std::move(_keepAliveRequest);
    m__d = ::KEEP_ALIVE;
}

const KeepAliveRequest_t& RequestData::keepAliveRequest() const
{
    bool b = false;
        
    switch(m__d)
    {
        case ::KEEP_ALIVE:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_keepAliveRequest;
}

KeepAliveRequest_t& RequestData::keepAliveRequest()
{
    bool b = false;
        
    switch(m__d)
    {
        case ::KEEP_ALIVE:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_keepAliveRequest;
}
void RequestData::logicalPortIsClosedRequest(const LogicalPortIsClosedRequest_t &_logicalPortIsClosedRequest)
{
    m_logicalPortIsClosedRequest = _logicalPortIsClosedRequest;
    m__d = ::LOGICAL_PORT_IS_CLOSED;
}

void RequestData::logicalPortIsClosedRequest(LogicalPortIsClosedRequest_t &&_logicalPortIsClosedRequest)
{
    m_logicalPortIsClosedRequest = std::move(_logicalPortIsClosedRequest);
    m__d = ::LOGICAL_PORT_IS_CLOSED;
}

const LogicalPortIsClosedRequest_t& RequestData::logicalPortIsClosedRequest() const
{
    bool b = false;
        
    switch(m__d)
    {
        case ::LOGICAL_PORT_IS_CLOSED:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_logicalPortIsClosedRequest;
}

LogicalPortIsClosedRequest_t& RequestData::logicalPortIsClosedRequest()
{
    bool b = false;
        
    switch(m__d)
    {
        case ::LOGICAL_PORT_IS_CLOSED:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_logicalPortIsClosedRequest;
}

size_t RequestData::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
    size_t reset_alignment = 0;
    size_t union_max_size_serialized = 0;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


        reset_alignment = current_alignment;

        reset_alignment += ConnectionRequest_t::getMaxCdrSerializedSize(reset_alignment);

        if(union_max_size_serialized < reset_alignment)
            union_max_size_serialized = reset_alignment;

        
        reset_alignment = current_alignment;

        reset_alignment += OpenLogicalPortRequest_t::getMaxCdrSerializedSize(reset_alignment);

        if(union_max_size_serialized < reset_alignment)
            union_max_size_serialized = reset_alignment;

        
        reset_alignment = current_alignment;

        reset_alignment += CheckLogicalPortsRequest_t::getMaxCdrSerializedSize(reset_alignment);

        if(union_max_size_serialized < reset_alignment)
            union_max_size_serialized = reset_alignment;

        
        reset_alignment = current_alignment;

        reset_alignment += KeepAliveRequest_t::getMaxCdrSerializedSize(reset_alignment);

        if(union_max_size_serialized < reset_alignment)
            union_max_size_serialized = reset_alignment;

        
        reset_alignment = current_alignment;

        reset_alignment += LogicalPortIsClosedRequest_t::getMaxCdrSerializedSize(reset_alignment);

        if(union_max_size_serialized < reset_alignment)
            union_max_size_serialized = reset_alignment;

        

    return union_max_size_serialized - initial_alignment;
}

// TODO(Ricardo) Review
size_t RequestData::getCdrSerializedSize(const RequestData& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    switch(data.m__d)
    {
        case ::BIND_CONNECTION:
        current_alignment += ConnectionRequest_t::getCdrSerializedSize(data.connectionRequest(), current_alignment);
        break;
        case ::OPEN_LOGICAL_PORT:
        current_alignment += OpenLogicalPortRequest_t::getCdrSerializedSize(data.openLogicalPortRequest(), current_alignment);
        break;
        case ::CHECK_LOGICAL_PORT:
        current_alignment += CheckLogicalPortsRequest_t::getCdrSerializedSize(data.checkLogicalPortsRequest(), current_alignment);
        break;
        case ::KEEP_ALIVE:
        current_alignment += KeepAliveRequest_t::getCdrSerializedSize(data.keepAliveRequest(), current_alignment);
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        current_alignment += LogicalPortIsClosedRequest_t::getCdrSerializedSize(data.logicalPortIsClosedRequest(), current_alignment);
        break;
        default:
        break;
    }

    return current_alignment - initial_alignment;
}

void RequestData::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << (uint32_t)m__d;

    switch(m__d)
    {
        case ::BIND_CONNECTION:
        scdr << m_connectionRequest;
        break;
        case ::OPEN_LOGICAL_PORT:
        scdr << m_openLogicalPortRequest;
        break;
        case ::CHECK_LOGICAL_PORT:
        scdr << m_checkLogicalPortsRequest;
        break;
        case ::KEEP_ALIVE:
        scdr << m_keepAliveRequest;
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        scdr << m_logicalPortIsClosedRequest;
        break;
        default:
        break;
    }
}

void RequestData::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> (uint32_t&)m__d;

    switch(m__d)
    {
        case ::BIND_CONNECTION:
        dcdr >> m_connectionRequest;
        break;
        case ::OPEN_LOGICAL_PORT:
        dcdr >> m_openLogicalPortRequest;
        break;
        case ::CHECK_LOGICAL_PORT:
        dcdr >> m_checkLogicalPortsRequest;
        break;
        case ::KEEP_ALIVE:
        dcdr >> m_keepAliveRequest;
        break;
        case ::LOGICAL_PORT_IS_CLOSED:
        dcdr >> m_logicalPortIsClosedRequest;
        break;
        default:
        break;
    }
}


ControlProtocolRequestData::ControlProtocolRequestData()
{
}

ControlProtocolRequestData::~ControlProtocolRequestData()
{
}

ControlProtocolRequestData::ControlProtocolRequestData(const ControlProtocolRequestData &x)
{
    m_requestData = x.m_requestData;
}

ControlProtocolRequestData::ControlProtocolRequestData(ControlProtocolRequestData &&x)
{
    m_requestData = std::move(x.m_requestData);
}

ControlProtocolRequestData& ControlProtocolRequestData::operator=(const ControlProtocolRequestData &x)
{
    m_requestData = x.m_requestData;
    
    return *this;
}

ControlProtocolRequestData& ControlProtocolRequestData::operator=(ControlProtocolRequestData &&x)
{
    m_requestData = std::move(x.m_requestData);
    
    return *this;
}

size_t ControlProtocolRequestData::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += RequestData::getMaxCdrSerializedSize(current_alignment);

    return current_alignment - initial_alignment;
}

size_t ControlProtocolRequestData::getCdrSerializedSize(const ControlProtocolRequestData& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += RequestData::getCdrSerializedSize(data.requestData(), current_alignment);

    return current_alignment - initial_alignment;
}

void ControlProtocolRequestData::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_requestData;
}

void ControlProtocolRequestData::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_requestData;
}

size_t ControlProtocolRequestData::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool ControlProtocolRequestData::isKeyDefined()
{
    return false;
}

void ControlProtocolRequestData::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}

BindConnectionResponse_t::BindConnectionResponse_t()
{
}

BindConnectionResponse_t::~BindConnectionResponse_t()
{
}

BindConnectionResponse_t::BindConnectionResponse_t(const BindConnectionResponse_t &x)
{
    m_locator = x.m_locator;
}

BindConnectionResponse_t::BindConnectionResponse_t(BindConnectionResponse_t &&x)
{
    m_locator = x.m_locator;
}

BindConnectionResponse_t& BindConnectionResponse_t::operator=(const BindConnectionResponse_t &x)
{
    m_locator = x.m_locator;
    
    return *this;
}

BindConnectionResponse_t& BindConnectionResponse_t::operator=(BindConnectionResponse_t &&x)
{
    m_locator = x.m_locator;
    
    return *this;
}

size_t BindConnectionResponse_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t BindConnectionResponse_t::getCdrSerializedSize(const BindConnectionResponse_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void BindConnectionResponse_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_locator;
}

void BindConnectionResponse_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_locator;
}

size_t BindConnectionResponse_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool BindConnectionResponse_t::isKeyDefined()
{
    return false;
}

void BindConnectionResponse_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}
CheckLogicalPortsResponse_t::CheckLogicalPortsResponse_t()
{
}

CheckLogicalPortsResponse_t::~CheckLogicalPortsResponse_t()
{
}

CheckLogicalPortsResponse_t::CheckLogicalPortsResponse_t(const CheckLogicalPortsResponse_t &x)
{
    m_availableLogicalPorts = x.m_availableLogicalPorts;
}

CheckLogicalPortsResponse_t::CheckLogicalPortsResponse_t(CheckLogicalPortsResponse_t &&x)
{
    m_availableLogicalPorts = std::move(x.m_availableLogicalPorts);
}

CheckLogicalPortsResponse_t& CheckLogicalPortsResponse_t::operator=(const CheckLogicalPortsResponse_t &x)
{
    m_availableLogicalPorts = x.m_availableLogicalPorts;
    
    return *this;
}

CheckLogicalPortsResponse_t& CheckLogicalPortsResponse_t::operator=(CheckLogicalPortsResponse_t &&x)
{
    m_availableLogicalPorts = std::move(x.m_availableLogicalPorts);
    
    return *this;
}

size_t CheckLogicalPortsResponse_t::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += (100 * 2) + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

size_t CheckLogicalPortsResponse_t::getCdrSerializedSize(const CheckLogicalPortsResponse_t& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += (data.availableLogicalPorts().size() * 2) + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}

void CheckLogicalPortsResponse_t::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << m_availableLogicalPorts;
}

void CheckLogicalPortsResponse_t::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> m_availableLogicalPorts;
}

size_t CheckLogicalPortsResponse_t::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            

    return current_align;
}

bool CheckLogicalPortsResponse_t::isKeyDefined()
{
    return false;
}

void CheckLogicalPortsResponse_t::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
}
ResponseData::ResponseData()
{
    m__d = ::BIND_CONNECTION;
}

ResponseData::~ResponseData()
{
}

ResponseData::ResponseData(const ResponseData &x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_bindConnectionResponse = x.m_bindConnectionResponse;
        break;
        default:
        break;
    }
}

ResponseData::ResponseData(ResponseData &&x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_bindConnectionResponse = std::move(x.m_bindConnectionResponse);
        break;
        default:
        break;
    }
}

ResponseData& ResponseData::operator=(const ResponseData &x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_bindConnectionResponse = x.m_bindConnectionResponse;
        break;
        default:
        break;
    }
    
    return *this;
}

ResponseData& ResponseData::operator=(ResponseData &&x)
{
    m__d = x.m__d;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        m_bindConnectionResponse = std::move(x.m_bindConnectionResponse);
        break;
        default:
        break;
    }
    
    return *this;
}

void ResponseData::_d(TCPCommonKind __d)
{
    bool b = false;
    
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        switch(__d)
        {
            case ::BIND_CONNECTION:
            b = true;
            break;
            default:
            break;
        }
        break;
    }
    
    if(!b) throw BadParamException("Discriminator doesn't correspond with the selected union member");
    
    m__d = __d;
}

TCPCommonKind ResponseData::_d() const
{
    return m__d;
}

TCPCommonKind& ResponseData::_d()
{
    return m__d;
}

void ResponseData::bindConnectionResponse(const BindConnectionResponse_t &_bindConnectionResponse)
{
    m_bindConnectionResponse = _bindConnectionResponse;
    m__d = ::BIND_CONNECTION;
}

void ResponseData::bindConnectionResponse(BindConnectionResponse_t &&_bindConnectionResponse)
{
    m_bindConnectionResponse = std::move(_bindConnectionResponse);
    m__d = ::BIND_CONNECTION;
}

const BindConnectionResponse_t& ResponseData::bindConnectionResponse() const
{
    bool b = false;
        
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_bindConnectionResponse;
}

BindConnectionResponse_t& ResponseData::bindConnectionResponse()
{
    bool b = false;
        
    switch(m__d)
    {
        case ::BIND_CONNECTION:
        b = true;
        break;
        default:
        break;
    }    
    if(!b) throw BadParamException("This member is not been selected");
    
    return m_bindConnectionResponse;
}

size_t ResponseData::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
    size_t reset_alignment = 0;
    size_t union_max_size_serialized = 0;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


        reset_alignment = current_alignment;

        reset_alignment += BindConnectionResponse_t::getMaxCdrSerializedSize(reset_alignment);

        if(union_max_size_serialized < reset_alignment)
            union_max_size_serialized = reset_alignment;

        

    return union_max_size_serialized - initial_alignment;
}

// TODO(Ricardo) Review
size_t ResponseData::getCdrSerializedSize(const ResponseData& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    switch(data.m__d)
    {
        case ::BIND_CONNECTION:
        current_alignment += BindConnectionResponse_t::getCdrSerializedSize(data.bindConnectionResponse(), current_alignment);
        break;
        default:
        break;
    }

    return current_alignment - initial_alignment;
}

void ResponseData::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << (uint32_t)m__d;

    switch(m__d)
    {
        case ::BIND_CONNECTION:
        scdr << m_bindConnectionResponse;
        break;
        default:
        break;
    }
}

void ResponseData::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> (uint32_t&)m__d;

    switch(m__d)
    {
        case ::BIND_CONNECTION:
        dcdr >> m_bindConnectionResponse;
        break;
        default:
        break;
    }
}


ControlProtocolResponseData::ControlProtocolResponseData()
{
    m_responseCode = ::RETCODE_OK;

}

ControlProtocolResponseData::~ControlProtocolResponseData()
{
}

ControlProtocolResponseData::ControlProtocolResponseData(const ControlProtocolResponseData &x)
{
    m_responseCode = x.m_responseCode;
    m_responseData = x.m_responseData;
}

ControlProtocolResponseData::ControlProtocolResponseData(ControlProtocolResponseData &&x)
{
    m_responseCode = x.m_responseCode;
    m_responseData = std::move(x.m_responseData);
}

ControlProtocolResponseData& ControlProtocolResponseData::operator=(const ControlProtocolResponseData &x)
{
    m_responseCode = x.m_responseCode;
    m_responseData = x.m_responseData;
    
    return *this;
}

ControlProtocolResponseData& ControlProtocolResponseData::operator=(ControlProtocolResponseData &&x)
{
    m_responseCode = x.m_responseCode;
    m_responseData = std::move(x.m_responseData);
    
    return *this;
}

size_t ControlProtocolResponseData::getMaxCdrSerializedSize(size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    current_alignment += ResponseData::getMaxCdrSerializedSize(current_alignment);

    return current_alignment - initial_alignment;
}

size_t ControlProtocolResponseData::getCdrSerializedSize(const ControlProtocolResponseData& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;
            
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    current_alignment += ResponseData::getCdrSerializedSize(data.responseData(), current_alignment);

    return current_alignment - initial_alignment;
}

void ControlProtocolResponseData::serialize(eprosima::fastcdr::Cdr &scdr) const
{
    scdr << (uint32_t)m_responseCode;
    scdr << m_responseData;
}

void ControlProtocolResponseData::deserialize(eprosima::fastcdr::Cdr &dcdr)
{
    dcdr >> (uint32_t&)m_responseCode;
    dcdr >> m_responseData;
}

size_t ControlProtocolResponseData::getKeyMaxCdrSerializedSize(size_t current_alignment)
{
	size_t current_align = current_alignment;
            



    return current_align;
}

bool ControlProtocolResponseData::isKeyDefined()
{
    return false;
}

void ControlProtocolResponseData::serializeKey(eprosima::fastcdr::Cdr &scdr) const
{
	 
	 
}