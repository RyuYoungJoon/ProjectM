#pragma once


enum EVENT_TYPE { EVENT_NPC_MOVE, EVENT_PLAYER_MOVE, EVENT_MOVE_COUNT, EVENT_ATTACK, EVENT_REVIVE, EVENT_HEAL, EVENT_NPC_ATTACK };
enum MONSTER_TPYE { LEVEL1 = 1, LEVEL2 = 3, LEVEL3 = 5, LEVEL4 = 7, NEUTRAL = 0 };

enum class COMP_OP {
	OP_RECV,
	OP_SEND, 
	OP_ACCEPT,
	OP_NPC_MOVE,
	OP_PLAYER_MOVE,
	OP_MOVE_COUNT,
	OP_ATTACK,
	OP_REVIVE,
	OP_HEAL 
};


enum class STATUS {
	ST_FREE,
	ST_ACCEPT,
	ST_INGAME
};

struct Exp_Over
{
	WSAOVERLAPPED	_wsa_over;
	COMP_OP		_comp_op;
	WSABUF			_wsa_buf;
	char			_net_buf[256];
	int				_target;
	union {
		WSABUF wsabuf;
		SOCKET c_sock;
	};

//public:
//	Exp_Over(COMP_OP comp_op, char num_bytes, void* mess);
//
//	Exp_Over(COMP_OP comp_op);
//
//	Exp_Over();
//
//	~Exp_Over();

};