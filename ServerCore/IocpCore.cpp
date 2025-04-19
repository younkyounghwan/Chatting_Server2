#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

/*--------------
	IocpCore
---------------*/

IocpCore::IocpCore()
{ // iocp ��Ʈ ����
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

// ����/���� �ڵ��� iocp�� ���
bool IocpCore::Register(IocpObjectRef iocpObject)
{
	// ��� �ڵ��� ���� iocp ��Ʈ�� ����
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, /*key*/0, 0);
}

// iocp ť���� �Ϸ�� �̺�Ʈ�� �޾� ó��
bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0; // �Ϸ�� io���� ���۵� ����Ʈ ��
	ULONG_PTR key = 0;	// iocp Ű
	IocpEvent* iocpEvent = nullptr; // �Ϸ�� �̺�Ʈ ����(overlapped)

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, OUT &key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{ // �̺�Ʈ�� ���� ��ü�� ���� ���� ó�� ����
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
