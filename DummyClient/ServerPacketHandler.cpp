#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include <Session.h>
#include "ServerSession.h"

void ServerPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	//cout << "HandlePacket" << endl;
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	switch (header.id)
	{
	case S_TEST:
		Handle_S_TEST(buffer, len);
		//cout << "Handle_S_TEST" << endl;
		break;

	case S_LOGIN:
		Handle_S_LOGIN(buffer, len);		
		break;

	case S_CHAT:
		Handle_S_CHAT(buffer, len);
		break;
	}
}

#pragma pack(1)
// [ PKT_S_TEST ][BuffsListItem BuffsListItem BuffsListItem]
struct PKT_S_TEST
{
	struct BuffsListItem
	{
		uint64 buffId;
		float remainTime;
	};

	uint16 packetSize; // 공용 헤더
	uint16 packetId; // 공용 헤더
	uint64 id; // 8
	uint32 hp; // 4
	uint16 attack; // 2
	string name;
	uint16 buffsOffset;
	uint16 buffsCount;


	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_S_TEST);
		if (packetSize < size)
			return false;

		size += buffsCount * sizeof(BuffsListItem);
		if (size != packetSize)
			return false;

		if (buffsOffset + buffsCount * sizeof(BuffsListItem) > packetSize)
			return false;

		return true;
	}

	using BuffsList = PacketList<PKT_S_TEST::BuffsListItem>;

	BuffsList GetBuffsList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buffsOffset;
		return BuffsList(reinterpret_cast<PKT_S_TEST::BuffsListItem*>(data), buffsCount);
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_S_LOGIN
{
	uint16 packetSize; // 공용 헤더
	uint16 packetId; // 공용 헤더
	uint64 id; // 8

};
#pragma pack()

#pragma pack(1)
struct PKT_S_CHAT
{
	uint16 packetSize; // 공용 헤더
	uint16 packetId; // 공용 헤더
	string msg;
	uint16 id;

};
#pragma pack()


/*-------------
 HandlePacket
-------------*/
// 테스트 패킷
// [ PKT_S_TEST ][BuffsListItem BuffsListItem BuffsListItem]
void ServerPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	//cout << "Handle_S_TEST" << endl;
	BufferReader br(buffer, len);


	PKT_S_TEST* pkt = reinterpret_cast<PKT_S_TEST*>(buffer);

	if (pkt->Validate() == false)
		return;

	//cout << "ID: " << id << " HP : " << hp << " ATT : " << attack << endl;

	PKT_S_TEST::BuffsList buffs = pkt->GetBuffsList();
	
	cout << "BufCount : " << buffs.Count() << endl;
	cout << "ID: " << pkt->id << " HP : " << pkt->hp << " ATT : " << pkt->attack << " Name : " << pkt->name << endl;

	for (auto& buff : buffs)
	{
		cout << "BufInfo : " << buff.buffId << " " << buff.remainTime << endl;
	}

	vector<BuffData> bs{ BuffData {100, 1.5f}, BuffData{200, 2.3f}, BuffData {300, 0.7f } };
	SendBufferRef sendbuffer = ServerPacketHandler::Make_C_TEST(pkt->id, pkt->hp, pkt->attack, bs, pkt->name);
	GSession.Broadcast(sendbuffer);
}

// 로그인 패킷
void ServerPacketHandler::Handle_S_LOGIN(BYTE* buffer, int32 len)
{
	//cout << "Handle_S_LOGIN" << endl;
	BufferReader br(buffer, len);

	PKT_S_LOGIN* pkt = reinterpret_cast<PKT_S_LOGIN*>(buffer);

	//cout << "Login complete !! " << endl;
	cout << "Hello Player " << pkt->id << endl;
}

// 채팅 패킷
void ServerPacketHandler::Handle_S_CHAT(BYTE* buffer, int32 len)
{
	//cout << "Handle_S_CHAT" << endl;
	BufferReader br(buffer, len);

	PKT_S_CHAT* pkt = reinterpret_cast<PKT_S_CHAT*>(buffer);

	//cout << "Login complete !! " << endl;
	cout << "Player " << pkt->id << " : " << pkt->msg << endl;
}

/*-------------
   MakePacket
--------------*/

// 테스트 패킷
SendBufferRef ServerPacketHandler::Make_C_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, string name)
{
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();

	bw << id << hp << attack << name;

	struct ListHeader
	{
		uint16 offset;
		uint16 count;
	};

	ListHeader* buffsHeader = bw.Reserve<ListHeader>();

	buffsHeader->offset = bw.WriteSize();
	buffsHeader->count = buffs.size();

	for (BuffData& buff : buffs)
		bw << buff.buffId << buff.remainTime;

	header->size = bw.WriteSize();
	header->id = C_TEST;

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
}


// 로그인 패킷
SendBufferRef ServerPacketHandler::Make_C_LOGIN()
{
	//cout << "Make_C_LOGIN" << endl;
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();

	header->size = bw.WriteSize();
	header->id = C_LOGIN; 

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
	
}
// 채팅 패킷
SendBufferRef ServerPacketHandler::Make_C_CHAT(string msg)
{
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();

	uint16 num = 0;
	bw << msg << num;
	header->size = bw.WriteSize();
	header->id = C_CHAT;

	sendBuffer->Close(bw.WriteSize());


	return sendBuffer;
}
