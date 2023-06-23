// dsfun.cpp : Defines the entry point for the application.
//

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <IXWebSocket.h>
#include <IXNetSystem.h>
#include <iostream>
#include "mainwindow.h"

ix::WebSocket webSocket;

void send();

int main(int argc, char* argv[])
{
    ix::initNetSystem();

    std::string url("ws://127.0.0.1:3031/haptic");
    webSocket.setUrl(url);

    // Optional heart beat, sent every 45 seconds when there is not any traffic
// to make sure that load balancers do not kill an idle connection.
    webSocket.setPingInterval(45);

    // Per message deflate connection is enabled by default. You can tweak its parameters or disable it
    webSocket.disablePerMessageDeflate();

    // Setup a callback to be fired when a message or an event (open, close, error) is received
    webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                std::cout << msg->str << std::endl;
            }
            if (msg -> type == ix::WebSocketMessageType::Open) {
                
            }
        }
    );

    //webSocket.start();

    //std::this_thread::sleep_for(std::chrono::milliseconds(600));
    //ix::WebSocketSendInfo debug = webSocket.send("mot2:0.5");
    //std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    //webSocket.stop();
    //Option 1, like in the mentioned stackoverflow answer
    //QWidget window;

    //Option 2, to test If maybe the QWidget above was the problem
    QApplication app(argc, argv);

    MainWindow main_window;
    main_window.show();

    return app.exec();
}

void send() {
    //webSocket.stop();
}
