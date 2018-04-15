//
//  ThreadPool.cpp
//  Cube
//
//  Created by Mike Allison on 3/16/16.
//
//

#include "ThreadPool.h"


ThreadPool::~ThreadPool()
{
	close();
}

void ThreadPool::open()
{
	if(mIoService && mWork){
		close();
	}
	
	mIoService = std::shared_ptr<asio::io_service>( new asio::io_service );
	mWork = std::shared_ptr<asio::io_service::work>( new asio::io_service::work( *mIoService ) );
	
	for(size_t t = 0; t < mNumThreads; t++){
		mThreadPool.push_back( std::shared_ptr<asio::thread>( new asio::thread( [&]{ mIoService->run(); } ) ) );
	}
	
}

void ThreadPool::close()
{
	if(!mIoService)return;
	
	if(!mIoService->stopped())
		mIoService->stop();
	
	for( auto & thread : mThreadPool )
		thread->join();
	
	mThreadPool.clear();
	mIoService = nullptr;
	mWork = nullptr;
}

ThreadPool::ThreadPool(int num_threads) : mNumThreads(num_threads)
{
	open();
}
