#pragma once

/*-------------------
	BaseAllocator
-------------------*/

class BaseAllocator
{ // Allocator 기본형
public:
	static void*	Alloc(int32 size);
	static void		Release(void* ptr);
};

/*-------------------
	StompAllocator
-------------------*/

class StompAllocator
{ // 페이징 연산을 통해 오버플로우를 막는 코드 작성
	enum { PAGE_SIZE = 0x1000 };

public:
	static void*	Alloc(int32 size);
	static void		Release(void* ptr);
};

/*-------------------
	PoolAllocator
-------------------*/

class PoolAllocator
{ // ObjectPool과 연동되어 사용된다. 
public:
	static void*	Alloc(int32 size);
	static void		Release(void* ptr);
};

/*-------------------
	STL Allocator
-------------------*/

template<typename T>
class StlAllocator
{ // stl자료형에 맞는 커스텀 Allocator를 작성
public:
	using value_type = T;

	StlAllocator() { }

	template<typename Other>
	StlAllocator(const StlAllocator<Other>&) { }

	T* allocate(size_t count)
	{
		const int32 size = static_cast<int32>(count * sizeof(T));
		return static_cast<T*>(PoolAllocator::Alloc(size));
	}

	void deallocate(T* ptr, size_t count)
	{
		PoolAllocator::Release(ptr);
	}
};