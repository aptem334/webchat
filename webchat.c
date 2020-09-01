#include <iostream>
#include <uwebsockets/App.h>
#include <thread>
#include <algorithm>

using namespace std;

struct UserConnection {
	unsigned long user_id;
	string* user_name;
};

int main()
{
	atomic_ulong latest_user_id = 10;
	vector<thread*> threads(thread::hardware_concurrency());
	transform(threads.begin(), threads.end(), threads.begin(), [&latest_user_id](auto* thr) {
		return new thread([&latest_user_id]() {		
			uWS::App().ws<UserConnection>("/*", {
				.open = [&latest_user_id](auto* ws) {
					UserConnection* data = (UserConnection*)ws->getUserData();
					data->user_id = latest_user_id++;
					cout << "New user connectied, id " << data->user_id << endl;
					ws->subscribe("broadcast");
					ws->subscribe("user#" + to_string(data->user_id));
					cout << "Total users connected: " << latest_user_id - 10 << endl;
				},
				.message = [](auto* ws, string_view message, uWS::OpCode opCode) {
					UserConnection* data = (UserConnection*)ws->getUserData();
					cout << "New message " << message << " User ID " << data->user_id << endl;
					auto beginning = message.substr(0, 9);
					if (beginning.compare("SET_NAME=") == 0) {
						auto valid = message.substr(9);
						int position = valid.find(",");
						int lenght = valid.length();
						if (position != -1 || lenght > 255) {
							cout << "Username not valid " << endl;
							}
						else {
							auto name = message.substr(9);
							data->user_name = new string(name);
							cout << "User set their name ID = " << data->user_id << " Name = " << (*data->user_name) << endl;
							string broadcast_message = "NEW_USER," + (*data->user_name) + "," + to_string(data->user_id);
							ws->publish("broadcast", string_view(broadcast_message), opCode, false);
						}
					}
					auto is_message_to = message.substr(0, 11);
					if (is_message_to.compare("MESSAGE_TO=") == 0) {
						auto rest = message.substr(11);
						int position = rest.find(",");
						if (position != -1) {
							auto id_string = rest.substr(0, position);
							auto user_message = rest.substr(position + 1);
							ws->publish("user#" + string(id_string), user_message, opCode, false);
						}
					}
				}
			}).listen(9999,
				[](auto* token) {
					if (token) {
						cout << "Server started and listening on port 9999 " << endl;
					}
					else {
						cout << "Server failed to start :( " << endl;
					}
				}).run();
		});
	});
	for_each(threads.begin(), threads.end(), [](auto* thr) {thr->join(); });
}

