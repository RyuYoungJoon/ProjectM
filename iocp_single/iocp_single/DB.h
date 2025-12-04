#pragma once

#include <sqlext.h>



class DB
{
public:
	bool IsSavedId(int id);
	void SavePlayerData(char* name, short exp, short hp);
	bool CheckID(char* id);
	void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
};

