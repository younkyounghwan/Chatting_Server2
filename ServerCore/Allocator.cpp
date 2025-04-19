#include "pch.h"
#include "Allocator.h"
#include "Memory.h"

/*-------------------
	BaseAllocator
-------------------*/

// 가장 기본적인 메모리 할당자 
void* BaseAllocator::Alloc(int32 size)
{
	return ::malloc(size);
}

void BaseAllocator::Release(void* ptr)
{
	::free(ptr);
}

/*-------------------
	StompAllocator
-------------------*/

// 디버깅 및 버그 탐지에 유용한 stomp 할당자
// 할당 시 페이지 경계에 맞춰 뒤쪽으로 정렬하고
// 인접 페이지에 대한 침범을 막을 수 있다.
void* StompAllocator::Alloc(int32 size)
{ // 요청한 페이지 단위로 올림 처리
	const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	// 메모리 블록의 시작 시점에서 원하는 데이터 오프셋을 계산
	// 페이지 끝에 데이터가 위치하도록 함 (디버깅 목적)
	const int64 dataOffset = pageCount * PAGE_SIZE - size;
	
	void* baseAddress = ::VirtualAlloc(
		NULL, 
		pageCount * PAGE_SIZE, // 전체 메모리 크기(페이지 정렬됨)
		MEM_RESERVE | MEM_COMMIT, // 예약 + 실재 커밋
		PAGE_READWRITE); // RW 권한
	return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
	// 원래 base 주소를 구하기 위해 현재 포인터를 페이지 정렬 기준으로 내림
	const int64 address = reinterpret_cast<int64>(ptr);
	const int64 baseAddress = address - (address % PAGE_SIZE);	
	::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}

/*-------------------
	PoolAllocator
-------------------*/

// ObjectPool과 연동되어 사용된다. 

void* PoolAllocator::Alloc(int32 size)
{ // 메모리 블록 할당
	return GMemory->Allocate(size);
}

void PoolAllocator::Release(void* ptr)
{ // 메모리 반환
	GMemory->Release(ptr);
}