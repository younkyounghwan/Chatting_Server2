#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "Player.h"
#include "DBConnectionPool.h"

// packet id에 따라 지정 메소드로 연결
void ClientPacketHandler::HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;
	//cout << " header.id : " << header.id << endl;
	switch (header.id)
	{		
	case C_TEST:
		Handle_C_TEST(session, buffer, len);
		break;
	
	case C_LOGIN:
		Handle_C_LOGIN(session, buffer, len);		
		break;

	case C_CHAT:
		Handle_C_CHAT(session, buffer, len);
		break;

	default:
		break;
	}
}

/*----------
  패킷 명세
-----------*/

#pragma pack(1)
struct PKT_C_TEST
{
	struct BuffsListItem
	{
		uint64 buffId;
		float remainTime;
	};

	uint16 packetSize; 
	uint16 packetId; 
	uint64 id; 
	uint32 hp;
	uint16 attack; 
	string name;
	uint16 buffsOffset;
	uint16 buffsCount;


	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_C_TEST);
		if (packetSize < size)
			return false;

		size += buffsCount * sizeof(BuffsListItem);
		if (size != packetSize)
			return false;

		if (buffsOffset + buffsCount * sizeof(BuffsListItem) > packetSize)
			return false;

		return true;
	}

	using BuffsList = PacketList<PKT_C_TEST::BuffsListItem>;

	BuffsList GetBuffsList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buffsOffset;
		return BuffsList(reinterpret_cast<PKT_C_TEST::BuffsListItem*>(data), buffsCount);
	}
};
#pragma pack()


#pragma pack(1)
struct PKT_C_LOGIN
{
	uint16 packetSize; 
	uint16 packetId; 
};
#pragma pack()

#pragma pack(1)
struct PKT_C_CHAT
{
	uint16 packetSize;
	uint16 packetId; 
	string msg;
	uint16 num;
};
#pragma pack()

/*-------------
 HandlePacket
-------------*/

// 테스트 패킷
void ClientPacketHandler::Handle_C_TEST(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	//cout << "Handle_C_TEST" << endl;
	BufferReader br(buffer, len);


	PKT_C_TEST* pkt = reinterpret_cast<PKT_C_TEST*>(buffer);

	if (pkt->Validate() == false)
		return;

	//cout << "ID: " << id << " HP : " << hp << " ATT : " << attack << endl;

	PKT_C_TEST::BuffsList buffs = pkt->GetBuffsList();

	cout << "BufCount : " << buffs.Count() << endl;
	cout << "ID: " << pkt->id << " HP : " << pkt->hp << " ATT : " << pkt->attack << " Name : " << pkt->name << endl;

	for (auto& buff : buffs)
	{
		cout << "BufInfo : " << buff.buffId << " " << buff.remainTime << endl;
	}
}

// 로그인 패킷
void ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	BufferReader br(buffer, len);

	PKT_C_LOGIN* pkt = reinterpret_cast<PKT_C_LOGIN*>(buffer);

	static Atomic<uint64> idGenerator = 1;
	
	PlayerRef playerRef = MakeShared<Player>();
	playerRef->playerId = idGenerator++;
	gameSession->_currentPlayer = playerRef;
	playerRef->ownerSession = gameSession;
	gameSession->_players.push_back(playerRef);
	

	cout << "Player " << playerRef->playerId <<" is Connected!! " << endl;
	
	SendBufferRef sendbuffer = ClientPacketHandler::Make_S_LOGIN(playerRef->playerId);
	session->Send(sendbuffer);

	{ // DB
		DBConnection* dbConn = GDBConnectionPool->Pop();

		dbConn->Unbind();	// 기존에 바인딩 된 정보 날림

		// 넘길 인자 바인딩
		int32 Count = 0;
		SQLLEN len = 0;

		// 넘길 인자 바인딩
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(Count), &Count, &len));

		// SQL 실행
		ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Count]([Count]) VALUES(?)"));

		GDBConnectionPool->Push(dbConn);
	}
}

// 채팅 패킷
void ClientPacketHandler::Handle_C_CHAT(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	BufferReader br(buffer, len);

	PKT_C_CHAT* pkt = reinterpret_cast<PKT_C_CHAT*>(buffer);


	cout << "Player " << gameSession->_currentPlayer->playerId << " : " << pkt->msg << endl;

	SendBufferRef sendbuffer = ClientPacketHandler::Make_S_CHAT(gameSession->_currentPlayer->playerId, pkt->msg);
	GSessionManager.Broadcast(sendbuffer);

	{ // DB
		DBConnection* dbConn = GDBConnectionPool->Pop();
		dbConn->Unbind();	// 기존에 바인딩 된 정보 날림

		// 조회할 ID 입력
		int32 targetId = gameSession->_currentPlayer->playerId;
		SQLLEN idLen = 0;

		// count 값을 받을 변수
		int32 outCount = 0;
		SQLLEN CountLen = 0;

		// 1. ID 조건으로 파라미터 바인딩
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(targetId), &targetId, &idLen));

		// 2. SELECT 컬럼 출력 바인딩 (gold 값)
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_LONG, sizeof(outCount), &outCount, &CountLen));

		// 3. Execute - 조건 ID에 해당하는 골드 조회
		ASSERT_CRASH(dbConn->Execute(L"SET NOCOUNT ON; SELECT count FROM [dbo].[Count]"));

		// 4. Fetch - 항상 맨 위의 row부터 (Unbind + SQL_CLOSE로 커서 초기화되기 때문)
		if (dbConn->Fetch())
		{ // 골드 값을 변경
			outCount++;

			// 5. 바인딩 초기화 후 업데이트 준비
			dbConn->Unbind();

			// 6. UPDATE 쿼리용 파라미터 바인딩 (골드, ID 순서)
			ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(outCount), &outCount, &CountLen));
			ASSERT_CRASH(dbConn->BindParam(2, SQL_C_LONG, SQL_INTEGER, sizeof(targetId), &targetId, &idLen));

			// 7. Execute - UPDATE 실행
			ASSERT_CRASH(dbConn->Execute(L"UPDATE [dbo].[Count] SET count = (?)"));

			cout << "ID: " << targetId << " Count: " << outCount << endl;
		}
		else
		{
			cout << "No data found for ID: " << targetId << endl;
		}
		dbConn->Unbind();

		GDBConnectionPool->Push(dbConn);
	}
}

/*------------- 
   MakePacket
-------------*/

// 테스트 패킷
SendBufferRef ClientPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, string name)
{
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();
	// id(uint64), 체력(uint32), 공격력(uint16)
	bw << id << hp << attack << name;

	struct ListHeader
	{
		uint16 offset;
		uint16 count;
	};

	// 가변 데이터
	ListHeader* buffsHeader = bw.Reserve<ListHeader>();

	buffsHeader->offset = bw.WriteSize();
	buffsHeader->count = buffs.size();

	for (BuffData& buff : buffs)
		bw << buff.buffId << buff.remainTime;

	header->size = bw.WriteSize();
	header->id = S_TEST; 

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
}

// 로그인 패킷
SendBufferRef ClientPacketHandler::Make_S_LOGIN(uint64 id)
{
	//cout << "Make_S_LOGIN" << endl;
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();

	bw << id;
	header->size = bw.WriteSize();
	header->id = S_LOGIN;

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
}

// 채팅 패킷
SendBufferRef ClientPacketHandler::Make_S_CHAT(uint64 id, string msg)
{
	//cout << "Make_S_CHAT" << endl;
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();

	bw << msg << id;
	header->size = bw.WriteSize();
	header->id = S_CHAT;

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
}

