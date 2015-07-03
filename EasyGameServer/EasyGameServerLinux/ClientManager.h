#pragma once

#include <map>
#include "EasyGameServerLinux.h"

class ClientSession ;
struct PacketHeader ;
struct DatabaseJobContext ;



class ClientManager
{
public:
	ClientManager() : mLastGCTick(0), mEpollFd(-1), mListenSocket(-1)
	{}

	bool Initialize();


	ClientSession* CreateClient(SOCKET sock) ;
	void DeleteClient(ClientSession* client);
	
	void BroadcastPacket(ClientSession* from, PacketHeader* pkt) ;

	void EventLoop();

	/// DB�� �÷��̾� ������ �����ϰų� �����ϴ� �Լ�
	void CreatePlayer(int pid, double x, double y, double z, const char* name, const char* comment) ;
	void DeletePlayer(int pid) ;

	void FlushClientSend();

	//SOCKET GetEpollFd() const { return mEpollFd; }
private:
	void CreatePlayerDone(DatabaseJobContext* dbJob) ;
	void DeletePlayerDone(DatabaseJobContext* dbJob) ;

private:

	void DispatchDatabaseJobResults() ;
	

private:
	typedef std::map<SOCKET, ClientSession*> ClientList ;
	ClientList	mClientList ;

	int64_t		mLastGCTick ;

	SOCKET		mEpollFd;
	SOCKET		mListenSocket;
} ;

extern ClientManager* GClientManager ;