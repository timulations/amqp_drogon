import pika

def callback(ch, method, properties, body):
    print(f" [x] Received {body}")

# Establish a connection with RabbitMQ server
connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()

# Declare the same queue as the producer to ensure we're reading from the correct place
channel.queue_declare(queue='hello')

# Subscribe the callback function to the queue
channel.basic_consume(queue='hello',
                      on_message_callback=callback,
                      auto_ack=True)

print(' [*] Waiting for messages. To exit press CTRL+C')
channel.start_consuming()
