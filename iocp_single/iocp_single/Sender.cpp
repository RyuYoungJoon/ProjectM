#include "pch.h"
#include "Sender.h"

void Sender::send_login_ok_packet(SOCKET s, int c_id,short x, short y, short level, short hp, int exp)
{
	sc_packet_login_ok packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.x = x;
	packet.y = y;
	packet.level = level;
	packet.hp = hp;
	packet.maxhp = 100;
	packet.exp = exp;
	do_send(s, &packet);
}
//	int ret = WSARecv(_socket, &_recv_over->_wsa_buf, 1, 0, &recv_flag, &_recv_over->_wsa_over, NULL);

void Sender::send_move_packet(SOCKET s, int c_id, int mover,short x, short y, int last_move_time, char dir)
{
	sc_packet_move packet;
	packet.id = mover;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MOVE;
	packet.x = x;
	packet.y = y;
	packet.move_time = last_move_time;
	packet.direction = dir;
	do_send(s, &packet);
}
void Sender::send_remove_object(SOCKET s, int c_id, int victim, bool isDie)
{
	sc_packet_remove_object packet;
	packet.id = victim;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_OBJECT;
	packet.isDie = isDie;
	do_send(s, &packet);

}
void Sender::send_attack_effect(SOCKET s, int target, int c_id)
{
	sc_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ATTACK;
	packet.id = c_id;

	do_send(s, &packet);

}
void Sender::send_put_object(SOCKET s, int c_id, int target,short x, short y, char* name)
{
	sc_packet_put_object packet;
	packet.id = target;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PUT_OBJECT;
	packet.x = x;
	packet.y = y;
	strcpy_s(packet.name, name);
	packet.object_type = 0;
	do_send(s, &packet);

}
void Sender::send_put_object(SOCKET s, int c_id, int target, 
	short x, short y, char* name,char npcCharactherType,char npcMoveType)
{
	sc_packet_put_object packet;
	packet.id = target;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PUT_OBJECT;
	packet.x = x;
	packet.y = y;
	packet.npcCharacterType = npcCharactherType;
	packet.npcMoveType = npcMoveType;
	strcpy_s(packet.name, name);
	packet.object_type = 0;
	do_send(s, &packet);

}
void Sender::send_status_change(SOCKET s, int target, int new_id,short hp, short maxhp, short level, int exp)
{
	sc_packet_status_change packet;
	packet.id = new_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_STATUS_CHANGE;
	packet.hp = hp;
	packet.maxhp = maxhp;
	packet.level = level;
	packet.exp = exp;

	do_send(s, &packet);

}
void Sender::send_chat_packet(SOCKET s, int user_id, int my_id, char* mess)
{
	sc_packet_chat packet;
	packet.id = my_id;
	packet.chatType = 1;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHAT;
	strcpy_s(packet.mess, mess);
	do_send(s, &packet);

}

void Sender::send_login_fail_packet(SOCKET s, int c_id)
{
	sc_packet_login_fail packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_FAIL;

	do_send(s, &packet);


}
void Sender::send_teleport_packet(SOCKET s, int x, int y)
{
	sc_packet_teleport packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TELEPORT;
	packet.x = x;
	packet.y = y;

	do_send(s, &packet);
}
//void Sender::do_recv()
//{
//	DWORD recv_flag = 0;
//	ZeroMemory(&_recv_over->_wsa_over, sizeof(_recv_over->_wsa_over));
//	_recv_over->_wsa_buf.buf = reinterpret_cast<char*>(_recv_over->_net_buf + _prev_size);
//	_recv_over->_wsa_buf.len = sizeof(_recv_over->_net_buf) - _prev_size;
//	int ret = WSARecv(_socket, &_recv_over->_wsa_buf, 1, 0, &recv_flag, &_recv_over->_wsa_over, NULL);
//	if (SOCKET_ERROR == ret) {
//		//cout << "do_recv error" << endl;
//		int error_num = WSAGetLastError();
//		if (ERROR_IO_PENDING != error_num)
//			errorMask->error_display(error_num);
//	}
//}

void Sender::do_send(SOCKET s, void* mess)
{
	//Exp_Over* ex_over = new Exp_Over(COMP_OP::OP_SEND, num_bytes, mess);
	char* packet = reinterpret_cast<char*>(mess);
	
	int packet_size = packet[0];
	Exp_Over* ex_over = new Exp_Over;
	ZeroMemory(&ex_over->_wsa_over, sizeof(Exp_Over));
	ex_over->_comp_op = COMP_OP::OP_SEND;
	memcpy(ex_over->_net_buf, packet, packet_size);
	ex_over->_wsa_buf.buf = ex_over->_net_buf;
	ex_over->_wsa_buf.len = packet_size;

	int ret = WSASend(s, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
	if (SOCKET_ERROR == ret) {
		//cout << "do_send error" << endl;
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
			errorMask->error_display(error_num);
	}
}