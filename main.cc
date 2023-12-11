#include <drogon/drogon.h>
#include <string>
#include "../plugins/AMQPClient.h"

using namespace drogon;

int main()
{
    app().loadConfigFile("../config.json");

    /* examples */
    app().registerHandler(
        "/produce",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            auto amqpPluginPtr = app().getPlugin<AMQPClient>();
            auto channel1 = amqpPluginPtr->createChannel("channel1");
            LOG_TRACE << "Channel created";
            channel1->declareQueue("hello")
                .onSuccess([](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
                
                    // report the name of the temporary queue
                    std::cout << "declared queue " << name << std::endl;
                    
                    app().getPlugin<AMQPClient>()->getChannel("channel1").value()->publish  ("", "hello", "Hello world! What's up, my guy!");
                    LOG_TRACE << "PUBLISHED MESSAGE";
                })
                .onError([](const char *message) {
                    LOG_TRACE << "Queue declare error: " << message;
                })
                .onFinalize([](){
                    LOG_TRACE << "Queue declare finalized";
                });

            Json::Value json;
            json["result"]="ok";
            auto resp=HttpResponse::newHttpJsonResponse(json);
            callback(resp);
        });

    app().setLogLevel(trantor::Logger::kTrace);
    app().run();
}