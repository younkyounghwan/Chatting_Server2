#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

/*--------------
	IocpCore
---------------*/

IocpCore::IocpCore()
{ // iocp 포트 생성
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

// 소켓/파일 핸들을 iocp에 등록
bool IocpCore::Register(IocpObjectRef iocpObject)
{
	// 대상 핸드을 기존 iocp 포트에 연결
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, /*key*/0, 0);
}

// iocp 큐에서 완료된 이벤트를 받아 처리
bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0; // 완료된 io에서 전송된 바이트 수
	ULONG_PTR key = 0;	// iocp 키
	IocpEvent* iocpEvent = nullptr; // 완료된 이벤트 정보(overlapped)

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, OUT &key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{ // 이벤트의 소유 객체를 통해 실제 처리 수행
		IocpObjectRef iocpObject = iocpEvent->owner;
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
