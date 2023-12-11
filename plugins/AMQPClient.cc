/**
 *
 *  AMQPClient.cc
 *
 */

#include "AMQPClient.h"
#include <drogon/drogon.h>

#include <vector>
#include <trantor/net/EventLoop.h>
#include <trantor/net/Resolver.h>

using namespace drogon;


TrantorConnectionHandler::TrantorConnectionHandler(const std::string& hostname, int port, trantor::EventLoop* loop)
    :client{std::make_shared<trantor::TcpClient>(loop, trantor::InetAddress(hostname, port), "AMQPCPP")}
    ,pConnection{nullptr}
    ,evLoop{loop}
    ,sendingLoopDone{true}
{}

void TrantorConnectionHandler::setup(std::function<void()> cb) {
    client->setConnectionCallback(
        [this, cb=cb](const trantor::TcpConnectionPtr &conn) {
            if (conn->connected())
            {
                LOG_DEBUG << "connected to AMQP broker, yay!";
                cb();

                sendingLoopDone.store(false);

                /* start the sending loop in another thread */
                sendingThread = std::move(std::thread([this] { sendDataLoop(); }));
            }
            else
            {
                sendingLoopDone.store(true);
                sendingThread.join();
            }
        });

        client->setMessageCallback(
        [this](const trantor::TcpConnectionPtr &conn, trantor::MsgBuffer *buf) {
            auto recvData = std::string(buf->peek(), buf->readableBytes());
            buf->retrieveAll();

            if (pConnection) 
            {
                buffer.insert(buffer.end(), recvData.begin(), recvData.end());

                size_t parsed = pConnection->parse(buffer.data(), buffer.size());
                if (parsed > 0) 
                {
                    buffer.erase(buffer.begin(), buffer.begin() + parsed);
                }
            }
        });

    client->connect();
}

void TrantorConnectionHandler::sendDataLoop() {
    while (!sendingLoopDone.load()) {
        std::string dataToSend;
        {
            std::scoped_lock<std::mutex> lck(mtx);
            if (!sendQueue.empty())
            {
                dataToSend = sendQueue.front();
                sendQueue.pop();
            }
        }

        if (dataToSend.size() > 0)
        {
            client->connection()->send(dataToSend.data(), dataToSend.size());
        }
    }
}

void TrantorConnectionHandler::onData(AMQP::Connection *connection, const char *data, size_t size) 
{
    if (!pConnection) pConnection = connection;
    std::scoped_lock<std::mutex> lck(mtx);
    sendQueue.emplace(data, size);
}

void TrantorConnectionHandler::onReady(AMQP::Connection *connection)
{
    if (!pConnection) pConnection = connection;
    /* ready logic */
}

void TrantorConnectionHandler::onError(AMQP::Connection *connection, const char *message)
{
    if (!pConnection) pConnection = connection;
}

void TrantorConnectionHandler::onClosed(AMQP::Connection *connection)
{
    if (pConnection) pConnection = nullptr;
    client->disconnect();
}


void AMQPClient::initAndStart(const Json::Value &config)
{
    /* establish TCP connection with the message broking service specified in the config */
    // TODO: come up with a way to be able to specify which loop the AMQP handler lives, or have a dedicated event loop for it.
    pHandler = std::make_unique<TrantorConnectionHandler>(config["ip"].asString(), config["port"].asInt(), app().getLoop());

    pHandler->setup([this, config](){
        connection = std::make_unique<AMQP::Connection>(pHandler.get(), AMQP::Login(config["user"].asCString(), config["password"].asCString()), config["vhost"].asCString()); 
    });
}

void AMQPClient::shutdown() 
{
    /// Shutdown the plugin
    pHandler.reset();
}

AMQPClient::amqpChannelPtr AMQPClient::createChannel(const std::string& channelName) 
{
    if (channels.count(channelName)) return channels[channelName];

    auto ret = std::make_shared<AMQP::Channel>(connection.get());
    channels[channelName] = ret;

    return ret;
}

std::optional<AMQPClient::amqpChannelPtr> AMQPClient::getChannel(const std::string& channelName) 
{
    if (channels.count(channelName)) return channels[channelName];
    else return std::nullopt;
}

bool AMQPClient::eraseChannel(const std::string& channelName) 
{
    if (!channels.count(channelName)) return false;
    channels.erase(channelName);
    return true;
}