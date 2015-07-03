#include <thread>
#include <chrono>

#include "EasyGameServerLinux.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "DatabaseJobContext.h"
#include "DatabaseJobManager.h"
#include "DbHelper.h"

DatabaseJobManager* GDatabaseJobManager = nullptr ;


void DatabaseJobManager::ExecuteDatabaseJobs()
{
	LThreadType = THREAD_DATABASE ;

	DatabaseJobContext* jobContext = nullptr ;
	while ( true )
	{
		if (mDbJobRequestQueue.PopFront(jobContext))
		{
			/// ���⼭ DBȣ���ؼ� ó���ϰ� 
			jobContext->mSuccess = jobContext->OnExecute();

			/// �� ����� result queue�� ��� ����
			CRASH_ASSERT(mDbJobResultQueue.PushBack(jobContext));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

/// �Ʒ� �Լ��� Ŭ���̾�Ʈ ó�� �����忡�� �ҷ��� �Ѵ�
void DatabaseJobManager::PushDatabaseJobRequest(DatabaseJobContext* jobContext)
{
	CRASH_ASSERT(LThreadType == THREAD_CLIENT);
	CRASH_ASSERT( mDbJobRequestQueue.PushBack(jobContext) );
}

/// �Ʒ� �Լ��� Ŭ���̾�Ʈ ó�� �����忡�� �ҷ��� �Ѵ�
bool DatabaseJobManager::PopDatabaseJobResult(DatabaseJobContext*& jobContext)
{
	CRASH_ASSERT(LThreadType == THREAD_CLIENT);

	/// DB �۾� �Ϸ�� ��ٸ��� �ʴ´�
	return mDbJobResultQueue.PopFront(jobContext) ;
}