#pragma once


#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>


namespace TDEngine2
{
	struct IJob
	{
		virtual void operator()() = 0;
	};


	template <typename... TArgs>
	struct TJob : public IJob
	{
		protected:
			typedef std::function<void(TArgs...)> TJobCallback;
			typedef std::tuple<TArgs...>          TArguments;
		public:
			TJob(const TJobCallback& callback, TArgs... args);

			void operator()() override;
		protected:
			TJobCallback mJobCallback;
			TArguments   mArguments;
	};


	template <typename... TArgs>
	TJob<TArgs...>::TJob(const TJobCallback& callback, TArgs... args) :
		mJobCallback(callback), mArguments(args...)
	{
	}

	template <typename... TArgs>
	void TJob<TArgs...>::operator() ()
	{
		mJobCallback(std::get<TArgs>(mArguments)...);
	}


	class JobManager
	{
		protected:
			using TThreadsArray = std::vector<std::thread>;
			using TJobQueue = std::queue<std::unique_ptr<IJob>>;
		public:
			explicit JobManager(uint32_t maxNumOfThreads);
			~JobManager();

			template <typename... TArgs>
			void SubmitJob(std::function<void(TArgs...)> jobCallback, TArgs... args)
			{
				_submitJob(std::make_unique<TJob<TArgs...>>(jobCallback, std::forward<TArgs>(args)...));
			}
		protected:
			void _executeTasksLoop();

			void _submitJob(std::unique_ptr<IJob> pJob);
		protected:
			uint32_t                mNumOfThreads;

			std::atomic<bool>       mIsRunning;

			TThreadsArray           mWorkerThreads;

			mutable std::mutex      mQueueMutex;

			TJobQueue               mJobs;

			std::condition_variable mHasNewJobAdded;
	};
}