#include "pch.h"
#include "DBConnectionPool.h"

/*-------------------
	DBConnectionPool
--------------------*/

DBConnectionPool::DBConnectionPool()
{

}

DBConnectionPool::~DBConnectionPool()
{ // 리소스 정리
	Clear();
}

bool DBConnectionPool::Connect(int32 connectionCount, const WCHAR* connectionString)
{ // 지정된 수의 DB 연결을 생성하여 풀에 추가
	WRITE_LOCK;

	// ODBC 환경 핸들
	if (::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_environment) != SQL_SUCCESS)
		return false;

	// ODBC 버전 설정(3.0 버전)
	if (::SQLSetEnvAttr(_environment, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0) != SQL_SUCCESS)
		return false;

	for (int32 i = 0; i < connectionCount; i++)
	{ // 지정된 수만큼 DBConnection 객체를 생성하고 풀에 저장
		DBConnection* connection = xnew<DBConnection>();
		if (connection->Connect(_environment, connectionString) == false)
			return false; // 실패시 전체 실패

		_connections.push_back(connection);
	}

	return true;
}

void DBConnectionPool::Clear()
{ // 풀에 있는 연결 및 환경 핸들을 해제하고 초기화
	WRITE_LOCK;

	if (_environment != SQL_NULL_HANDLE)
	{ // 환경 핸들 해제
		::SQLFreeHandle(SQL_HANDLE_ENV, _environment);
		_environment = SQL_NULL_HANDLE;
	}

	// 각 연결 객체를 메모리에서 해제
	for (DBConnection* connection : _connections)
		xdelete(connection);

	_connections.clear();
}

DBConnection* DBConnectionPool::Pop()
{ // 커낵션 풀에서 하나의 연결 객체를 가져옴
	WRITE_LOCK;

	if (_connections.empty())
		return nullptr;

	// 가장 뒤에 있는 커낵션을 가져오고 제거
	DBConnection* connection = _connections.back();
	_connections.pop_back();
	return connection;
}

void DBConnectionPool::Push(DBConnection* connection)
{ // 커낵션을 다시 풀에 반환
	WRITE_LOCK;
	_connections.push_back(connection);
}
