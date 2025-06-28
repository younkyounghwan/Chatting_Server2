# Chatting_Server
C++로 구현된 TCP/IP기반의 IOCP 채팅 서버 프로젝트입니다.
IOCP를 사용하여 효율적으로 네트워크 이벤트를 처리할 수 있도록 설계되었습니다.

## 특징
- 비동기 이벤트 기반 (IOCP)
- 멀티스레드 환경에서 다중 클라이언트 연결 및 패킷 송수신 지원
- 패킷 기반의 'send', 'recv' 처리 로직 구현
- 'Session'을 활용한 연결 관리 및 이벤트 처리

## 스크립트 설명

> **Client**
 + Main
    + DummyClient : 클라이언트의 연결과 클라의 기본적인 동작을 정의한다.
    + ServerPacketHandler : server로부터 들어오는 패킷을 handling하고 server로 보낼 패킷을 make해서 session을 통해 send한다.
    + ServerSession : server와의 연결과 패킷 송수신을 매개한다.
    + SessionManager: 다중 client 연결을 위해 server와 client간 연결중 생성된 session을 관리한다.

> **Server**
  + GameContents
    + Player : 플레이어의 id와 고유 session을 저장한다. 이는 server가 각 클라와 상호작용 할때 사용된다.

  + Main
    + GameServer : 다중 클라이언트의 연결을 보장하고, 멀티쓰레드 환경에서 클라로부터 들어오는 이벤트를 처리할 수 있는 환경을 제공한다.
    + ClientPacketHandler : client로부터 들어오는 패킷을 handling하고 client로 보낼 패킷을 make해서 session을 통해 send한다.
    + GameSession : client와의 연결과 패킷 송수신을 매개한다.
    + GameSessionManager : server에 연결된 여러 client의 session을 관리한다.

> **ServerCore**
  + DB - ODBC 사용
    + DBConnection: DB 연결. 쿼리를 실행하고 데이터 fetch 수행
    + DBConnectionPool: 지정된 수의 Handle을 pool을 사용하여 관리.
  
  + Main
    + CoreGlobal : ThreadManager, Memory, SendBuffer, DeadLockProfiler를 글로벌하게 사용할 수 있게 한다.
    + CoreMacro : Crash관리와 Lock을 사용하기 위한 메크로를 정의한다. 
    + CorePch: lib를 솔루션 전체에서 사용할 수 있게끔 한다. 미리 컴파일된 헤더로 분류된다.
    + CoreTLS: 멀티쓰레드 환경에서 사용되는 쓰레드의 정보를 저장한다.
    + Types: 기본 자료형를 재정의하고 스마트 포인터 형식을 재정의한다.

  + Memory 
    + Allocator: 기존 메모리할당채제에서 생길 수 있는 메모리 오염이나 무분별한 typecasting 문제를 보완한다. 
    + Container: STL자료형의 Allocator를 커스텀해서 재정의한다.
    + Memory: 커스텀한 Allocator를 사용할 xnew, xdelete를 정의. memoryPool을 관리
    + MemoryPool: 메모리를 memoryPool에서 끌어쓰고 다쓴 메모리를 재활용한다.
    + ObjectPool: memoryPool을 사용할 때 원하는 타입을 지정해서 사용한다. (디버깅 용이)
    + RefCounting: 스마트 포인터를 사용할때 필요한 reference counting을 처리한다.
    + TypeCast: 기존 자료형이 type casting을 할때 보다 유동적으로 casting할 수 있게 한다.

  + Network
    + IocpCore: 각종 이벤트를 handling하여 적재적소에 dispatch한다. IOCP 구현의 핵심
    + IocpEvent: Connect, Disconnect, Accept, Recv, Send 이벤트를 정의한다.
    + Listener: Dispatch를 통해 Client로부터 들어오는 연결을 비동기로 처리한다.
    + NetAddress: 네트워크 주소를 저장하는 포멧
    + RecvBuffer: Data를 recv하는 buffer
    + SendBuffer: Data를 send하는 buffer. SendBuffer를 다양한 곳에 사용하기 위해 SendBufferManager추가
    + Service: session생성과 해제 관리. client와 server차원에서의 session 관리.
    + Session: Connect, Disconnect, send, recv 동작을 수행하는 곳. 비동기 IOCP이벤트 처리(Process)와 등록(Register)을 관리
    + SocketUtils: socket 옵션 설정과 Bind, listen, close를 관리 

  + Thread
    + DeadLockProfiler: dfs 알고리즘을 통해 deadlock을 식별한다. (순환구조 적발)
    + Lock: RWLock을 사용한다. LockGuard를 이용하여 lock을 걸고 해제한다.
    + ThreadManager: ThreadPool에 thread를 launch하고 threadid를 발급한다.

  + Utils
    + BufferReader: buffer를 read하는 로직
    + BufferWriter: buffer에 data를 write하는 로직
