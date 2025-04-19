#pragma once

enum
{
	S_TEST = 1,
	C_TEST = 2,
	S_LOGIN = 3,
	C_LOGIN = 4,
	S_CHAT = 5,
	C_CHAT = 6
};

struct BuffData
{
	uint64 buffId;
	float remainTime;
};

class ClientPacketHandler
{
public:
	static void HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len);

	static void Handle_C_TEST(PacketSessionRef& session, BYTE* buffer, int32 len);
	static void Handle_C_LOGIN(PacketSessionRef& session, BYTE* buffer, int32 len);
	static void Handle_C_CHAT(PacketSessionRef& session, BYTE* buffer, int32 len);
	static SendBufferRef Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, string name);
	static SendBufferRef Make_S_LOGIN(uint64 id);
	static SendBufferRef Make_S_CHAT(uint64 id, string msg);

private:

};

template<typename T, typename C>
class PacketIterator
{
public:
	PacketIterator(C& container, uint16 index) : _container(container), _index(index) { }

	bool				operator!=(const PacketIterator& other) const { return _index != other._index; }
	const T& operator*() const { return _container[_index]; }
	T& operator*() { return _container[_index]; }
	T* operator->() { return &_container[_index]; }
	PacketIterator& operator++() { _index++; return *this; }
	PacketIterator		operator++(int32) { PacketIterator ret = *this; ++_index; return ret; }

private:
	C& _container;
	uint16			_index;
};

template<typename T>
class PacketList
{
public:
	PacketList() : _data(nullptr), _count(0) { }
	PacketList(T* data, uint16 count) : _data(data), _count(count) { }

	T& operator[](uint16 index)
	{
		ASSERT_CRASH(index < _count);
		return _data[index];
	}

	uint16 Count() { return _count; }

	// ranged-base for Áö¿ø
	PacketIterator<T, PacketList<T>> begin() { return PacketIterator<T, PacketList<T>>(*this, 0); }
	PacketIterator<T, PacketList<T>> end() { return PacketIterator<T, PacketList<T>>(*this, _count); }

private:
	T* _data;
	uint16		_count;
};