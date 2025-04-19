#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include <tchar.h>
#include "DBConnectionPool.h"

int main()
{
	// VS 내장 SQL 데이터베이스 서버를 사용

	ASSERT_CRASH(GDBConnectionPool->Connect(
		10, 
		L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\ProjectsV13;Database=ServerDB;Trusted_Connection=Yes;"
	));

	// Create Table
	{
		auto query = L"									\
			DROP TABLE IF EXISTS [dbo].[Count];			\
			CREATE TABLE [dbo].[Count]					\
			(											\
				[id] INT NOT NULL PRIMARY KEY IDENTITY, \
				[count] INT NULL							\
			);";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}

	cout << "I'm Server " << endl;
	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager 등
		1);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}				
			});
	}		


	GThreadManager->Join();
}