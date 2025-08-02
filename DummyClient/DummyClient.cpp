#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ServerSession.h"
#include "SessionManager.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "ServerPacketHandler.h"

char sendData[] = "Hello World";

int main()
{
	cout << "I'm Client " << endl;
	this_thread::sleep_for(2s);

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>, 
		5);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	while (true)
	{
		SendBufferRef sendbuffer = ServerPacketHandler::Make_C_CHAT(sendData);
		GSession.Broadcast(sendbuffer);

		this_thread::sleep_for(1s);
	}

	GThreadManager->Join();
}