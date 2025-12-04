
#include "pch.h"
#include "Protocol.h"

HANDLE g_h_iocp;
SOCKET g_s_socket;

//concurrency::concurrent_priority_queue <timer_event> timer_queue;
priority_queue<timer_event> timer_queue;

void do_npc_move(int npc_id);

error errorMask;
Sender sendPacket;
Client clients;
DB		userDB;
//Exp_Over expOver;
array<Client, MAX_USER + MAX_NPC> Clients;
int acceptPlayer = 0;

MAP g_Map[WORLD_HEIGHT][WORLD_WIDTH];

mutex timer_lock;


void add_timer(int obj_id, EVENT_TYPE ev_type, system_clock::time_point t)
{
	timer_event ev{ obj_id, t, ev_type };
	timer_lock.lock();
	timer_queue.emplace(ev);
	timer_lock.unlock();
}


bool is_near(int a, int b)
{
	if (RANGE < abs(Clients[a].x - Clients[b].x)) return false;
	if (RANGE < abs(Clients[a].y - Clients[b].y)) return false;
	return true;
}

bool is_npc(int id)
{
	return (id >= NPC_ID_START) && (id <= NPC_ID_END);
}

bool is_player(int id)
{
	return (id >= 0) && (id < MAX_USER);
}

int get_new_id()
{
	static int g_id = 0;

	for (int i = 0; i < MAX_USER; ++i) {
		Clients[i].state_lock.lock();
		if (STATUS::ST_FREE == Clients[i]._state) {
			Clients[i]._state = STATUS::ST_ACCEPT;
			Clients[i].state_lock.unlock();
			return i;
		}
		Clients[i].state_lock.unlock();
	}
	cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

bool CAS(volatile atomic_bool* addr, bool expected, bool new_val)
{
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_bool*>(addr), &expected, new_val);
}

void wake_up_npc(int id)
{
	if (false == Clients[id]._is_active)
	{
		//Clients[id].is_active = true;      // CAS로 구현해서 이중 활성화를 막아야 한다.
		if (true == CAS(&Clients[id]._is_active, false, true))
		{
			add_timer(id, EVENT_NPC_MOVE, system_clock::now() + 1s);
		}
	}
}

void Disconnect(int c_id)
{
	Client& cl = Clients[c_id];
	cl.vl.lock();
	unordered_set <int> my_vl = cl.viewlist;
	cl.vl.unlock();
	for (auto& other : my_vl) {
		Client& target = Clients[other];
		if (true == is_npc(target._id)) continue;
		if (STATUS::ST_INGAME != target._state)
			continue;
		target.vl.lock();
		if (0 != target.viewlist.count(c_id)) {
			target.viewlist.erase(c_id);
			target.vl.unlock();
			sendPacket.send_remove_object(cl._socket, other, c_id, false);
		}
		else target.vl.unlock();
	}
	Clients[c_id].state_lock.lock();
	closesocket(Clients[c_id]._socket);
	
	Clients[c_id]._state = STATUS::ST_FREE;
	Clients[c_id].state_lock.unlock();
}

void Activate_Player_Move_Event(int target, int player_id)
{
	cout << "Activate_Player_Move_Event" << endl;
	Exp_Over* exp_over = new Exp_Over;
	exp_over->_comp_op = COMP_OP::OP_PLAYER_MOVE;
	exp_over->_target = player_id;
	PostQueuedCompletionStatus(g_h_iocp, 1, target, &exp_over->_wsa_over);
}

bool is_attack(int id, int target)
{
	if (Clients[id].x == Clients[target].x) {
		if (abs(Clients[id].y - Clients[target].y) <= 1)
			return true;
	}
	else if (Clients[id].y == Clients[target].y) {
		if (abs(Clients[id].x - Clients[target].x) <= 1)
			return true;

	}
	return false;
}

void add_hp(int id, short value)
{
	Clients[id].hp += value;
}
void add_exp(int id, short value)
{
	Clients[id].exp += value;
	cout << "add_exp : " << Clients[id].exp << endl;


	char mess[100];
	sprintf_s(mess, "%s -> ADD EXP +%d", Clients[id].name, value);
	sendPacket.send_chat_packet(Clients[id]._socket, id, id, mess);

}
void add_level(int id, short value)
{
	Clients[id].level += value;
}
int get_need_exp(int id)
{
	return pow(2, Clients[id].level - 1) * 100;
}
void Player_Heal(int id)
{
	if (Clients[id].hp < 100 + (20 * (Clients[id].level - 1))) {
		Clients[id].state_lock.lock();
		add_hp(id, (100 + (20 * (Clients[id].level - 1))) * 0.1);
		if (Clients[id].hp > 100 + (20 * (Clients[id].level - 1)))
			Clients[id].hp = 100 + (20 * (Clients[id].level - 1));
		Clients[id].state_lock.unlock();

		sendPacket.send_status_change(Clients[id]._socket, id,
			id, Clients[id].hp, Clients[id].maxhp,
			Clients[id].level, Clients[id].exp);
		
		for (int i = 0; i < acceptPlayer; ++i) {
			if (id == i) continue;
			sendPacket.send_status_change(Clients[i]._socket, id,
				id, Clients[id].hp, Clients[id].maxhp,
				Clients[id].level, Clients[id].exp);
		}
		
		char mess[100];
		sprintf_s(mess, "%s -> ADD HP +%2d", Clients[id].name, 20 * Clients[id].level);
		sendPacket.send_chat_packet(Clients[id]._socket, id,id,  mess);
	}

	add_timer(id, EVENT_HEAL, system_clock::now() + 5s);
}

void do_move(int Client_id, char direction,int move_time)
{
	Client& cl = Clients[Client_id];
	cl.last_move_time = move_time;
	int x = cl.x;
	int y = cl.y;

	switch (direction) {
	case D_UP:
		if (g_Map[y - 1][x].type == eBLANK) {
			if (y > 0)
			{
				y--;
				cl.direction = D_UP;
			}
		}
		break;
	case D_DOWN:
		if (g_Map[y + 1][x].type == eBLANK) {
			if (y < (WORLD_HEIGHT - 1))
			{
				y++;
				cl.direction = D_DOWN;
			}
		}
		break;
	case D_LEFT:
		if (g_Map[y][x - 1].type == eBLANK) {
			if (x > 0) {
				x--;
				cl.direction = D_LEFT;
			}
		}
		break;
	case D_RIGHT:
		if (g_Map[y][x + 1].type == eBLANK) {
			if (x < (WORLD_WIDTH - 1)) {
				x++;
				cl.direction = D_RIGHT;
			}
		}
		break;
	default:
		cout << "Invalid move in Client " << Client_id << endl;
		exit(-1);
	}
	cl.x = x;
	cl.y = y;

	unordered_set <int> nearlist;
	for (auto& other : Clients) {
		if (other._id == Client_id)
			continue;
		if (STATUS::ST_INGAME != other._state)
			continue;
		if (false == is_near(Client_id, other._id))
			continue;
		if (true == is_npc(other._id)) {
			//Activate_Player_Move_Event(other._id, cl._id);
			if (true == is_near(Client_id, other._id)) nearlist.insert(other._id);
			wake_up_npc(other._id);

		}
		nearlist.insert(other._id);
	}


	sendPacket.send_move_packet(cl._socket, cl._id, cl._id,
		cl.x, cl.y, cl.last_move_time, cl.direction);



	cl.vl.lock();
	unordered_set <int> my_vl{ cl.viewlist };
	cl.vl.unlock();



	// 새로시야에 들어온 플레이어 처리
	for (auto other : nearlist) {
		if (0 == my_vl.count(other)) {
			cl.vl.lock();
			cl.viewlist.insert(other);
			cl.vl.unlock();
			sendPacket.send_put_object(cl._socket, cl._id,
				other, Clients[other].x, Clients[other].y, Clients[other].name,
				Clients[other].npcCharacterType, Clients[other].npcMoveType);
			sendPacket.send_status_change(cl._socket, cl._id,
				other, Clients[other].hp, Clients[other].maxhp, Clients[other].level, Clients[other].exp);


			if (true == is_npc(other)) {
				continue;
			}

			Clients[other].vl.lock();
			if (0 == Clients[other].viewlist.count(cl._id)) {
				Clients[other].viewlist.insert(cl._id);
				Clients[other].vl.unlock();
				sendPacket.send_put_object(Clients[other]._socket, other,
					cl._id, cl.x, cl.y, cl.name,
					cl.npcCharacterType, cl.npcMoveType);
				sendPacket.send_status_change(Clients[other]._socket, other,
					cl._id, cl.hp, cl.maxhp, cl.level, cl.exp);
			}
			else {
				Clients[other].vl.unlock();
				sendPacket.send_move_packet(Clients[other]._socket, other,
					cl._id, cl.x, cl.y, cl.last_move_time, cl.direction);
			}
		}
		// 계속 시야에 존재하는 플레이어 처리
		else {
			if (true == is_npc(other)) continue;
			Clients[other].vl.lock();
			if (0 != Clients[other].viewlist.count(cl._id)) {
				Clients[other].vl.unlock();

				sendPacket.send_move_packet(Clients[other]._socket, other,
					cl._id, cl.x, cl.y, cl.last_move_time, cl.direction);
			}
			else {
				Clients[other].viewlist.insert(cl._id);
				Clients[other].vl.unlock();
				sendPacket.send_put_object(Clients[other]._socket, other,
					cl._id, cl.x, cl.y, cl.name,
					cl.npcCharacterType, cl.npcMoveType);
				sendPacket.send_status_change(Clients[other]._socket, other,
					cl._id, cl.hp, cl.maxhp, cl.level, cl.exp);
			}
		}
	}
	// 시야에서 사라진 플레이어 처리
	for (auto other : my_vl) {
		if (0 == nearlist.count(other)) {
			cl.vl.lock();
			cl.viewlist.erase(other);
			cl.vl.unlock();
			sendPacket.send_remove_object(cl._socket, cl._id, other, false);

			if (true == is_npc(other)) continue;

			Clients[other].vl.lock();
			if (0 != Clients[other].viewlist.count(cl._id)) {
				Clients[other].viewlist.erase(cl._id);
				Clients[other].vl.unlock();
				sendPacket.send_remove_object(Clients[other]._socket, other, cl._id, false);
			}
			else Clients[other].vl.unlock();
		}
	}


}
void AccessGame(short user_id, char* name)
{
	Client& cl = Clients[user_id];
	Clients[user_id].cl.lock();
	Clients[user_id].name[MAX_ID_LEN] = NULL;
	strcpy_s(Clients[user_id].name, name);
	sendPacket.send_login_ok_packet(cl._socket, user_id, 
		cl.x, cl.y, cl.level, cl.hp, cl.exp);

	if (cl.hp < 100)
		Player_Heal(user_id);
	Clients[user_id]._state = STATUS::ST_INGAME;
	
	Clients[user_id].cl.unlock();

	for (auto& cl : Clients)
	{
		int i = cl._id;
		if (user_id == i) continue;
		//시야를 벗어났으면 처리하지 말아라 (입장 시 시야처리)
		if (true == is_near(user_id, i))
		{
			//npc가 자고있다면 깨우기
			if (STATUS::ST_FREE == Clients[i]._state)
				cl._state = STATUS::ST_INGAME;//activate_npc(i);

			//Clients[i].m_cLock.lock(); //i와 user_id가 같아지는 경우 2중락으로 인한 오류 발생 (데드락)
			//다른 곳에서 status를 바꿀 수 있지만, 실습에서 사용할정도의 복잡도만 유지해야 하기 때문에 일단 놔두기로 한다. 
			//그래도 일단 컴파일러 문제랑 메모리 문제는 해결해야 하기 때문에 status 선언을 atomic으로 바꾼다.
			if (STATUS::ST_INGAME == Clients[i]._state)
			{
				sendPacket.send_put_object(cl._socket, i,
					Clients[i]._id, Clients[i].x, Clients[i].y, Clients[i].name,
					Clients[i].npcCharacterType, Clients[i].npcMoveType);
				sendPacket.send_status_change(cl._socket, i,
					Clients[i]._id, Clients[i].hp, Clients[i].maxhp, Clients[i].level, Clients[i].exp);
					//니가 나를 보면 나도 너를 본다
				if (true == is_player(i)) //플레이어인 경우만 send. 안그러면 소켓이 없어서 에러남
				{
					sendPacket.send_put_object(Clients[i]._socket, user_id
						, Clients[user_id]._id, Clients[user_id].x, Clients[user_id].y, Clients[user_id].name);
					sendPacket.send_status_change(Clients[i]._socket, user_id,
						Clients[user_id]._id, Clients[user_id].hp, Clients[user_id].maxhp,
						Clients[user_id].level, Clients[user_id].exp);
				}
				else
				{
					if (cl.npcCharacterType == NPC_FIGHT)
					{
						do_npc_move(i);

					}
				}
			}
		}
	}


}
void process_packet(int Client_id, char* p)
{
	unsigned char packet_type = p[1];
	Client& cl = Clients[Client_id];
	int DBID = 0;
	switch (packet_type) {
	case CS_PACKET_LOGIN: {

		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
		
		//if (userDB.CheckID(packet->userID))
		//{
			AccessGame(Client_id, packet->userID);
		//}
		//else
		//	sendPacket.send_login_fail_packet(cl._socket, Client_id);
		
		break;
	}
	case CS_PACKET_MOVE: {
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
		
		Clients[Client_id].direction = packet->direction;
		do_move(Client_id, packet->direction, packet->move_time);
		
		break;
	}
	
	case CS_PACKET_ATTACK: {
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(p);

		Client& cl = Clients[Client_id];

		if (false == cl.can_attack) {
			return;
		}

		unordered_set<int> targetlist = cl.viewlist;
		unordered_set<int> eraselist;

		for (const auto& other : targetlist) {
			if (is_npc(other)) {
				if (true == is_attack(cl._id, other)) {
					/*Exp_Over* exp_over = new Exp_Over;
					exp_over->_comp_op = COMP_OP::OP_PLAYER_MOVE;
					*(int*)(exp_over->_net_buf) = other;



					PostQueuedCompletionStatus(g_h_iocp, 1, other, &exp_over->_wsa_over);*/
					
					Clients[other].state_lock.lock();
					add_hp(other, cl.level * 10 * -1);

					// 몬스터 스텟 변경
					sendPacket.send_status_change(cl._socket, other,
						other, Clients[other].hp, Clients[other].maxhp, 
						Clients[other].level, Clients[other].exp);

					
					// 플레이어 사망

					if (Clients[other].hp <= 0) {

						Clients[other]._state = STATUS::ST_FREE;
						
						CAS(&cl._is_active, true, false);
						
						sendPacket.send_remove_object(cl._socket, cl._id, other, true);
						add_exp(cl._id, Clients[other].level * Clients[other].level * 2);
						
						if (cl.exp >= get_need_exp(cl._id)) {
							cl.level += 1;
							//cl.hp += cl.level * 10;
							cl.maxhp = 100 + (20 * (cl.level - 1));

							char mess[100];
							sprintf_s(mess, "%s -> Level UP / %s -> Level : %d", cl.name, cl.name, cl.level);
							for (int i = 0; i < acceptPlayer; ++i)
								sendPacket.send_chat_packet(Clients[i]._socket, 1, i, mess);

						}
							sendPacket.send_status_change(cl._socket, cl._id,
								cl._id, cl.hp, cl.maxhp, cl.level, cl.exp);

						

						eraselist.insert(other);
					}
					Clients[other].state_lock.unlock();
				}
			}
		}
		cl.state_lock.lock();
		cl.can_attack = false;
		cl.state_lock.unlock();

		for (const auto& el : eraselist) {
			cl.vl.lock();
			cl.viewlist.erase(el);
			cl.vl.unlock();

			Clients[el].vl.lock();
			Clients[el].viewlist.clear();
			Clients[el].vl.unlock();

			//add_timer(el, EVENT_REVIVE, system_clock::now() + 5000s);
		}
		
		add_timer(cl._id, EVENT_ATTACK, system_clock::now() + 1s);

		break;
	}
	case CS_PACKET_TELEPORT: {
		cout << "CS_PACKET_TELEPORT" << endl;
		cs_packet_teleport* packet = reinterpret_cast<cs_packet_teleport*>(&p);

		cl.x = 500;
		cl.y = 500;

		sendPacket.send_teleport_packet(Clients[Client_id]._socket, 
			Clients[Client_id].x, Clients[Client_id].y);
	}
		break;
	}
}
void init_map()
{
	char data;
	FILE* fp = fopen("mapData.txt", "rb");
	
	int count = 0;

	while (fscanf(fp, "%c", &data) != EOF) {
		//printf("%c", data);
		switch (data)
		{
		case '0':
			g_Map[count / 800][count % 800].type = eBLANK;
			count++;
			break;
		case '3':
			g_Map[count / 800][count % 800].type = eBLOCKED;
			count++;
			break;
		default:
			break;
		}
	}
	fclose(fp);

	//for (int i = 0; i < WORLD_HEIGHT; ++i)
	//{
	//	for (int j = 0; j < WORLD_WIDTH; ++j)
	//	{
	//		g_Map[i][j].type = eBLANK;
	//	}
	//}
}
void worker()
{
	for (;;) {
		DWORD num_byte;
		LONG64 iocp_key;
		WSAOVERLAPPED* p_over;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_byte, (PULONG_PTR)&iocp_key, &p_over, INFINITE);
		//cout << "GQCS returned.\n";
		int Client_id = static_cast<int>(iocp_key);
		Exp_Over* exp_over = reinterpret_cast<Exp_Over*>(p_over);
		//if (FALSE == ret) {
		//	int err_no = WSAGetLastError();
		//	cout << "GQCS Error : ";
		//	//errorMask.error_display(err_no);
		//	cout << endl;
		//	Disconnect(Client_id);
		//	if (exp_over->_comp_op == COMP_OP::OP_SEND)
		//		delete exp_over;
		//	continue;
		//}

		switch (exp_over->_comp_op) {
		case COMP_OP::OP_RECV: {
			if (num_byte == 0) {
				Disconnect(Client_id);
				continue;
			}
			Client& cl = Clients[Client_id];
			int remain_data = num_byte + cl._prev_size;
			char* packet_start = exp_over->_net_buf;
			int packet_size = packet_start[0];

			while (packet_size <= remain_data) {
				process_packet(Client_id, packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl._prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}
			ZeroMemory(&cl._recv_over._wsa_over, sizeof(cl._recv_over._wsa_over));
			DWORD recv_flag = 0;
			WSARecv(cl._socket, &cl._recv_over._wsa_buf, 1, 0, &recv_flag, &cl._recv_over._wsa_over, NULL);

			
			break;
		}
		case COMP_OP::OP_SEND: {
			if (num_byte != exp_over->_wsa_buf.len) {
				Disconnect(Client_id);
			}
			delete exp_over;
			break;
		}
		case COMP_OP::OP_ACCEPT: {
			cout << "Accept Completed.\n";
			
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
			int new_id = get_new_id();
			if (-1 == new_id) {
				cout << "Maxmum user overflow. Accept aborted.\n";
			}
			else {
				Client& cl = Clients[new_id];
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, new_id, 0);

				cl.x = rand() % WORLD_WIDTH;
				cl.y = rand() % WORLD_HEIGHT;
				cl._id = new_id;
				cl.hp = 10;
				cl.exp = 0;
				cl.level = 1;
				cl.can_attack = true;
				cl.maxhp = 100;
				
				++acceptPlayer;

				cl._recv_over._comp_op = COMP_OP::OP_RECV;
				cl._recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl._recv_over._net_buf);

				cl._recv_over._wsa_buf.len = sizeof(cl._recv_over._net_buf);


				ZeroMemory(&cl._recv_over._wsa_over, sizeof(cl._recv_over._wsa_over));
				cl._socket = c_socket;


				DWORD recv_flag = 0;
				int ret = WSARecv(cl._socket, &cl._recv_over._wsa_buf, 1, 0,
					&recv_flag, &cl._recv_over._wsa_over, NULL);
				if (SOCKET_ERROR == ret) {
						cout << "do_recv error1" << endl;
						int error_num = WSAGetLastError();
						if (ERROR_IO_PENDING != error_num)
							errorMask.error_display(error_num);
						
				}
			}

			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_net_buf)) = c_socket;
			AcceptEx(g_s_socket, c_socket, exp_over->_net_buf + 8, 0, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, NULL, &exp_over->_wsa_over);
		}
			break;
		case COMP_OP::OP_NPC_MOVE: {

			do_npc_move(Client_id);
			delete exp_over;


			break;
		}
		case COMP_OP::OP_PLAYER_MOVE: {
			lua_State* L = Clients[Client_id].L;
			lua_getglobal(L, "event_player_attack");
			lua_pushnumber(L, Client_id);
			lua_pushnumber(L, Clients[Client_id].x);
			lua_pushnumber(L, Clients[Client_id].y);
			lua_pcall(L, 3, 0, 0);
			delete exp_over;
			break;
		}
		case COMP_OP::OP_MOVE_COUNT: {
			lua_State* L = Clients[Client_id].L;
			lua_getglobal(L, "check_move_count");
			lua_pushnumber(L, exp_over->_target);
			lua_pcall(L, 1, 1, 0);
			delete exp_over;
			break;
		}
		case COMP_OP::OP_ATTACK:{
			Client& cl = Clients[Client_id];
			cl.state_lock.lock();
			cl.can_attack = true;
			cl.state_lock.unlock();
			delete exp_over;
			break;
		}
		case COMP_OP::OP_REVIVE: {
			cout << "REVIVE" << endl;
			Client& cl = Clients[Client_id];
			cl.state_lock.lock();
			cl._recv_over._comp_op = COMP_OP::OP_RECV;

			cl.hp = 10;
			cl.level = 1;

			cl.overlap = false;
			cl.move_count = 0;
			cl.state_lock.unlock();

			do_npc_move(cl._id);
			delete exp_over;
			break;
		}
		case COMP_OP::OP_HEAL: {
			Player_Heal(Client_id);
			delete exp_over;
			break;
		}
		
		}
	}
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	//sendPacket.send_chat_packet(user_id, my_id, mess);
	return 0;
}

int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = Clients[user_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = Clients[user_id].y;
	lua_pushnumber(L, y);
	return 1;
}


void update_npc_info(int npc, int player)
{
	Clients[player].hp -= Clients[npc].damage;

	if (0 >= Clients[player].hp) {
		Clients[player].x = 100;
		Clients[player].y = 100;
		Clients[player].hp = Clients[player].maxhp;
		if (0 < Clients[player].exp)
			Clients[player].exp = Clients[player].exp / 2;

	}
	sendPacket.send_status_change(Clients[player]._socket, player,
		player, Clients[player].hp, Clients[player].maxhp, 
		Clients[player].level, Clients[player].exp);

	for (auto other : Clients[player].viewlist) {
		if (true == is_npc(other)) continue;
		sendPacket.send_status_change(Clients[other]._socket, other,
			player, Clients[player].hp, Clients[player].maxhp,
			Clients[player].level, Clients[player].exp);
	}
}
int API_npc_attack(lua_State* L)
{
	int player =
		(int)lua_tointeger(L, -2);
	int npc_id =
		(int)lua_tonumber(L, -1);
	lua_pop(L, 3);


	if (true == Clients[npc_id].can_attack) {
		Clients[npc_id].can_attack = false;
		cout << "npc can_attack" << endl;
		update_npc_info(npc_id, player);

		add_timer(npc_id, EVENT_ATTACK, system_clock::now() + 1s);
	}
	return 0;
}


void Initialize_NPC()
{
	for (int i = NPC_ID_START; i <= NPC_ID_END; ++i) {
		sprintf_s(Clients[i].name, "N%d", i);
		Clients[i].x = rand() % WORLD_WIDTH;
		Clients[i].y = rand() % WORLD_HEIGHT;

		Clients[i]._id = i;
		
		Clients[i]._state = STATUS::ST_INGAME;
		Clients[i]._type = 2;

		// 맵에 충돌체를 제외한 곳에 몬스터 x,y좌표 설정
		while (true)
		{
			int x = rand() % WORLD_WIDTH;
			int y = rand() % WORLD_HEIGHT;

			if (g_Map[x][y].type == eBLANK) {
				Clients[i].x = x;
				Clients[i].y = y;
				break;
			}
		}
		
		lua_State* L = Clients[i].L = luaL_newstate();
		luaL_openlibs(L);
		int error = luaL_loadfile(L, "monster.lua") ||
			lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);
		//lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_npc_attack", API_npc_attack);

		// 몬스터 stat
		Clients[i].level = rand() % 5 + 1;		// 레벨 랜덤으로 세팅
		Clients[i].npcCharacterType = rand() % 3; // STAY FIGHT SPECIAL
		Clients[i].hp = Clients[i].level * 5;
		Clients[i].maxhp = Clients[i].hp;

		if (Clients[i].npcCharacterType == NPC_SPECIAL)
			Clients[i].npcMoveType = NPC_SPECIAL;
		else
			Clients[i].npcMoveType = rand() % 2;

		Clients[i]._is_active = false;
		Clients[i].overlap = false;
		Clients[i].move_count = 0;

		if (Clients[i].npcMoveType == NPC_RANDOM_MOVE) {
			add_timer(i, EVENT_NPC_MOVE, system_clock::now() + 1s);
		}




		
	}
	cout << "Initailize NPC" << endl;

}

void do_npc_move(int npc_id)
{
	Client& cl = Clients[npc_id];
	unordered_set <int> old_viewlist = cl.viewlist;
	unordered_set <int> new_viewlist;

	for (auto& obj : Clients) {
		if (obj._state != STATUS::ST_INGAME)
			continue;
		if (false == is_player(obj._id))
			continue;
		if (true == is_near(npc_id, obj._id))
			old_viewlist.insert(obj._id);
	}
	
	int x = Clients[npc_id].x;
	int y = Clients[npc_id].y;

	switch (rand() % 4) {
	case D_UP:
		if (g_Map[y - 1][x].type == eBLANK) {
			if (y > 0)
			{
				y--;
				cl.direction = D_UP;
			}
		}
		break;
	case D_DOWN:
		if (g_Map[y + 1][x].type == eBLANK) {
			if (y < (WORLD_HEIGHT - 1))
			{
				y++;
				cl.direction = D_DOWN;
			}
		}
		break;
	case D_LEFT:
		if (g_Map[y][x - 1].type == eBLANK) {
			if (x > 0) {
				x--;
				cl.direction = D_LEFT;
			}
		}
		break;
	case D_RIGHT:
		if (g_Map[y][x + 1].type == eBLANK) {
			if (x < (WORLD_WIDTH - 1)) {
				x++;
				cl.direction = D_RIGHT;
			}
		}
		break;
	default:
		cout << "Invalid move in Client " << npc_id << endl;
		exit(-1);
	}

	Clients[npc_id].x = x;
	Clients[npc_id].y = y;
	
	//플레이어 시야 뷰리스트

	for (auto& obj : Clients) {
		if (obj._state != STATUS::ST_INGAME) // INGAME아니면 다음번호
			continue;
		if (false == is_player(obj._id))
			continue;
		if(false == is_npc(obj._id))
			if (true == is_near(npc_id, obj._id)) {
				new_viewlist.insert(obj._id);
			}
		/*if (is_attack(npc_id, obj._id))
			add_timer(npc_id, EVENT_NPC_ATTACK, system_clock::now() + 1s);*/
	}

	// 새로 시야에 들어온 플레이어
	for (auto pl : new_viewlist) {
		if (0 == old_viewlist.count(pl)) {
			Clients[pl].vl.lock();
			Clients[pl].viewlist.insert(npc_id);
			Clients[pl].vl.unlock();
			sendPacket.send_put_object(Clients[pl]._socket, pl,
				npc_id, Clients[npc_id].x, Clients[npc_id].y, Clients[npc_id].name,
				Clients[npc_id].npcCharacterType,Clients[npc_id].npcMoveType);
			sendPacket.send_status_change(Clients[pl]._socket, pl,
				npc_id, Clients[npc_id].hp, Clients[npc_id].maxhp, Clients[npc_id].level, Clients[npc_id].exp);


		}
		else {
			sendPacket.send_move_packet(Clients[pl]._socket, pl, npc_id,
				Clients[npc_id].x, Clients[npc_id].y, Clients[npc_id].last_move_time, Clients[npc_id].direction);
		}
	}
	// 시야에서 사라지는 경우
	for (auto pl : old_viewlist) {
		if (0 == new_viewlist.count(pl)) {
			Clients[pl].vl.lock();
			Clients[pl].viewlist.erase(npc_id);
			Clients[pl].vl.unlock();
			sendPacket.send_remove_object(Clients[pl]._socket, pl, npc_id, false);
		}
	}

	if (true == new_viewlist.empty()) {
		CAS(&cl._is_active, true, false);
	}
	else {
		if(cl._state == STATUS::ST_INGAME)
			add_timer(npc_id, EVENT_NPC_MOVE, system_clock::now() + 1s);
	}
}


void do_timer()
{

	cout << "do_timer" << endl;
	while (true) {
		while (true) {
			if (false == timer_queue.empty()) {
				timer_event ev = timer_queue.top();
				if (ev.start_time > system_clock::now()) break;
				timer_queue.pop();

				switch (ev.ev)
				{
				case EVENT_NPC_MOVE:
				{
					Exp_Over* exp_over = new Exp_Over;
					exp_over->_comp_op = COMP_OP::OP_NPC_MOVE;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &exp_over->_wsa_over);
				}
				break;
				case EVENT_ATTACK:
				{
					Exp_Over* exp_over = new Exp_Over;
					exp_over->_comp_op = COMP_OP::OP_ATTACK;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &exp_over->_wsa_over);
				}
				break;
				case EVENT_REVIVE:
				{

					Exp_Over* exp_over = new Exp_Over;
					exp_over->_comp_op = COMP_OP::OP_REVIVE;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &exp_over->_wsa_over);
				}
				break;
				case EVENT_HEAL:
				{

					Exp_Over* exp_over = new Exp_Over;
					exp_over->_comp_op = COMP_OP::OP_HEAL;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &exp_over->_wsa_over);
				}
				break;
				case EVENT_NPC_ATTACK:
				{
					Exp_Over* exp_over = new Exp_Over;
					exp_over->_comp_op = COMP_OP::OP_PLAYER_MOVE;
				}
				break;
				}
			}
			else break;
		}
		this_thread::sleep_for(1ms);
	}
}

int main()
{
	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char	accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	Exp_Over	accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._net_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_op = COMP_OP::OP_ACCEPT;

	AcceptEx(g_s_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &accept_ex._wsa_over);
	cout << "Accept Called\n";

	for (int i = 0; i < MAX_USER; ++i)
		Clients[i]._id = i;


	/*char userID[20];
	int userPassWord;

	cout << "ID : ";
	cin >> userID;
	if (userDB.CheckID(userID)) {
		cout << "PassWord : ";
		cin >> userPassWord;
	}*/


	cout << "Creating Worker Threads\n";
	init_map();
	Initialize_NPC();

	vector <thread> worker_threads;
	thread timer_thread{ do_timer };
	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(worker);
	for (auto& th : worker_threads)
		th.join();

	timer_thread.join();
	for (auto& cl : Clients) {
		if (STATUS::ST_INGAME == cl._state)
			Disconnect(cl._id);
	}
	closesocket(g_s_socket);
	WSACleanup();
}


