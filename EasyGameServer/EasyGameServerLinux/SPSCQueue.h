#pragma once

#include <atomic>

template<typename TElem, int QSize>
class SPSCQueue
{
public:
	SPSCQueue() :mHeadPos(0), mTailPos(0) {}
	~SPSCQueue() {}

	/// ������ true, ��á�� ��� false
	bool PushBack(const TElem& item) ;

	/// ������ true, ������� ��� false
	bool PopFront(TElem& item) ;

private:

	std::atomic<int>	mHeadPos ; ///< for pop_front
	std::atomic<int>	mTailPos ; ///< for push_back

	TElem mQueueArray[QSize+1] ;

} ;

template<typename TElem, int QSize>
bool SPSCQueue<TElem, QSize>::PushBack(const TElem& item)
{
	/// ť�� �ڿ��� ����

	int currTailPos = mTailPos.load(std::memory_order_relaxed) ; 

	/// ť�� ������ ���Ҵ� full/empty���θ� ������ ���� �� �������� �س��� ������ QSize+1�Ѵ�
	int nextTailPos = (currTailPos + 1) % (QSize + 1) ; 

	if ( nextTailPos == mHeadPos.load(std::memory_order_acquire) )
	{
		/// tail+1 == head�� ����̹Ƿ� ť ��á��
		return false ;
	}

	mQueueArray[currTailPos] = item ;
	mTailPos.store(nextTailPos, std::memory_order_release) ; 
	
	return true ;
}

template<typename TElem, int QSize>
bool SPSCQueue<TElem, QSize>::PopFront(TElem& item)
{
	int currHeadPos = mHeadPos.load(std::memory_order_relaxed) ;

	if ( currHeadPos == mTailPos.load(std::memory_order_acquire) ) 
	{
		/// head == tail�� ����̹Ƿ� ť�� ������ ���� 
		return false ;
	}
	
	item = mQueueArray[currHeadPos] ;

	///  push������ ���� ������..  QSize+1 ���ִ°���
	int nextHeadPos = (currHeadPos + 1) % (QSize + 1) ;

	mHeadPos.store(nextHeadPos, std::memory_order_release) ; 

	return true ;
}
