/**
 *
 *  AMQPClient.h
 *
 */

#pragma once

#include <drogon/plugins/Plugin.h>
#include <amqpcpp.h>
#include <memory>
#include <trantor/net/TcpClient.h>
#include <mutex>
#include <thread>
#include <atomic>

class TrantorConnectionHandler : public AMQP::ConnectionHandler 
{
    std::shared_ptr<trantor::TcpClient> client;
    AMQP::Connection* pConnection;
    std::vector<char> buffer;
    trantor::EventLoop* evLoop;
    std::queue<std::string> sendQueue;
    std::mutex mtx;
    std::thread sendingThread;
    std::atomic<bool> sendingLoopDone;

    void sendDataLoop();
public:
    /* create a TCP client on the given hostname and port, running on the specified trantor/drogon event loop */
    TrantorConnectionHandler(const std::string& hostname, int port, trantor::EventLoop* loop);

    void setup(std::function<void()> cb);
    void onData(AMQP::Connection *connection, const char *data, size_t size) override;
    void onReady(AMQP::Connection *connection) override;
    void onError(AMQP::Connection *connection, const char *message) override;
    void onClosed(AMQP::Connection *connection) override;
};

/* The AMQP-CPP opens a single TCP connection to the RabbitMQ message broker service running
 * on the IP and port specified in the config file.
 *
 * Only supports a AMQP Connection (single message broker service connection).
 */
class AMQPClient : public drogon::Plugin<AMQPClient>
{
  public:
    using amqpChannelPtr = std::shared_ptr<AMQP::Channel>;

    /// This method must be called by drogon to initialize and start the plugin.
    /// It must be implemented by the user.
    void initAndStart(const Json::Value &config) override;

    /// This method must be called by drogon to shutdown the plugin.
    /// It must be implemented by the user.
    void shutdown() override;

    amqpChannelPtr createChannel(const std::string& channelName);
    std::optional<amqpChannelPtr> getChannel(const std::string& channelName);
    bool eraseChannel(const std::string& channelName);

  private:
    std::unique_ptr<TrantorConnectionHandler> pHandler;
    std::unique_ptr<AMQP::Connection> connection;
    std::map<std::string, amqpChannelPtr> channels;
};



