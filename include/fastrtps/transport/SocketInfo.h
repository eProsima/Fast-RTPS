#ifndef RTPS_SOCKET_INFO_
#define RTPS_SOCKET_INFO_

#include <asio.hpp>
#include <memory>
#include <fastrtps/rtps/messages/MessageReceiver.h>

namespace eprosima{
namespace fastrtps{
namespace rtps{

#if defined(ASIO_HAS_MOVE)
    // Typedefs
	typedef asio::ip::udp::socket eProsimaUDPSocket;
	typedef asio::ip::tcp::socket eProsimaTCPSocket;
    // UDP
	inline eProsimaUDPSocket* getSocketPtr(eProsimaUDPSocket &socket)
    {
        return &socket;
    }
    inline eProsimaUDPSocket moveSocket(eProsimaUDPSocket &socket)
    {
        return std::move(socket);
    }
    inline eProsimaUDPSocket createUDPSocket(asio::io_service& io_service)
    {
        return std::move(asio::ip::udp::socket(io_service));
    }
    inline eProsimaUDPSocket& getRefFromPtr(eProsimaUDPSocket* socket)
    {
        return *socket;
    }
    // TCP
	inline eProsimaTCPSocket* getSocketPtr(eProsimaTCPSocket &socket)
    {
        return &socket;
    }
    inline eProsimaTCPSocket moveSocket(eProsimaTCPSocket &socket)
    {
        return std::move(socket);
    }
    inline eProsimaTCPSocket createTCPSocket(asio::io_service& io_service)
    {
        return std::move(asio::ip::tcp::socket(io_service));
    }
    inline eProsimaTCPSocket& getRefFromPtr(eProsimaTCPSocket* socket)
    {
        return *socket;
    }
#else
    // Typedefs
	typedef std::shared_ptr<asio::ip::udp::socket> eProsimaUDPSocket;
	typedef std::shared_ptr<asio::ip::tcp::socket> eProsimaTCPSocket;
    // UDP
    inline eProsimaUDPSocket getSocketPtr(eProsimaUDPSocket &socket)
    {
        return socket;
    }
    inline eProsimaUDPSocket& moveSocket(eProsimaUDPSocket &socket)
    {
        return socket;
    }
    inline eProsimaUDPSocket createUDPSocket(asio::io_service& io_service)
    {
        return std::make_shared<asio::ip::udp::socket>(io_service);
    }
    inline eProsimaUDPSocket& getRefFromPtr(eProsimaUDPSocket &socket)
    {
        return socket;
    }
    // TCP
    inline eProsimaTCPSocket getSocketPtr(eProsimaTCPSocket &socket)
    {
        return socket;
    }
    inline eProsimaTCPSocket& moveSocket(eProsimaTCPSocket &socket)
    {
        return socket;
    }
    inline eProsimaTCPSocket createTCPSocket(asio::io_service& io_service)
    {
        return std::make_shared<asio::ip::tcp::socket>(io_service);
    }
    inline eProsimaTCPSocket& getRefFromPtr(eProsimaTCPSocket &socket)
    {
        return socket;
    }
#endif

class SocketInfo
{
public:
    SocketInfo()
    : mp_receiver(nullptr)
    , m_bAlive(true)
    , m_thread(nullptr)
    {
    }

    virtual ~SocketInfo()
    {
        m_bAlive = false;
        if (m_thread != nullptr)
        {
            m_thread->join();
            delete m_thread;
        }
        mp_receiver = nullptr;
    }

    inline void SetThread(std::thread* pThread)
    {
        m_thread = pThread;
    }

    inline bool IsAlive() const
    {
        return m_bAlive;
    }

    inline void Disable()
    {
        m_bAlive = false;
    }

    inline void SetMessageReceiver(std::shared_ptr<MessageReceiver> receiver)
    {
        mp_receiver = receiver;
    }

    inline std::shared_ptr<MessageReceiver> GetMessageReceiver()
    {
        return mp_receiver;
    }
protected:
    std::shared_ptr<MessageReceiver> mp_receiver; //Associated Readers/Writers inside of MessageReceiver
    bool m_bAlive;
    std::thread* m_thread;
};

class UDPSocketInfo : public SocketInfo
{
public:
    UDPSocketInfo(eProsimaUDPSocket& socket)
        : socket_(moveSocket(socket))
    {
    }

    UDPSocketInfo(UDPSocketInfo&& socketInfo)
        : socket_(moveSocket(socketInfo.socket_))
    {
    }

    virtual ~UDPSocketInfo()
    {
    }

    UDPSocketInfo& operator=(UDPSocketInfo&& socketInfo)
    {
        socket_ = moveSocket(socketInfo.socket_);
        return *this;
    }

    void only_multicast_purpose(const bool value)
    {
        only_multicast_purpose_ = value;
    };

    bool& only_multicast_purpose()
    {
        return only_multicast_purpose_;
    }

    bool only_multicast_purpose() const
    {
        return only_multicast_purpose_;
    }

#if defined(ASIO_HAS_MOVE)
    inline eProsimaUDPSocket* getSocket()
#else
    inline eProsimaUDPSocket getSocket()
#endif
    {
        return getSocketPtr(socket_);
    }

private:

    eProsimaUDPSocket socket_;
    bool only_multicast_purpose_;
    UDPSocketInfo(const UDPSocketInfo&) = delete;
    UDPSocketInfo& operator=(const UDPSocketInfo&) = delete;
};

class TCPSocketInfo : public SocketInfo
{
    public:
        TCPSocketInfo(eProsimaTCPSocket& socket)
            : socket_(moveSocket(socket))
            , m_physicalPort(0)
        {
            mMutex = std::make_shared<std::recursive_mutex>();
        }

        TCPSocketInfo(TCPSocketInfo&& socketInfo)
            : socket_(moveSocket(socketInfo.socket_))
            , m_physicalPort(0)
        {
            mMutex = std::make_shared<std::recursive_mutex>();
        }

        virtual ~TCPSocketInfo()
        {
        }

        TCPSocketInfo& operator=(TCPSocketInfo&& socketInfo)
        {
            socket_ = moveSocket(socketInfo.socket_);
            return *this;
        }

        bool operator==(const TCPSocketInfo& socketInfo) const
        {
            return &socket_ == &(socketInfo.socket_);
        }

        void only_multicast_purpose(const bool value)
        {
            only_multicast_purpose_ = value;
        };

        bool& only_multicast_purpose()
        {
            return only_multicast_purpose_;
        }

        bool only_multicast_purpose() const
        {
            return only_multicast_purpose_;
        }

#if defined(ASIO_HAS_MOVE)
        inline eProsimaTCPSocket* getSocket()
#else
        inline eProsimaTCPSocket getSocket()
#endif
        {
            return getSocketPtr(socket_);
        }

        std::recursive_mutex& GetMutex() const
        {
            return *mMutex;
        }

        inline void SetPhysicalPort(uint16_t port)
        {
            m_physicalPort = port;
        }

        inline uint16_t GetPhysicalPort() const
        {
            return m_physicalPort;
        }
    private:

        uint16_t m_physicalPort;
        std::shared_ptr<std::recursive_mutex> mMutex;
        eProsimaTCPSocket socket_;
        bool only_multicast_purpose_;
        TCPSocketInfo(const TCPSocketInfo&) = delete;
        TCPSocketInfo& operator=(const TCPSocketInfo&) = delete;
};


} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // RTPS_TCP_PORT_MANAGER_