#if 0
// Start by `#include`-ing the Mosquitto MQTT Library and other standard libraries.
#include <mqtt/client.h>  // Mosquitto client.
#include <ostream>  // std::cout.
#include <chrono>  // Time keeping.
#include <thread>  // Sleep.

// With the library header files included, continue by defining a main function.
int main()
{
    // In order to connect the mqtt client to a broker,
    // Define an Ip address pointing to a broker. In this case, the localhost on port 1883.
    std::string ip = "tcp://localhost:1883";
    // Then, define an ID to be used by the client when communicating with the broker.
    std::string id = "publisher";

    // Construct a client using the Ip and Id, specifying usage of MQTT V5.
    mqtt::client client(ip, id, mqtt::create_options(MQTTVERSION_5));
    // Use the connect method of the client to establish a connection to the broker.
    client.connect();
    // Initialize an empty message with specified topic.
    mqtt::message_ptr timeLeftMessagePointer = mqtt::make_message("consumer/in", "");

    // Create a loop for counting down from N seconds.
    for (int i = 10; i > 0; --i)
    {
        // Sleep for one second.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // Configure Mqtt message to contain payload specifying time until end.
        timeLeftMessagePointer->set_payload("Time Left: " + std::to_string(i));
        // Publish the Mqtt message using the connected client.
        client.publish(timeLeftMessagePointer);
    }

    // After counting down, configure Mqtt message for sending the quit signal.
    timeLeftMessagePointer->set_payload("quit");
    // Send quit signal to listeners.
    client.publish(timeLeftMessagePointer);

    return 0;
}



#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>	// For sleep
#include <atomic>
#include <chrono>
#include <cstring>
#include "mqtt/async_client.h"

using namespace std;
using namespace std::chrono;

const std::string DFLT_SERVER_ADDRESS { "tcp://localhost:1883" };

// The QoS for sending data
const int QOS = 1;

// How often to sample the "data"
const auto SAMPLE_PERIOD = milliseconds(5);

// How much the "data" needs to change before we publish a new value.
const int DELTA_MS = 100;

// How many to buffer while off-line
const int MAX_BUFFERED_MESSAGES = 1200;

// --------------------------------------------------------------------------
// Gets the current time as the number of milliseconds since the epoch:
// like a time_t with ms resolution.

uint64_t timestamp()
{
    auto now = system_clock::now();
    auto tse = now.time_since_epoch();
    auto msTm = duration_cast<milliseconds>(tse);
    return uint64_t(msTm.count());
}

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // The server URI (address)
    string address = (argc > 1) ? string(argv[1]) : DFLT_SERVER_ADDRESS;

    // The amount of time to run (in ms). Zero means "run forever".
    uint64_t trun = (argc > 2) ? stoll(argv[2]) : 0LL;

    cout << "Initializing for server '" << address << "'..." << endl;

    // We configure to allow publishing to the client while off-line,
    // and that it's OK to do so before the 1st successful connection.
    auto createOpts = mqtt::create_options_builder()
                          .send_while_disconnected(true, true)
                          .max_buffered_messages(MAX_BUFFERED_MESSAGES)
                          .delete_oldest_messages()
                          .finalize();

    mqtt::async_client cli(address, "", createOpts);

    // Set callbacks for when connected and connection lost.

    cli.set_connected_handler([&cli](const std::string&) {
        std::cout << "*** Connected ("
            << timestamp() << ") ***" << std::endl;
    });

    cli.set_connection_lost_handler([&cli](const std::string&) {
        std::cout << "*** Connection Lost ("
            << timestamp() << ") ***" << std::endl;
    });

    auto willMsg = mqtt::message("test/events", "Time publisher disconnected", 1, true);
    auto connOpts = mqtt::connect_options_builder()
        .clean_session()
        .will(willMsg)
        .automatic_reconnect(seconds(1), seconds(10))
        .finalize();

    try {
        // Note that we start the connection, but don't wait for completion.
        // We configured to allow publishing before a successful connection.
        cout << "Starting connection..." << endl;
        cli.connect(connOpts);

        auto top = mqtt::topic(cli, "test1", QOS);
        cout << "Publishing data..." << endl;

        while (timestamp() % DELTA_MS != 0)
            ;

        uint64_t	t = timestamp(),
                    tlast = t,
                    tstart = t;

        top.publish(to_string(t));

        while (true) {
            this_thread::sleep_for(SAMPLE_PERIOD);

            t = timestamp();
            //cout << t << endl;
            if (abs(int(t - tlast)) >= DELTA_MS)
                top.publish(to_string(tlast = t));

            if (trun > 0 && t >= (trun + tstart))
                break;
        }

        // Disconnect
        cout << "\nDisconnecting..." << endl;
        cli.disconnect()->wait();
        cout << "  ...OK" << endl;
    }
    catch (const mqtt::exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}
#endif


#if 0
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include "mqtt/async_client.h"

using namespace std;

const string DFLT_SERVER_ADDRESS	{ "tcp://localhost:1883" };
const string CLIENT_ID				{ "paho_cpp_async_publish" };
const string PERSIST_DIR			{ "./persist" };

const string TOPIC { "test1" };

const char* PAYLOAD1 = "Hello World!";
const char* PAYLOAD2 = "Hi there!";
const char* PAYLOAD3 = "Is anyone listening?";
const char* PAYLOAD4 = "Someone is always listening.";

const char* LWT_PAYLOAD = "Last will and testament.";

const int  QOS = 1;

const auto TIMEOUT = std::chrono::seconds(10);

/////////////////////////////////////////////////////////////////////////////

/**
 * A callback class for use with the main MQTT client.
 */
class callback : public virtual mqtt::callback
{
public:
    void connection_lost(const string& cause) override {
        cout << "\nConnection lost" << endl;
        if (!cause.empty())
            cout << "\tcause: " << cause << endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr tok) override {
        cout << "\tDelivery complete for token: "
            << (tok ? tok->get_message_id() : -1) << endl;
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A base action listener.
 */
class action_listener : public virtual mqtt::iaction_listener
{
protected:
    void on_failure(const mqtt::token& tok) override {
        cout << "\tListener failure for token: "
            << tok.get_message_id() << endl;
    }

    void on_success(const mqtt::token& tok) override {
        cout << "\tListener success for token: "
            << tok.get_message_id() << endl;
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A derived action listener for publish events.
 */
class delivery_action_listener : public action_listener
{
    atomic<bool> done_;

    void on_failure(const mqtt::token& tok) override {
        action_listener::on_failure(tok);
        done_ = true;
    }

    void on_success(const mqtt::token& tok) override {
        action_listener::on_success(tok);
        done_ = true;
    }

public:
    delivery_action_listener() : done_(false) {}
    bool is_done() const { return done_; }
};

/////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    // A client that just publishes normally doesn't need a persistent
    // session or Client ID unless it's using persistence, then the local
    // library requires an ID to identify the persistence files.

    string	address  = (argc > 1) ? string(argv[1]) : DFLT_SERVER_ADDRESS,
            clientID = (argc > 2) ? string(argv[2]) : CLIENT_ID;

    cout << "Initializing for server '" << address << "'..." << endl;
    mqtt::async_client client(address, clientID, PERSIST_DIR);

    callback cb;
    client.set_callback(cb);

    auto connOpts = mqtt::connect_options_builder()
        .clean_session()
        .will(mqtt::message(TOPIC, LWT_PAYLOAD, QOS))
        .finalize();

    cout << "  ...OK" << endl;

    try {
        cout << "\nConnecting..." << endl;
        mqtt::token_ptr conntok = client.connect(connOpts);
        cout << "Waiting for the connection..." << endl;
        conntok->wait();
        cout << "  ...OK" << endl;

        // First use a message pointer.

        cout << "\nSending message..." << endl;
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, PAYLOAD1);
        pubmsg->set_qos(QOS);
        client.publish(pubmsg)->wait_for(TIMEOUT);
        cout << "  ...OK" << endl;

        // Now try with itemized publish.

        cout << "\nSending next message..." << endl;
        mqtt::delivery_token_ptr pubtok;
        pubtok = client.publish(TOPIC, PAYLOAD2, strlen(PAYLOAD2), QOS, false);
        cout << "  ...with token: " << pubtok->get_message_id() << endl;
        cout << "  ...for message with " << pubtok->get_message()->get_payload().size()
            << " bytes" << endl;
        pubtok->wait_for(TIMEOUT);
        cout << "  ...OK" << endl;

        // Now try with a listener

        cout << "\nSending next message..." << endl;
        action_listener listener;
        pubmsg = mqtt::make_message(TOPIC, PAYLOAD3);
        pubtok = client.publish(pubmsg, nullptr, listener);
        pubtok->wait();
        cout << "  ...OK" << endl;

        // Finally try with a listener, but no token

        cout << "\nSending final message..." << endl;
        delivery_action_listener deliveryListener;
        pubmsg = mqtt::make_message(TOPIC, PAYLOAD4);
        client.publish(pubmsg, nullptr, deliveryListener);

        while (!deliveryListener.is_done()) {
            this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        cout << "OK" << endl;

        // Double check that there are no pending tokens

        auto toks = client.get_pending_delivery_tokens();
        if (!toks.empty())
            cout << "Error: There are pending delivery tokens!" << endl;

        // Disconnect
        cout << "\nDisconnecting..." << endl;
        client.disconnect()->wait();
        cout << "  ...OK" << endl;
    }
    catch (const mqtt::exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}
#endif
#if 0
// Start by `#include`-ing the Mosquitto MQTT Library and other standard libraries.
#include <mqtt/client.h>  // Mosquitto client.
#include <ostream>  // std::cout.
#include <chrono>  // Time keeping.
#include <thread>  // Sleep.

// With the library header files included, continue by defining a main function.
int main()
{
    // In order to connect the mqtt client to a broker,
    // Define an Ip address pointing to a broker. In this case, the localhost on port 1883.
    std::string ip = "tcp://localhost:1883";
    // Then, define an ID to be used by the client when communicating with the broker.
    std::string id = "publisher";

    // Construct a client using the Ip and Id, specifying usage of MQTT V5.
    mqtt::client client(ip, id, mqtt::create_options(MQTTVERSION_5));
    // Use the connect method of the client to establish a connection to the broker.
    client.connect();
    // Initialize an empty message with specified topic.
    mqtt::message_ptr timeLeftMessagePointer = mqtt::make_message("consumer/in", "");

    // Create a loop for counting down from N seconds.
    for (int i = 10; i > 0; --i)
    {
        // Sleep for one second.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // Configure Mqtt message to contain payload specifying time until end.
        timeLeftMessagePointer->set_payload("Time Left: " + std::to_string(i));
        // Publish the Mqtt message using the connected client.
        client.publish(timeLeftMessagePointer);
    }

    // After counting down, configure Mqtt message for sending the quit signal.
    timeLeftMessagePointer->set_payload("quit");
    // Send quit signal to listeners.
    client.publish(timeLeftMessagePointer);

    return 0;
}



#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>	// For sleep
#include <atomic>
#include <chrono>
#include <cstring>
#include "mqtt/async_client.h"

using namespace std;
using namespace std::chrono;

const std::string DFLT_SERVER_ADDRESS { "tcp://localhost:1883" };

// The QoS for sending data
const int QOS = 1;

// How often to sample the "data"
const auto SAMPLE_PERIOD = milliseconds(5);

// How much the "data" needs to change before we publish a new value.
const int DELTA_MS = 100;

// How many to buffer while off-line
const int MAX_BUFFERED_MESSAGES = 1200;

// --------------------------------------------------------------------------
// Gets the current time as the number of milliseconds since the epoch:
// like a time_t with ms resolution.

uint64_t timestamp()
{
    auto now = system_clock::now();
    auto tse = now.time_since_epoch();
    auto msTm = duration_cast<milliseconds>(tse);
    return uint64_t(msTm.count());
}

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // The server URI (address)
    string address = (argc > 1) ? string(argv[1]) : DFLT_SERVER_ADDRESS;

    // The amount of time to run (in ms). Zero means "run forever".
    uint64_t trun = (argc > 2) ? stoll(argv[2]) : 0LL;

    cout << "Initializing for server '" << address << "'..." << endl;

    // We configure to allow publishing to the client while off-line,
    // and that it's OK to do so before the 1st successful connection.
    auto createOpts = mqtt::create_options_builder()
                          .send_while_disconnected(true, true)
                          .max_buffered_messages(MAX_BUFFERED_MESSAGES)
                          .delete_oldest_messages()
                          .finalize();

    mqtt::async_client cli(address, "", createOpts);

    // Set callbacks for when connected and connection lost.

    cli.set_connected_handler([&cli](const std::string&) {
        std::cout << "*** Connected ("
            << timestamp() << ") ***" << std::endl;
    });

    cli.set_connection_lost_handler([&cli](const std::string&) {
        std::cout << "*** Connection Lost ("
            << timestamp() << ") ***" << std::endl;
    });

    auto willMsg = mqtt::message("test/events", "Time publisher disconnected", 1, true);
    auto connOpts = mqtt::connect_options_builder()
        .clean_session()
        .will(willMsg)
        .automatic_reconnect(seconds(1), seconds(10))
        .finalize();

    try {
        // Note that we start the connection, but don't wait for completion.
        // We configured to allow publishing before a successful connection.
        cout << "Starting connection..." << endl;
        cli.connect(connOpts);

        auto top = mqtt::topic(cli, "test1", QOS);
        cout << "Publishing data..." << endl;

        while (timestamp() % DELTA_MS != 0)
            ;

        uint64_t	t = timestamp(),
                    tlast = t,
                    tstart = t;

        top.publish(to_string(t));

        while (true) {
            this_thread::sleep_for(SAMPLE_PERIOD);

            t = timestamp();
            //cout << t << endl;
            if (abs(int(t - tlast)) >= DELTA_MS)
                top.publish(to_string(tlast = t));

            if (trun > 0 && t >= (trun + tstart))
                break;
        }

        // Disconnect
        cout << "\nDisconnecting..." << endl;
        cli.disconnect()->wait();
        cout << "  ...OK" << endl;
    }
    catch (const mqtt::exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}
#endif


#if 0
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include "mqtt/async_client.h"

using namespace std;

const string DFLT_SERVER_ADDRESS	{ "tcp://localhost:1883" };
const string CLIENT_ID				{ "paho_cpp_async_publish" };
const string PERSIST_DIR			{ "./persist" };

const string TOPIC { "test1" };

const char* PAYLOAD1 = "Hello World!";
const char* PAYLOAD2 = "Hi there!";
const char* PAYLOAD3 = "Is anyone listening?";
const char* PAYLOAD4 = "Someone is always listening.";

const char* LWT_PAYLOAD = "Last will and testament.";

const int  QOS = 1;

const auto TIMEOUT = std::chrono::seconds(10);

/////////////////////////////////////////////////////////////////////////////

/**
 * A callback class for use with the main MQTT client.
 */
class callback : public virtual mqtt::callback
{
public:
    void connection_lost(const string& cause) override {
        cout << "\nConnection lost" << endl;
        if (!cause.empty())
            cout << "\tcause: " << cause << endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr tok) override {
        cout << "\tDelivery complete for token: "
            << (tok ? tok->get_message_id() : -1) << endl;
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A base action listener.
 */
class action_listener : public virtual mqtt::iaction_listener
{
protected:
    void on_failure(const mqtt::token& tok) override {
        cout << "\tListener failure for token: "
            << tok.get_message_id() << endl;
    }

    void on_success(const mqtt::token& tok) override {
        cout << "\tListener success for token: "
            << tok.get_message_id() << endl;
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A derived action listener for publish events.
 */
class delivery_action_listener : public action_listener
{
    atomic<bool> done_;

    void on_failure(const mqtt::token& tok) override {
        action_listener::on_failure(tok);
        done_ = true;
    }

    void on_success(const mqtt::token& tok) override {
        action_listener::on_success(tok);
        done_ = true;
    }

public:
    delivery_action_listener() : done_(false) {}
    bool is_done() const { return done_; }
};

/////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    // A client that just publishes normally doesn't need a persistent
    // session or Client ID unless it's using persistence, then the local
    // library requires an ID to identify the persistence files.

    string	address  = (argc > 1) ? string(argv[1]) : DFLT_SERVER_ADDRESS,
            clientID = (argc > 2) ? string(argv[2]) : CLIENT_ID;

    cout << "Initializing for server '" << address << "'..." << endl;
    mqtt::async_client client(address, clientID, PERSIST_DIR);

    callback cb;
    client.set_callback(cb);

    auto connOpts = mqtt::connect_options_builder()
        .clean_session()
        .will(mqtt::message(TOPIC, LWT_PAYLOAD, QOS))
        .finalize();

    cout << "  ...OK" << endl;

    try {
        cout << "\nConnecting..." << endl;
        mqtt::token_ptr conntok = client.connect(connOpts);
        cout << "Waiting for the connection..." << endl;
        conntok->wait();
        cout << "  ...OK" << endl;

        // First use a message pointer.

        cout << "\nSending message..." << endl;
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, PAYLOAD1);
        pubmsg->set_qos(QOS);
        client.publish(pubmsg)->wait_for(TIMEOUT);
        cout << "  ...OK" << endl;

        // Now try with itemized publish.

        cout << "\nSending next message..." << endl;
        mqtt::delivery_token_ptr pubtok;
        pubtok = client.publish(TOPIC, PAYLOAD2, strlen(PAYLOAD2), QOS, false);
        cout << "  ...with token: " << pubtok->get_message_id() << endl;
        cout << "  ...for message with " << pubtok->get_message()->get_payload().size()
            << " bytes" << endl;
        pubtok->wait_for(TIMEOUT);
        cout << "  ...OK" << endl;

        // Now try with a listener

        cout << "\nSending next message..." << endl;
        action_listener listener;
        pubmsg = mqtt::make_message(TOPIC, PAYLOAD3);
        pubtok = client.publish(pubmsg, nullptr, listener);
        pubtok->wait();
        cout << "  ...OK" << endl;

        // Finally try with a listener, but no token

        cout << "\nSending final message..." << endl;
        delivery_action_listener deliveryListener;
        pubmsg = mqtt::make_message(TOPIC, PAYLOAD4);
        client.publish(pubmsg, nullptr, deliveryListener);

        while (!deliveryListener.is_done()) {
            this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        cout << "OK" << endl;

        // Double check that there are no pending tokens

        auto toks = client.get_pending_delivery_tokens();
        if (!toks.empty())
            cout << "Error: There are pending delivery tokens!" << endl;

        // Disconnect
        cout << "\nDisconnecting..." << endl;
        client.disconnect()->wait();
        cout << "  ...OK" << endl;
    }
    catch (const mqtt::exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}
#endif
