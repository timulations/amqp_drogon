# AMQP RabbitMQ Drogon Plugin
A Drogon Plugin that integrates AMQP-CPP into the main Drogon event loop, utilising Trantor for TCP communications with AMQP service.

## How to Use
### Config
The plugin is responsible for communicating with a single RabbitMQ service, and a single service only. Add this entry into your `plugins` section of your `config.json`:

```
    "plugins": [
        ...
        {
            "name": "AMQPClient",
            "config": {
                "ip": "127.0.0.1",    # your AMQP server IP
                "port": 5672,         # your AMQP server port
                "user": "guest",      # your AMQP server login
                "password": "guest",  # your AMQP server password
                "vhost": "/"
            }
        }
    ]
```

### Invoking the Plugin

Invoke the plugin like below to get a `std::shared_ptr<AMQP::Channel>`. You can then use this pointer to invoke the API offered by `AMQP::Channel` to produce and consume messages. 
```cpp
    app().registerHandler(
        "/produce",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {


            auto amqpPluginPtr = app().getPlugin<AMQPClient>(); /* get the plugin ptr */
            auto channel1 = amqpPluginPtr->createChannel("channel1"); /* create channel. This will fetch existing channel of same name if exists */
            LOG_TRACE << "Channel created";

            /* Get a AMQP::Channel pointer that you can use. Refer to the AMQP-CPP API for more details */
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
```

### Test Example
1. Build the example shown in `main.cc`.
2. Run `python3 consumer.py` to create a mock RabbitMQ listener.
3. Use `curl -X GET 'http://127.0.0.1:5000/produce'` to invoke the `/produce` endpoint to send a message to the `"hello"` queue.
4. Watch the message come up on `python3 consumer.py`
