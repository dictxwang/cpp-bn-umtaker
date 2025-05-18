#ifndef _ZMQ_CLIENT_H_
#define _ZMQ_CLIENT_H_

#include <zmq.h>
#include "logger/logger.h"

class ZMQClient {
 public:
    ZMQClient(int role_type) {
        role_type_ = role_type;
        context_ = zmq_ctx_new();
        requester_ = zmq_socket(context_, role_type_);
    }

    virtual ~ZMQClient() {
        zmq_close(requester_);
        zmq_ctx_destroy(context_);
    }

    void PublisherBind(const std::string& host) {
        int ret = zmq_bind(requester_, host.c_str());
        assert(ret == 0);
    }

    void SubscriberConnect(const std::string& host) {
        int ret = zmq_connect(requester_, host.c_str());
        assert(ret == 0);
        // subscribe all topics
        zmq_setsockopt(requester_, ZMQ_SUBSCRIBE, "", 0);
    }

    void Send(const std::string& msg) {
        int ret = zmq_send(requester_, msg.c_str(), msg.size(), 0);         // ZMQ_DONTWAIT

        if (ret == -1) {
            throw std::runtime_error("send error, errno=" + std::to_string(zmq_errno()) + ", msg=" + std::string(zmq_strerror(zmq_errno())));
        }
    }

    std::string Receive() {

        // char topic[256];
        char message[1024];

        // // Receive the topic
        // int topic_size = zmq_recv(requester_, topic, sizeof(topic) - 1, 0);
        // if (topic_size == -1) {
        //     std::cerr << "Failed to receive topic: " << zmq_strerror(zmq_errno()) << std::endl;
        // }
        // topic[topic_size] = '\0'; // Null-terminate the topic string

        // Receive the payload
        int message_size = zmq_recv(requester_, message, sizeof(message) - 1, 0);
        if (message_size == -1) {
            throw std::runtime_error("receive error, errno=" + std::to_string(zmq_errno()) + ", msg=" + std::string(zmq_strerror(zmq_errno())));
        }
        message[message_size] = '\0'; // Null-terminate the message string

        // return std::string(topic) + ":" + std::string(message);
        return std::string(message);
    }
 private:
    void* context_;
    void* requester_;
    int role_type_;
};

#endif