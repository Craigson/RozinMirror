//
//  ThreadPool.hpp
//  Cube
//
//  Created by Mike Allison on 3/16/16.
//
//

#pragma once
#include <asio/asio.hpp>


    
using ThreadPoolRef = std::shared_ptr<class ThreadPool>;

 /**
 * @brief ThreadPool holds a pool of threads available to do async tasks.
 *
 * tasks submitted to the io_service must be thread-safe or wrapped in a strand.
 *
 */

class ThreadPool {
	
public:
	
	//! creates the threadpool with the specified number of threads, defualts to the hardware maximum
	static ThreadPoolRef create( int num_threads = std::thread::hardware_concurrency() ){ return ThreadPoolRef( new ThreadPool(num_threads) ); }
	
	//! returns the service running in the threadpool
	asio::io_service& getIoService(){ return *mIoService; }
	
	//!posts a task to the pool, handler must be thread-safe or wrapped in an \a asio::strand
	template<typename Handler>
	void post_async( const Handler& fn ){ mIoService->post(fn); }
	
	~ThreadPool();
	
	//! terminates the service and joins the threadpool
	void close();
	
	//! opens the service and spins up the pool
	void open();
	
private:
	
	ThreadPool(int num_threads);
	
	int mNumThreads;
	std::shared_ptr<asio::io_service> mIoService;
	std::shared_ptr<asio::io_service::work> mWork;
	std::vector< std::shared_ptr<asio::thread> > mThreadPool;
	
};
