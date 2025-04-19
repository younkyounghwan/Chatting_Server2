#include "pch.h"
#include "ServerSession.h"
#include "SessionManager.h"
#include "ServerPacketHandler.h"

void ServerSession::OnConnected()
{
	//cout << "OnConnected!!" << endl;
	ServerSessionRef session = static_pointer_cast<ServerSession>(shared_from_this());
	GSession.Add(session);
	SendBufferRef sendBuffer = ServerPacketHandler::Make_C_LOGIN();
	session->Send(sendBuffer);
	//GSession.Broadcast(sendBuffer);
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	//cout << "OnRecvPacket" << endl;
	ServerPacketHandler::HandlePacket(buffer, len);
}

void ServerSession::OnDisconnected()
{
	//cout << "Disconnected" << endl;

	GSession.Release(static_pointer_cast<ServerSession>(shared_from_this()));
}

void ServerSession::OnSend(int32 len)
{
}
