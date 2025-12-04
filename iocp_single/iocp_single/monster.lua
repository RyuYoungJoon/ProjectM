myid = 99999;

function set_uid(x)
   myid = x;
end

function event_player_attack(player_id, x, y)
	my_x = API_get_x(my_id)
	my_y = API_get_y(my_id)
	if x == my_x then 
		if y == my_y then
			API_npc_attack(player_id, my_id)
		end
	end
end

function Send_Message(player_id, mess)
	my_x = API_get_x(my_id)
	my_y = API_get_y(my_id)
	if x == my_x then
		if y == my_y then
			API_SendMessage(my_id, mess)
		end
	end
end



