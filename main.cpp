#include <iostream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <cstring>
#include <mutex>

#include "httpserver.hpp"


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
//redis post code
         }
         catch(const runtime_error& exc)
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
//        str.replace(5,3," ");
        cout << "GET request with key parameter: " << fruit << endl;
        try
        {
//redis get code
            if (true)
            {
//                cout << msg->get_topic() <<  msg->to_string() << endl;
                cout << "...Ok" << endl;
                return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("msg->to_string()"));
            }
            else
                return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("zero message received"));
        }
        catch(const runtime_error& exc)
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

    const std::shared_ptr<httpserver::http_response> render_DELETE(const httpserver::http_request& req)
    {
        std::lock_guard<std::mutex> lck (mtx);
        string fruit = req.get_arg("fruit");
//        str.replace(5,3," ");
        cout << "DELETE request with parameter: " << fruit << endl;
        try
        {
//redis code
        }
        catch(const runtime_error& exc)
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

    void set_db()
    {

    }

private:
     std::mutex mtx;
};

int main() {
    httpserver::webserver ws = httpserver::create_webserver(5880);

    better_store hwr;
    ws.register_resource("/save/", &hwr);
    ws.register_resource("/show/", &hwr);
    ws.register_resource("/del/", &hwr);
    hwr.set_db();

    ws.start(true);

    return 0;
}
