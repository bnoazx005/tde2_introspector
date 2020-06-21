#include "../include/jobmanager.h"


namespace TDEngine2
{
	JobManager::JobManager(uint32_t maxNumOfThreads):
		mNumOfThreads(maxNumOfThreads)
	{
		mIsRunning = true;

		for (uint32_t i = 0; i < mNumOfThreads; ++i)
		{
			mWorkerThreads.emplace_back(&JobManager::_executeTasksLoop, this);
		}
	}

	JobManager::~JobManager()
	{
		mIsRunning = false;

		mHasNewJobAdded.notify_all();

		// wait for all working threads
		for (std::thread& currThread : mWorkerThreads)
		{
			if (currThread.joinable())
			{
				currThread.join();
			}
		}
	}

	void JobManager::_executeTasksLoop()
	{
		std::unique_ptr<IJob> pJob;

		while (true)
		{
			{
				std::unique_lock<std::mutex> lock(mQueueMutex);

				mHasNewJobAdded.wait(lock, [this] { return !mIsRunning || !mJobs.empty(); });

				if (!mIsRunning && mJobs.empty())
				{
					return;
				}

				pJob = std::move(mJobs.front());

				mJobs.pop();
			}

			(*pJob)();
		}
	}

	void JobManager::_submitJob(std::unique_ptr<IJob> pJob)
	{
		if (!pJob)
		{
			return;
		}

		std::lock_guard<std::mutex> lock(mQueueMutex);

		mJobs.emplace(std::move(pJob));

		mHasNewJobAdded.notify_one();
	}
}