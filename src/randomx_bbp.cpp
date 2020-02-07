// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "randomx_bbp.h"
#include "hash.h"
//#include <boost/thread/thread.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>

static std::map<int, randomx_cache*> rxcache;
static std::map<int, randomx_vm*> myvm;
static std::map<int, bool> fInitialized;
static std::map<int, bool> fBusy;
static std::map<int, uint256> msGlobalKey;

void init(uint256 uKey, int iThreadID)
{
	std::vector<unsigned char> hashKey = std::vector<unsigned char>(uKey.begin(), uKey.end());
	randomx_flags flags = randomx_get_flags();
	rxcache[iThreadID] = randomx_alloc_cache(flags);
	randomx_init_cache(rxcache[iThreadID], hashKey.data(), hashKey.size());
	myvm[iThreadID] = randomx_create_vm(flags, rxcache[iThreadID], NULL);
	fInitialized[iThreadID] = true;
	msGlobalKey[iThreadID] = uKey;
}

void destroy(int iThreadID)
{
	randomx_destroy_vm(myvm[iThreadID]);
	randomx_release_cache(rxcache[iThreadID]);
	fInitialized[iThreadID] = false;
	fBusy[iThreadID] = false;
}

void BusyWaitGuard(int iThreadID)
{
	static int MAX_MS = 5000;
	if (!fBusy[iThreadID])
		return;
	// Note 1: When we add the std::unique_lock<std::mutex> in front of the rx vm, we throw all kinds of errors, I believe because each cmake randomx dependency needs re-built with pthreads, boost, and std::mutex.
	// Note 2: When we don't use a unique_lock, the vm throws an illegal instruction if accessed by more than one thread.
	// Note 3: If we create and destroy the vm sequentially, we spend a massive amount of time initializing and de-initializing (IE we receive 1 hashpersec).
	// Note 4: If we create multiple vms, we use a lot of ram, which is not really desirable on a sanctuary, as they may want to run multiple instances.
	// For now, we have an interim solution that seems to be working.  We create multiple threads for miners, and normally, these never interfere with each other due to the threadid.
	// If the thread enters the below busy guard, it just joins and exits and this appears to prevent crashing and this allows 50hps per thread (roughly), which is the actual performance of rx-slowhash.
	// Note, to prevent the core from crashing, I added a std::unique_lock in the rpc everywhere we ask for a hash (since multiple rpc threads ask for hashes).
	// The core block syncer runs on one single thread so it is not guarded.
	// ToDo:  Let's try to compile the randomx submodule with boost and see if we can add a standard mutex in this class as well - and remove this busy wait guard.
	if (fBusy[iThreadID])
	{
		for (int i = 0; i < MAX_MS; i++)
		{
			for (int z = 1; z < 65535; z++)
			{
				// boost::this_thread::sleep(boost::posix_time::millisec(1));
			}
			if (!fBusy[iThreadID]) 
				return;
		}
	}
	fBusy[iThreadID] = false;
}

uint256 RandomX_BBPHash(uint256 hash, uint256 uKey, int iThreadID)
{
		BusyWaitGuard(iThreadID);
						
		if (fInitialized[iThreadID] && msGlobalKey[iThreadID] != uKey)
		{
			destroy(iThreadID);
		}

		if (!fInitialized[iThreadID] || uKey != msGlobalKey[iThreadID])
		{
			init(uKey, iThreadID);
		}
		std::vector<unsigned char> hashIn = std::vector<unsigned char>(hash.begin(), hash.end());
		char *hashOut1 = (char*)malloc(RANDOMX_HASH_SIZE + 1);
		fBusy[iThreadID] = true;
		randomx_calculate_hash(myvm[iThreadID], hashIn.data(), hashIn.size(), hashOut1);
		std::vector<unsigned char> data1(hashOut1, hashOut1 + RANDOMX_HASH_SIZE);
		free(hashOut1);
		fBusy[iThreadID] = false;
		return uint256(data1);
}

uint256 RandomX_BBPHash(std::vector<unsigned char> data0, uint256 uKey, int iThreadID)
{
		BusyWaitGuard(iThreadID);
						
		if (fInitialized[iThreadID] && msGlobalKey[iThreadID] != uKey)
		{
			destroy(iThreadID);
		}

		if (!fInitialized[iThreadID] || uKey != msGlobalKey[iThreadID])
		{
			init(uKey, iThreadID);
		}
		char *hashOut0 = (char*)malloc(RANDOMX_HASH_SIZE + 1);
		fBusy[iThreadID] = true;
		randomx_calculate_hash(myvm[iThreadID], data0.data(), data0.size(), hashOut0);
		std::vector<unsigned char> data1(hashOut0, hashOut0 + RANDOMX_HASH_SIZE);
		free(hashOut0);
		fBusy[iThreadID] = false;
		return uint256(data1);
}


uint256 RandomX_BBPHash(std::vector<unsigned char> data0, std::vector<unsigned char> datakey)
{
	int iThreadID = 101;
	randomx_flags flags = randomx_get_flags();
	rxcache[iThreadID] = randomx_alloc_cache(flags);
	randomx_init_cache(rxcache[iThreadID], datakey.data(), datakey.size());
	myvm[iThreadID] = randomx_create_vm(flags, rxcache[iThreadID], NULL);
	char *hashOut0 = (char*)malloc(RANDOMX_HASH_SIZE + 1);
	randomx_calculate_hash(myvm[iThreadID], data0.data(), data0.size(), hashOut0);
	std::vector<unsigned char> data1(hashOut0, hashOut0 + RANDOMX_HASH_SIZE);
	free(hashOut0);
	randomx_destroy_vm(myvm[iThreadID]);
	randomx_release_cache(rxcache[iThreadID]);
	return uint256(data1);
}



