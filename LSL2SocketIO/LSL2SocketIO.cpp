#include "socket.io\sio_client.h"
#include "lsl_cpp.h"
#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#ifdef WIN32
#define HIGHLIGHT(__O__) std::cout<<__O__<<std::endl
#define EM(__O__) std::cout<<__O__<<std::endl

#include <stdio.h>
#include <tchar.h>
const char *channels[] = { "C3","C4","Cz","FPz","POz","CPz","O1","O2" };
#define MAIN_FUNC int _tmain(int argc, _TCHAR* argv[])
#else
#define HIGHLIGHT(__O__) std::cout<<"\e[1;31m"<<__O__<<"\e[0m"<<std::endl
#define EM(__O__) std::cout<<"\e[1;30;1m"<<__O__<<"\e[0m"<<std::endl

#define MAIN_FUNC int main(int argc ,const char* args[])
#endif

using namespace sio;
using namespace std;
std::mutex _lock;

std::condition_variable_any _cond;
lsl::stream_info info("MyEventStream", "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, "myuniquesourceid23443");
lsl::stream_info inletInfo("CommandStream", "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, "myuniquesourceid23442");
// make a new outlet
lsl::stream_outlet outlet(info);
lsl::stream_inlet inlet(inletInfo);
bool connect_finish = false;

class connection_listener
{
	sio::client &handler;

public:

	connection_listener(sio::client& h) :
		handler(h)
	{
	}


	void on_connected()
	{
		_lock.lock();
		_cond.notify_all();
		connect_finish = true;
		cout << "[Info] Connected to server" << endl;
		_lock.unlock();
	}
	void on_close(client::close_reason const& reason)
	{
		std::cout << "[Info] Socket closed" << std::endl;
		exit(0);
	}

	void on_fail()
	{
		std::cout << "[Error] Connection to server failed" << std::endl;
		exit(0);
	}
};


int participants = -1;

socket::ptr current_socket;
void bind_events(socket::ptr &socket)
{
	current_socket->on("EEG", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		string message = data->get_string();
		//int testm = atoi(message.c_str());
		int startCode = 700;
		outlet.push_sample(&startCode);
		cout << "pushed:" << message << endl;
		_lock.unlock();
		//_lock.lock();
		//lsl::stream_info info2("TEST_EEG", "EEG", 8, 100, lsl::cf_float32, string(name) += "EEG");

		//// add some description fields
		//info2.desc().append_child_value("manufacturer", "BioSemi");
		//lsl::xml_element chns = info.desc().append_child("channels");
		//for (int k = 0; k<8; k++)
		//	chns.append_child("channel")
		//	.append_child_value("label", channels[k])
		//	.append_child_value("unit", "microvolts")
		//	.append_child_value("type", "EEG");

		//// make a new outlet
		//lsl::stream_outlet outlet2(info2);

		//// send data forever
		//cout << "Now sending data... " << endl;
		//double starttime = ((double)clock()) / CLOCKS_PER_SEC;
		//for (unsigned t = 0;t<1000; t++) {

		//	// wait a bit and create random data
		//	while (((double)clock()) / CLOCKS_PER_SEC < starttime + t*0.01);
		//	float sample[8];
		//	for (int c = 0; c<8; c++)
		//		sample[c] = (float)((rand() % 1500) / 500.0 - 1.5);

		//	// send the sample
		//	outlet2.push_sample(sample);
		//	outlet.push_sample(&testm);
		//}
		//_lock.unlock();
		//_lock.lock();
		//string user = data->get_map()["username"]->get_string();
		//string message = data->get_map()["message"]->get_string();
		//EM(user << ":" << message);
		//_lock.unlock();
	}));

	current_socket->on("user joined", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		participants = data->get_map()["numUsers"]->get_int();
		bool plural = participants != 1;

		//     abc "
		HIGHLIGHT(user << " joined" << "\nthere" << (plural ? " are " : "'s ") << participants << (plural ? " participants" : " participant"));
		_lock.unlock();
	}));
	current_socket->on("user left", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		participants = data->get_map()["numUsers"]->get_int();
		bool plural = participants != 1;
		HIGHLIGHT(user << " left" << "\nthere" << (plural ? " are " : "'s ") << participants << (plural ? " participants" : " participant"));
		_lock.unlock();
	}));
}

void listen()
{
	while (true) {
		vector<string> event;
		inlet.pull_sample(event);
		for (int i = 0; i < event.size(); i++) {
			cout << "event = " << event[i] << endl;
			current_socket->emit("new message", event[i]);
		}
	}
}

MAIN_FUNC
{

	sio::client h;
connection_listener l(h);

h.set_open_listener(std::bind(&connection_listener::on_connected, &l));
h.set_close_listener(std::bind(&connection_listener::on_close, &l,std::placeholders::_1));
h.set_fail_listener(std::bind(&connection_listener::on_fail, &l));
h.connect("http://127.0.0.1:3000");
_lock.lock();
if (!connect_finish)
{
	_cond.wait(_lock);
}
_lock.unlock();
current_socket = h.socket();
cout << "[Info] LSL Outlet initialized" << endl;
int s = 1;
outlet.push_sample(&s);
thread inlet_listen_thread(listen);
//Login:
//string nickname;
//while (nickname.length() == 0) {
//	HIGHLIGHT("Type your nickname:");
//
//	getline(cin, nickname);
//}
//current_socket->on("login", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck,message::list &ack_resp) {
//	//_lock.lock();
//	//participants = data->get_map()["numUsers"]->get_int();
//	//bool plural = participants != 1;
//	//HIGHLIGHT("Welcome to Socket.IO Chat-\nthere" << (plural ? " are " : "'s ") << participants << (plural ? " participants" : " participant"));
//	//_cond.notify_all();
//	//_lock.unlock();
//	//current_socket->off("login");
//	_cond.notify_all();
//}));
//current_socket->emit("add user", nickname);
//_lock.lock();
//if (participants<0) {
//_cond.wait(_lock);
//}
//_lock.unlock();
bind_events(current_socket);

HIGHLIGHT("commands:\n'exit' : exit application\n#[number] : send custom event code");
for (std::string line; std::getline(std::cin, line);) {
	if (line.length() > 0)
	{
		if (line == "exit")
		{
			break;
		}
		else if (line[0] == '#')
		{
			string code = line.substr(1, line.length() - 1);

			string::const_iterator it = code.begin();
			while (it != code.end() && isdigit(*it)) ++it;
			if (!code.empty() && it == code.end()) {
				int a = std::stoi(code);
				outlet.push_sample(&code);
				cout << "[Info] Sent event: " << a << endl;
			}
			else {
				cout << "[Error] Numeric value expected" << endl;
			}
		}
		else {
			cout << "[Error] Unrecognized command" << endl;
		}
		//else if (line.length() > 5 && line.substr(0,5) == "$nsp ")
		//{
		//	string new_nsp = line.substr(5);
		//	if (new_nsp == current_socket->get_namespace())
		//	{
		//		continue;
		//	}
		//	current_socket->off_all();
		//	current_socket->off_error();
		//	//per socket.io, default nsp should never been closed.
		//	if (current_socket->get_namespace() != "/")
		//	{
		//		current_socket->close();
		//	}
		//	current_socket = h.socket(new_nsp);
		//	bind_events(current_socket);
		//	//if change to default nsp, we do not need to login again (since it is not closed).
		//	if (current_socket->get_namespace() == "/")
		//	{
		//		continue;
		//	}
		//	goto Login;
		//}
		//current_socket->emit("new message", line);
		//_lock.lock();
		//EM("\t\t\t" << line << ":" << "You");
		//_lock.unlock();
	}
}
inlet.close_stream();
HIGHLIGHT("Closing...");
h.sync_close();
h.clear_con_listeners();
return 0;
}
