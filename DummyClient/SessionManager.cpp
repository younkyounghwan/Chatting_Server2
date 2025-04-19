#include "pch.h"
#include "SessionManager.h"
#include "ServerSession.h"

SessionManager GSession;

void SessionManager::Add(ServerSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void SessionManager::Release(ServerSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

void SessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (ServerSessionRef session : _sessions)
	{
		session->Send(sendBuffer);
		//cout << "Send" << endl;
	}
}
