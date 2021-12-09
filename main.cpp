#include <iostream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <cstring>
#include <mutex>

#include "httpserver.hpp"

#include <sw/redis++/redis++.h>

using namespace sw::redis;

using namespace std;


class better_store : public httpserver::http_resource
{
public:
     const std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req)
     {
         std::lock_guard<std::mutex> lck(mtx);
         string fruit = req.get_arg("fruit");
         string color = req.get_arg("color");
         cout << "POST request with parameters: " << fruit <<", " << color << endl;
         try
         {
             auto val = *redis->command<OptionalStringPair>("time");
             redis->set(fruit,color);
             redis->set(fruit +":time",val.first);
         }
         catch(const Error& exc)
         {
             cerr << exc.what() << endl;
             return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(exc.what()));
         }
         catch(...)
         {
             cerr << "unknown exception\n" << endl;
             return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("unknown exception\n"));
         }
         cout << "...Ok" << endl;

         return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("success!"));
     }

    const std::shared_ptr<httpserver::http_response> render_GET(const httpserver::http_request& req)
    {
        std::lock_guard<std::mutex> lck(mtx);
        string fruit = req.get_arg("fruit");
        string color;
        string timestamp;
        string response;
        cout << "GET request with key parameter: " << fruit << endl;
        try
        {
            auto tmp_p = redis->get(fruit);
            if(!tmp_p)
            {
                response = string("None");
            }

            else
            {
                string tmp_str = *tmp_p;
                if(tmp_str.empty())
                {
                    response = string("None");
                }
                else
                {
                timestamp = *redis->get(fruit + string(":time"));
                response = string("color = ") + tmp_str + string(" at: ") + timestamp;
                }
            }
        }
        catch(const Error& e)
        {
            cerr << e.what() << endl;
            return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(e.what()));
        }
        catch(...)
        {
            cerr << "unknown exception\n" << endl;
            return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("unknown exception\n"));
        }
        cout << "...Ok" << endl;
        shared_ptr<httpserver::string_response> rsp = make_shared<httpserver::string_response>(response);
        return rsp;

    }

    const std::shared_ptr<httpserver::http_response> render_DELETE(const httpserver::http_request& req)
    {
        std::lock_guard<std::mutex> lck (mtx);
        string fruit = req.get_arg("fruit");
        cout << "DELETE request with parameter: " << fruit << endl;
        try
        {
            redis->del(fruit);
            redis->del(fruit + string(":time"));
        }
        catch(const Error& e)
        {
            cerr << e.what() << endl;
            return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(e.what()));
        }
        catch(...)
        {
            cerr << "unknown exception\n" << endl;
            return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("unknown exception\n"));
        }
        cout << "...Ok" << endl;
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("success!"));
    }

    bool set_db()
    {
        try
        {
            // Create an Redis object, which is movable but NOT copyable.
            Redis redis = Redis("tcp://127.0.0.1:6379");
        }
        catch (const Error &e)
        {
            cerr << e.what() << endl;
            return false;
        }
        catch(...)
        {
            cerr << "unknown exception in database connection\n" << endl;
            return false;
        }
        return true;
    }

    void set_db_1(shared_ptr<Redis> r)
    {
        redis = move(r);
    }

private:
    std::mutex mtx;
    shared_ptr<Redis> redis;
};

int main() {
    httpserver::webserver ws = httpserver::create_webserver(5880);

    shared_ptr<Redis> redis;
    try
    {
        redis = make_shared<Redis>("tcp://127.0.0.1:6379");
    }
    catch (const Error &e)
    {
        cerr << e.what() << endl;
        return -1;
    }
    catch(...)
    {
        cerr << "unknown exception in database connection\n" << endl;
        return -1;
    }

    better_store hwr;
    ws.register_resource("/save/", &hwr);
    ws.register_resource("/show/", &hwr);
    ws.register_resource("/del/", &hwr);
    if (!hwr.set_db()) return -1;
    hwr.set_db_1(redis);

    ws.start(true);

    return 0;
}
