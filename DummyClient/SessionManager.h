#pragma once

class ServerSession;
using ServerSessionRef = shared_ptr<ServerSession>;

class SessionManager
{
public:
	void Add(ServerSessionRef session);
	void Release(ServerSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	Set<ServerSessionRef> _sessions;
};

extern SessionManager GSession;
