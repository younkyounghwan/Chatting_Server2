#include "pch.h"
#include "MemoryPool.h"

/*-----------------
	MemoryPool
------------------*/

// 지정된 크기의 메모리 블록을 관리할 수 있도록 초기화
MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
	::InitializeSListHead(&_header);
}

// S_List에 남아있는 모든 메모리 블록 해제
MemoryPool::~MemoryPool()
{
	while (MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header)))
		::_aligned_free(memory);
}

// 메모리 블록 반환
void MemoryPool::Push(MemoryHeader* ptr)
{
	ptr->allocSize = 0;

	::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

	// 사용가운트 감소, 예약 카운트 증가
	_useCount.fetch_sub(1);
	_reserveCount.fetch_add(1);
}

// 메모리 블록 요청
MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header));

	// 없으면 새로 만들다
	if (memory == nullptr)
	{
		memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, SLIST_ALIGNMENT));
	}
	else
	{
		ASSERT_CRASH(memory->allocSize == 0);
		_reserveCount.fetch_sub(1);
	}

	// 예약 카운트 감소
	_useCount.fetch_add(1);

	return memory;
}
