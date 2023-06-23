#include "timedRead.h"
#include <QDebug>

timedRead::timedRead(std::string url, std::string device, int mode, const std::shared_ptr<baseReroute>& bp)
{
	//gInstance = instance;
	gInstance = static_cast<dsiiReroute*>(bp.get());
	vMode = mode;
	devTag = device;
	// create a timer
	timer = new QTimer(this);

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
			}
			if (msg->type == ix::WebSocketMessageType::Open) {
			}
		}
	);

	webSocket.start();

	// setup signal and slot
	connect(timer, SIGNAL(timeout()),
		this, SLOT(timerSlot()));

	// msec
	timer->start(100);
}

timedRead::~timedRead() {
	delete timer;
}

void timedRead::timerSlot()
{
	//webSocket.send(devTag+":0.5");
	static unsigned long int millisTimer = 0;
	const int maxTime = 30;
	static int vibDuration = 50;
	gInstance->getStats();
	if (vMode == 0 && gInstance->dmgRecStr > 0) {
		millisTimer = 0;
		vibDuration = (double)maxTime * gInstance->dmgRecStr;
		std::string str = std::to_string(gInstance->dmgRecStr);
		if (!webSocket.send(devTag + ":" + str).success) {
			QMessageBox msgBox;
			msgBox.setText("Something is wrong with socket connection. Check port and restart.");
			int ret = msgBox.exec();
			timer->stop();
		}
	}

	if (vMode == 1 && gInstance->dmgDealtStr > 0) {
		millisTimer = 0;
		vibDuration = maxTime * gInstance->dmgDealtStr;
		std::string str = std::to_string(gInstance->dmgDealtStr);
		if (!webSocket.send(devTag + ":" + str).success) {
			QMessageBox msgBox;
			msgBox.setText("Something is wrong with socket connection. Check port and restart.");
			int ret = msgBox.exec();
			timer->stop();
		}
	}
	else if (millisTimer > vibDuration) {
		millisTimer = 0;
		if (!webSocket.send(devTag + ":0").success) {
			QMessageBox msgBox;
			msgBox.setText("Something is wrong with socket connection. Check port and restart.");
			int ret = msgBox.exec();
			timer->stop();
		}
	}
	millisTimer++;
}