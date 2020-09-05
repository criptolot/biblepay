#include "pose.h"
#include "clientversion.h"
#include "chainparams.h"
#include "init.h"
#include "net.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#include "masternode-sync.h"


void ThreadPOOS(CConnman& connman)
{

	SyncSideChain(0);
	int nIterations = 0;

	while (1 == 1)
	{
	    if (ShutdownRequested())
			return;

		try
		{
			double nOrphanBanning = GetSporkDouble("EnableOrphanSanctuaryBanning", 0);
			bool fConnectivity = POOSOrphanTest("status", 60 * 60);
			bool fPOOSEnabled = nOrphanBanning == 1 && fConnectivity;
			if (fPOOSEnabled)
			{

				auto mnList = deterministicMNManager->GetListAtChainTip();
				mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) 
				{
					if (!ShutdownRequested())
					{
						std::string sPubKey = dmn->pdmnState->pubKeyOperator.Get().ToString();
						bool fOK = POOSOrphanTest(sPubKey, 60 * 60);
						mapPOOSStatus[sPubKey] = fOK;
						MilliSleep(1000);
					}
				});
			}
			nIterations++;
			int64_t nTipAge = GetAdjustedTime() - chainActive.Tip()->GetBlockTime();
			if (nTipAge < (60 * 60 * 4) && chainActive.Tip()->nHeight % 10 == 0)
			{
				SyncSideChain(chainActive.Tip()->nHeight);
			}

		}
		catch(...)
		{
			LogPrintf("Error encountered in POOS main loop. %f \n", 0);
		}
		int nSleepLength = nIterations < 5 ? 60*5 : 60*15;

		for (int i = 0; i < nSleepLength; i++)
		{
			if (ShutdownRequested())
				break;
			MilliSleep(1000);
		}
	}
}

