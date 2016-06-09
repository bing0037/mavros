/**
 * Test mavconn library
 *
 * This test created for issue #72.
 * It is hand test, no build rules.
 *
 * Compile command:
 *     g++ -I/opt/ros/kinetic/include -I$HOME/ros/install/include -Iinclude \
 *     -std=c++11 -ggdb test/test_hang.cpp -o /tmp/hang \
 *     -L/opt/ros/kinetic/lib -L$HOME/ros/devel/lib -lroscpp -lrosconsole -lboost_system -lmavconn -lrt \
 *     -DMAVLINK_DIALECT=ardupilotmega
 */

#include <ros/ros.h>

#include <mavconn/interface.h>

using namespace mavconn;
using namespace mavlink;

static void send_heartbeat(MAVConnInterface *ip) {
#if 0
	mavlink_message_t msg;
	mavlink_msg_heartbeat_pack_chan(ip->get_system_id(), ip->get_component_id(), ip->get_channel(), &msg,
		MAV_TYPE_ONBOARD_CONTROLLER,
		MAV_AUTOPILOT_INVALID,
		MAV_MODE_MANUAL_ARMED,
		0,
		MAV_STATE_ACTIVE);
	ip->send_message(&msg);
#endif

	using mavlink::common::MAV_TYPE;
	using mavlink::common::MAV_AUTOPILOT;
	using mavlink::common::MAV_MODE;
	using mavlink::common::MAV_STATE;

	mavlink::common::msg::HEARTBEAT hb{};
	hb.type = int(MAV_TYPE::ONBOARD_CONTROLLER);
	hb.autopilot = int(MAV_AUTOPILOT::INVALID);
	hb.base_mode = int(MAV_MODE::MANUAL_ARMED);
	hb.custom_mode = 0;
	hb.system_status = int(MAV_STATE::ACTIVE);

	ip->send_message(hb);
}

static void send_sys_status(MAVConnInterface *ip) {
	mavlink_message_t msg{};
	mavlink::MsgMap map(msg);

	mavlink::common::msg::SYS_STATUS st{};
	st.load = 100;

	st.serialize(map);
	mavlink::mavlink_finalize_message(&msg, ip->get_system_id(), ip->get_component_id(), st.MIN_LENGTH, st.LENGTH, st.CRC_EXTRA);

	const mavlink::mavlink_msg_entry_t *e = mavlink::mavlink_get_msg_entry(msg.msgid);
	std::cout << "SS e: " << e << std::endl;

	std::cout << "SS: " << msg.msgid << std::endl;

	ip->send_message(&msg);
}

int main(int argc, char **argv){
	ros::init(argc, argv, "mavconn_test", ros::init_options::AnonymousName);

	MAVConnInterface::Ptr server;
	MAVConnInterface::Ptr client;

	// create echo server
	server = MAVConnInterface::open_url("udp://:45000@localhost:45001", 42, 200);
	//server->message_received += [&](const mavlink_message_t *msg, uint8_t sysid, uint8_t compid) { server->send_message(msg); };
	//server->message_received.connect([&](const mavlink_message_t *msg) { server->send_message(msg); });
	server->message_received.connect([&](const mavlink_message_t *msg) {
			std::cout << "S:RECV: " << msg->msgid << std::endl;
			//send_sys_status(server.get());
			send_sys_status(client.get());
			});

	// create client
	client = MAVConnInterface::open_url("udp://:45001@localhost:45000", 44, 200);
	//client->message_received += recv_message;
	client->message_received.connect([&](const mavlink_message_t *msg) { std::cout << "C:RECV: " << msg->msgid << std::endl; });

	while (ros::ok()) {
		send_heartbeat(client.get());
		//send_sys_status(server.get());
	}
	return 0;
}
