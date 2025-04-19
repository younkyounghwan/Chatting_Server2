#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

/*-------------
	Service
--------------*/

// 공통 서비스 클래스 생성자
Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{

}

Service::~Service()
{
}

void Service::CloseService()
{
	// TODO - 서비스 종료 처리(세션 정리)
}

// 세션 생성
SessionRef Service::CreateSession()
{
	// 팩토리 함수로 세션 생성
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());

	// iocp core에 세션 등록
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}
// 세션 추가
void Service::AddSession(SessionRef session)
{
	WRITE_LOCK;
	_sessionCount++;
	_sessions.insert(session);
}

// 세션 해제
void Service::ReleaseSession(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
}

/*-----------------
	ClientService
------------------*/

// 클라전용 서비스 생성자
ClientService::ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount)
{
}

// 클라 서비스 시작(연결 시도)
bool ClientService::Start()
{
	if (CanStart() == false)
		return false;

	// 최대 세션 수만큼 연결 시도
	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; i++)
	{
		SessionRef session = CreateSession();
		if (session->Connect() == false)
			return false;
	}

	return true;
}

// 서버 전용 서비스 생성자
ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

// 서버 서비스 시작(리스너 생성 및 acceptex 등록
bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	// 리스너 생성
	_listener = MakeShared<Listener>();
	if (_listener == nullptr)
		return false;

	// 서버 서비스 자기 자신을 listener에 전달
	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	// TODO - 리스너 정리 및 세션 종료 처리

	Service::CloseService(); // 종료 호출
}
