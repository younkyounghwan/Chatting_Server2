#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/*--------------
	Listener
---------------*/

Listener::~Listener()
{ // lisnter ���� �� ���� accept �̺�Ʈ ����
	SocketUtils::Close(_socket);

	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		// TODO - ���� ���� ����

		xdelete(acceptEvent);
	}
}

// ���� ���� ���� �� acceptex ���
bool Listener::StartAccept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	// �����׿� ���� ����
	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	// iocp�� ������ ���� ���
	if (_service->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	// �ּ� ���� ���
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// linger �뼷 ����(���� ����)
	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	// ���ε�
	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	// ������ ����
	if (SocketUtils::Listen(_socket) == false)
		return false;

	// acceptex�� �̺�Ʈ ��ü ���
	const int32 acceptCount = _service->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();
		acceptEvent->owner = shared_from_this(); // �̺�Ʈ ������ ����
		_acceptEvents.push_back(acceptEvent); // ����Ʈ�� ����
		RegisterAccept(acceptEvent); 
	}

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(_socket);
}

// iocp �ڵ� ��ȯ
HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

// accept ó��
void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	// ���� ���� �� iocp ���
	SessionRef session = _service->CreateSession(); // Register IOCP

	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;

	// �񵿱� accept ��û
	if (false == SocketUtils::AcceptEx(_socket, 
		session->GetSocket(), 
		session->_recvBuffer.WritePos(), // �ʱ� ���� ����
		0, 
		sizeof(SOCKADDR_IN) + 16, // ���� �ּ� ����
		sizeof(SOCKADDR_IN) + 16, // ���� �ּ� ����
		OUT & bytesReceived, 
		static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{ // pending�� ��õ�
			RegisterAccept(acceptEvent);
		}
	}
}

// accept �Ϸ� �� ó��
void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	SessionRef session = acceptEvent->session;

	// acceptex���� ������ ���Ͽ� ���� ���� ������Ʈ
	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress)); // ��Ʈ��ũ �ּ� ����
	session->ProcessConnect(); // ���� ó�� �� recv ��� �� ����
	RegisterAccept(acceptEvent); // ���� accept ���
}