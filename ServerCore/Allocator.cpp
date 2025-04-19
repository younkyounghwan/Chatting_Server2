#include "pch.h"
#include "Allocator.h"
#include "Memory.h"

/*-------------------
	BaseAllocator
-------------------*/

// ���� �⺻���� �޸� �Ҵ��� 
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

// ����� �� ���� Ž���� ������ stomp �Ҵ���
// �Ҵ� �� ������ ��迡 ���� �������� �����ϰ�
// ���� �������� ���� ħ���� ���� �� �ִ�.
void* StompAllocator::Alloc(int32 size)
{ // ��û�� ������ ������ �ø� ó��
	const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	// �޸� ����� ���� �������� ���ϴ� ������ �������� ���
	// ������ ���� �����Ͱ� ��ġ�ϵ��� �� (����� ����)
	const int64 dataOffset = pageCount * PAGE_SIZE - size;
	
	void* baseAddress = ::VirtualAlloc(
		NULL, 
		pageCount * PAGE_SIZE, // ��ü �޸� ũ��(������ ���ĵ�)
		MEM_RESERVE | MEM_COMMIT, // ���� + ���� Ŀ��
		PAGE_READWRITE); // RW ����
	return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
	// ���� base �ּҸ� ���ϱ� ���� ���� �����͸� ������ ���� �������� ����
	const int64 address = reinterpret_cast<int64>(ptr);
	const int64 baseAddress = address - (address % PAGE_SIZE);	
	::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}

/*-------------------
	PoolAllocator
-------------------*/

// ObjectPool�� �����Ǿ� ���ȴ�. 

void* PoolAllocator::Alloc(int32 size)
{ // �޸� ��� �Ҵ�
	return GMemory->Allocate(size);
}

void PoolAllocator::Release(void* ptr)
{ // �޸� ��ȯ
	GMemory->Release(ptr);
}