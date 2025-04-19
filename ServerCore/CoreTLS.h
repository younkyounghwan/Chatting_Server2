#pragma once
#include <stack>

/*
TLS 영역을 정의
*/

extern thread_local uint32				LThreadId;
extern thread_local std::stack<int32>	LLockStack;
extern thread_local SendBufferChunkRef	LSendBufferChunk;