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
{ // lisnter 소켓 및 관련 accept 이벤트 정리
	SocketUtils::Close(_socket);

	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		// TODO - 새션 연결 해제

		xdelete(acceptEvent);
	}
}

// 서버 소켓 생성 및 acceptex 등록
bool Listener::StartAccept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	// 리스닝용 소켓 생성
	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	// iocp에 리스너 소켓 등록
	if (_service->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	// 주소 재사용 허용
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// linger 용섭 해제(지연 방지)
	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	// 바인드
	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	// 리스너 시작
	if (SocketUtils::Listen(_socket) == false)
		return false;

	// acceptex용 이벤트 객체 등록
	const int32 acceptCount = _service->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();
		acceptEvent->owner = shared_from_this(); // 이벤트 소유자 설정
		_acceptEvents.push_back(acceptEvent); // 리스트에 저장
		RegisterAccept(acceptEvent); 
	}

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(_socket);
}

// iocp 핸들 반환
HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

// accept 처리
void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	// 새션 생성 및 iocp 등록
	SessionRef session = _service->CreateSession(); // Register IOCP

	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;

	// 비동기 accept 요청
	if (false == SocketUtils::AcceptEx(_socket, 
		session->GetSocket(), 
		session->_recvBuffer.WritePos(), // 초기 수신 버퍼
		0, 
		sizeof(SOCKADDR_IN) + 16, // 로컬 주소 공간
		sizeof(SOCKADDR_IN) + 16, // 원격 주소 공간
		OUT & bytesReceived, 
		static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{ // pending시 재시도
			RegisterAccept(acceptEvent);
		}
	}
}

// accept 완료 후 처리
void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	SessionRef session = acceptEvent->session;

	// acceptex에서 생성한 소켓에 대한 정보 업데이트
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

	session->SetNetAddress(NetAddress(sockAddress)); // 네트워크 주소 저장
	session->ProcessConnect(); // 연결 처리 및 recv 등록 및 수행
	RegisterAccept(acceptEvent); // 다음 accept 등록
}