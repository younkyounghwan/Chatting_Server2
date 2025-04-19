#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

/*-------------
	Service
--------------*/

// ���� ���� Ŭ���� ������
Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{

}

Service::~Service()
{
}

void Service::CloseService()
{
	// TODO - ���� ���� ó��(���� ����)
}

// ���� ����
SessionRef Service::CreateSession()
{
	// ���丮 �Լ��� ���� ����
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());

	// iocp core�� ���� ���
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}
// ���� �߰�
void Service::AddSession(SessionRef session)
{
	WRITE_LOCK;
	_sessionCount++;
	_sessions.insert(session);
}

// ���� ����
void Service::ReleaseSession(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
}

/*-----------------
	ClientService
------------------*/

// Ŭ������ ���� ������
ClientService::ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount)
{
}

// Ŭ�� ���� ����(���� �õ�)
bool ClientService::Start()
{
	if (CanStart() == false)
		return false;

	// �ִ� ���� ����ŭ ���� �õ�
	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; i++)
	{
		SessionRef session = CreateSession();
		if (session->Connect() == false)
			return false;
	}

	return true;
}

// ���� ���� ���� ������
ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

// ���� ���� ����(������ ���� �� acceptex ���
bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	// ������ ����
	_listener = MakeShared<Listener>();
	if (_listener == nullptr)
		return false;

	// ���� ���� �ڱ� �ڽ��� listener�� ����
	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	// TODO - ������ ���� �� ���� ���� ó��

	Service::CloseService(); // ���� ȣ��
}
