#pragma once


class error;
class Client;
class Sender
{
	Client* cl;
	error* errorMask;

public:
	
	SOCKET  _socket;

	void send_login_ok_packet(SOCKET s, int c_id, short x, short y, short level, short hp, int exp);

	void send_move_packet(SOCKET s, int c_id, int mover, short x, short y, int last_move_time, char dir);
	void send_remove_object(SOCKET s, int c_id, int victim, bool isDie);
	void send_attack_effect(SOCKET s, int target, int c_id);
	void send_put_object(SOCKET s, int c_id, int target, short x, short y, char* name);
	void send_status_change(SOCKET s, int target, int new_id, short hp, short maxhp, short level, int exp);
	void send_chat_packet(SOCKET s, int user_id, int my_id, char* mess);
	void send_login_fail_packet(SOCKET s, int c_id);
	void send_teleport_packet(SOCKET s, int x, int y);
	void send_put_object(SOCKET s, int c_id, int target,
		short x, short y, char* name, char npcCharactherType, char npcMoveType);
	void do_recv();

	void do_send(SOCKET s, void* mess);

};

