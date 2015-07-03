#include <stdio.h>
#include <typeinfo>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <algorithm>

#include "EasyGameServerLinux.h"
#include "PacketType.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "ClientManager.h"
#include "DatabaseJobContext.h"
#include "DatabaseJobManager.h"
#include "Scheduler.h"

ClientManager* GClientManager = nullptr ;


bool ClientManager::Initialize()
{
	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket < 0)
		return false;

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(sockaddr_in));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret = bind(mListenSocket, (sockaddr*)&serveraddr, sizeof(serveraddr));
	if (ret < 0)
		return false;

	/// listen
	ret = listen(mListenSocket, SOMAXCONN);
	if (ret < 0)
		return false;

	mEpollFd = epoll_create(MAX_CONNECTION);
	if (mEpollFd < 0)
		return false;

	epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = mListenSocket;
	ret = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mListenSocket, &ev);
	if (ret < 0)
		return false;

	return true;

}

void ClientManager::EventLoop()
{
	LThreadType = THREAD_CLIENT;
	LScheduler = new Scheduler;

	epoll_event events[MAX_CONNECTION];

	while (true)
	{
		int nfd = epoll_wait(mEpollFd, events, MAX_CONNECTION, POLL_INTERVAL);

		for (int i = 0; i < nfd; ++i)
		{
			if (events[i].data.fd == mListenSocket)
			{
				sockaddr_in clientAddr;
				socklen_t clientAddrLen = sizeof(sockaddr_in);

				SOCKET client = accept(mListenSocket, (sockaddr *)&clientAddr, &clientAddrLen);
				if (client < 0) 
				{
					printf("[DEBUG] accept error: IP=%s, PORT=%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					continue;
				}

				ClientSession* newClient = CreateClient(client);
				newClient->OnConnect(&clientAddr);

				epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLIN | EPOLLET;
				ev.data.ptr = static_cast<void*>(newClient);
				epoll_ctl(mEpollFd, EPOLL_CTL_ADD, client, &ev);
			}
			else
			{
				ClientSession* clientSession = static_cast<ClientSession*>(events[i].data.ptr);
				clientSession->OnReceive();
				
			}
		}

		/// ��Ƽ� ������
		FlushClientSend();

		/// ó�� �Ϸ�� DB �۾��� ������ Client�� dispatch
		DispatchDatabaseJobResults();

		/// scheduled jobs
		LScheduler->DoTasks();
	 
	}

}

ClientSession* ClientManager::CreateClient(SOCKET sock)
{
	CRASH_ASSERT(LThreadType == THREAD_CLIENT);

	ClientSession* client = new ClientSession(sock) ;
	mClientList.insert(ClientList::value_type(sock, client)) ;

	return client ;
}

void ClientManager::DeleteClient(ClientSession* client)
{
	CRASH_ASSERT(LThreadType == THREAD_CLIENT);

	epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	epoll_ctl(mEpollFd, EPOLL_CTL_DEL, client->mSocket, &ev);

	mClientList.erase(client->mSocket);
	delete client;
}


void ClientManager::BroadcastPacket(ClientSession* from, PacketHeader* pkt)
{
	for (ClientList::const_iterator it=mClientList.begin() ; it!=mClientList.end() ; ++it)
	{
		ClientSession* client = it->second ;
		
		if ( from == client )
			continue ;
		
		client->SendRequest(pkt) ;
	}
}

void ClientManager::DispatchDatabaseJobResults()
{
	/// �׿� �ִ� DB �۾� ó�� ������� ������ Ŭ�󿡰� �ѱ��
	DatabaseJobContext* dbResult = nullptr ;
	while ( GDatabaseJobManager->PopDatabaseJobResult(dbResult) )
	{
		if ( false == dbResult->mSuccess )
		{
			printf("DB JOB FAIL \n") ;
		}
		else
		{
			if ( typeid(*dbResult) == typeid(CreatePlayerDataContext) )
			{
				CreatePlayerDone(dbResult) ;
			}
			else if ( typeid(*dbResult) == typeid(DeletePlayerDataContext) )
			{
				DeletePlayerDone(dbResult) ;
			}
			else
			{
				/// ����� �ش� DB��û�� �ߴ� Ŭ���̾�Ʈ���� ���� ����� �� ����
				auto it = mClientList.find(dbResult->mSockKey) ;

				if ( it != mClientList.end() && it->second->IsConnected() )
				{
					/// dispatch here....
					it->second->DatabaseJobDone(dbResult) ;
				}
			}
		}
	
	
		/// �Ϸ�� DB �۾� ���ؽ�Ʈ�� ����������
		DatabaseJobContext* toBeDelete = dbResult ;
		delete toBeDelete ;
	}
}

void ClientManager::FlushClientSend()
{
	CRASH_ASSERT(LThreadType == THREAD_CLIENT);

	for (auto& it : mClientList)
	{
		ClientSession* client = it.second;
		if (false == client->SendFlush())
		{
			client->Disconnect();
		}
	}
}

void ClientManager::CreatePlayer(int pid, double x, double y, double z, const char* name, const char* comment)
{
	CreatePlayerDataContext* newPlayerJob = new CreatePlayerDataContext() ;
	newPlayerJob->mPlayerId = pid ;
	newPlayerJob->mPosX = x ;
	newPlayerJob->mPosY = y ;
	newPlayerJob->mPosZ = z ;
	strcpy(newPlayerJob->mPlayerName, name) ;
	strcpy(newPlayerJob->mComment, comment) ;

	GDatabaseJobManager->PushDatabaseJobRequest(newPlayerJob) ;

}

void ClientManager::DeletePlayer(int pid)
{
	DeletePlayerDataContext* delPlayerJob = new DeletePlayerDataContext(pid) ;
	GDatabaseJobManager->PushDatabaseJobRequest(delPlayerJob) ;
}

void ClientManager::CreatePlayerDone(DatabaseJobContext* dbJob)
{
	CreatePlayerDataContext* createJob = dynamic_cast<CreatePlayerDataContext*>(dbJob) ;

	printf("PLAYER[%d] CREATED: %s \n", createJob->mPlayerId, createJob->mPlayerName) ;
}

void ClientManager::DeletePlayerDone(DatabaseJobContext* dbJob)
{
	DeletePlayerDataContext* deleteJob = dynamic_cast<DeletePlayerDataContext*>(dbJob) ;
	
	printf("PLAYER [%d] DELETED\n", deleteJob->mPlayerId) ;

}

