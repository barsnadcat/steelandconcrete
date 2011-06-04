#include <pch.h>
#include <Network.h>

#include <Header.pb.h>
#include <ServerLog.h>

Network::Network(SocketSharedPtr aSocket): mSocket(aSocket), mMessageBuffer(NULL),
mBufferSize(0), mAsync(false)
{
    assert(aSocket);
    HeaderMsg header;
    header.set_size(0);
    mHeaderSize = header.ByteSize();
}

Network::~Network()
{
    delete mMessageBuffer;
    mSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    mSocket->close();
}

void Network::AllocBuffer(int aSize)
{
    if (aSize > mBufferSize)
    {
        delete mMessageBuffer;
        mMessageBuffer = new char[aSize];
        mBufferSize = aSize;
    }
}

void Network::WriteMessage(const google::protobuf::Message& aMessage)
{
    std::cout << "NET:WriteMessage " << aMessage.ShortDebugString() << std::endl;
    if (mAsync)
    {
        boost::throw_exception(std::runtime_error("Async op in progress"));
    }
    size_t messageSize = aMessage.ByteSize();
    AllocBuffer(messageSize);
    aMessage.SerializeToArray(mMessageBuffer, messageSize);

    HeaderMsg header;
    header.set_size(messageSize);
    size_t headerSize = header.ByteSize();
    header.SerializeToArray(mHeaderBuffer, headerSize);

    if (boost::asio::write(*mSocket, boost::asio::buffer(mHeaderBuffer, headerSize)) != headerSize)
    {
        boost::throw_exception(std::runtime_error("Неудалось записать в сокет загловок!"));
    }
    if (boost::asio::write(*mSocket, boost::asio::buffer(mMessageBuffer, messageSize)) != messageSize)
    {
        boost::throw_exception(std::runtime_error("Неудалось записать в сокет сообщение!"));
    }
}

void Network::ReadMessage(google::protobuf::Message& aMessage)
{
    std::cout << "NET:ReadMessage " << aMessage.ShortDebugString() << std::endl;
    if (mAsync)
    {
        boost::throw_exception(std::runtime_error("Async op in progress"));
    }
    if (boost::asio::read(*mSocket, boost::asio::buffer(mHeaderBuffer, mHeaderSize)) != mHeaderSize)
    {
        boost::throw_exception(std::runtime_error("Не удалось прочитать из сокета заголовок!"));
    }
    HeaderMsg header;
    if (!header.ParseFromArray(mHeaderBuffer, mHeaderSize))
    {
        boost::throw_exception(std::runtime_error("Не удалось разобрать заголовок!"));
    }

    size_t messageSize = header.size();
    AllocBuffer(messageSize);

    if (boost::asio::read(*mSocket, boost::asio::buffer(mMessageBuffer, messageSize)) != messageSize)
    {
        boost::throw_exception(std::runtime_error("Не удалось прочитать из сокета сообщение!"));
    }
    if (!aMessage.ParseFromArray(mMessageBuffer, messageSize))
    {
        boost::throw_exception(std::runtime_error("Не удалось разобрать сообщение!"));
    }
}


void Network::AsyncReadMessage(ResponseCallBack aCallBack)
{
    boost::asio::async_read(*mSocket, boost::asio::buffer(mHeaderBuffer, mHeaderSize),
                            boost::bind(&Network::ParseHeader,
                                        this, aCallBack,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void Network::ParseHeader(ResponseCallBack aCallBack,
                          const boost::system::error_code& aError,
                          std::size_t aBytesTransferred)
{
    std::cout << "NET:ParseHeader " << aError << " " << aBytesTransferred << std::endl;
    if (aError)
    {
        boost::throw_exception(std::runtime_error("Не удалось прочитать из сокета заголовок!"));
    }

    HeaderMsg header;
    if (!header.ParseFromArray(mHeaderBuffer, mHeaderSize))
    {
        boost::throw_exception(std::runtime_error("Не удалось разобрать заголовок!"));
    }

    size_t messageSize = header.size();
    AllocBuffer(messageSize);

    boost::asio::async_read(*mSocket, boost::asio::buffer(mMessageBuffer, messageSize),
                            boost::bind(&Network::ParseMessage,
                                        this, aCallBack,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}


void Network::ParseMessage(ResponseCallBack aCallBack,
                           const boost::system::error_code& aError,
                           std::size_t aBytesTransferred)
{
    std::cout << "NET:ParseMessage " << aError << " " << aBytesTransferred << std::endl;
    if (aError)
    {
        boost::throw_exception(std::runtime_error("Не удалось прочитать из сокета сообщение!"));
    }

    boost::shared_ptr<ResponseMsg> msg(new ResponseMsg());
    if (!msg->ParseFromArray(mMessageBuffer, aBytesTransferred))
    {
        boost::throw_exception(std::runtime_error("Не удалось разобрать сообщение!"));
    }


    if (msg->type() == RESPONSE_PART)
    {
        aCallBack(msg);
        AsyncReadMessage(aCallBack);
    }
    else
    {
        mAsync = false;
        aCallBack(msg);
    }
}

void Network::Request(ResponseCallBack aCallBack, RequestPtr aRequestMsg)
{
    std::cout << "NET:Request " << aRequestMsg->ShortDebugString() << std::endl;
    if (mAsync)
    {
        boost::throw_exception(std::runtime_error("Async op in progress"));
    }

    mAsync = true;
    size_t messageSize = aRequestMsg->ByteSize();
    AllocBuffer(messageSize);
    aRequestMsg->SerializeToArray(mMessageBuffer, messageSize);

    HeaderMsg header;
    header.set_size(messageSize);
    size_t headerSize = header.ByteSize();
    header.SerializeToArray(mHeaderBuffer, headerSize);

    if (boost::asio::write(*mSocket, boost::asio::buffer(mHeaderBuffer, headerSize)) != headerSize)
    {
        boost::throw_exception(std::runtime_error("Неудалось записать в сокет загловок!"));
    }
    if (boost::asio::write(*mSocket, boost::asio::buffer(mMessageBuffer, messageSize)) != messageSize)
    {
        boost::throw_exception(std::runtime_error("Неудалось записать в сокет сообщение!"));
    }

    AsyncReadMessage(aCallBack);
}
