// Copyright (c) 2014-2019 The Dash Core Developers, The BiblePay Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "spork.h"
#include "util.h"
#include "utilmoneystr.h"
#include "rpcpodc.h"
#include "init.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "governance-classes.h"
#include "masternode-sync.h"
#include "masternode-payments.h"
#include "masternodeconfig.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/algorithm/string.hpp> // for trim()
#include <boost/date_time/posix_time/posix_time.hpp> // for StringToUnixTime()
#include <math.h>       /* round, floor, ceil, trunc */
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <openssl/crypto.h>
#include <stdint.h>
#include <univalue.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <openssl/md5.h>

// CRITICAL TODO:  Add extern CWallet* pwalletMain; reference for v1.0

/*
std::string GenerateNewAddress(std::string& sError, std::string sName)
{
    LOCK2(cs_main, pwalletMain->cs_wallet);
	{
		if (!pwalletMain->IsLocked(true))
			pwalletMain->TopUpKeyPool();
		// Generate a new key that is added to wallet
		CPubKey newKey;
		if (!pwalletMain->GetKeyFromPool(newKey))
		{
			sError = "Keypool ran out, please call keypoolrefill first";
			return "";
		}
		CKeyID keyID = newKey.GetID();
		pwalletMain->SetAddressBook(keyID, sName, "receive"); //receive == visible in address book, hidden = non-visible
		LogPrintf(" created new address %s ", CBitcoinAddress(keyID).ToString().c_str());
		return CBitcoinAddress(keyID).ToString();
	}
}
*/

std::string RoundToString(double d, int place)
{
	std::ostringstream ss;
    ss << std::fixed << std::setprecision(place) << d ;
    return ss.str() ;
}

double Round(double d, int place)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(place) << d ;
	double r = boost::lexical_cast<double>(ss.str());
	return r;
}

double cdbl(std::string s, int place)
{
	if (s=="") s = "0";
	if (s.length() > 255) return 0;
	s = strReplace(s,"\r","");
	s = strReplace(s,"\n","");
	std::string t = "";
	for (int i = 0; i < (int)s.length(); i++)
	{
		std::string u = s.substr(i,1);
		if (u=="0" || u=="1" || u=="2" || u=="3" || u=="4" || u=="5" || u=="6" || u == "7" || u=="8" || u=="9" || u=="." || u=="-") 
		{
			t += u;
		}
	}

    double r = boost::lexical_cast<double>(t);
	double d = Round(r,place);
	return d;
}

bool Contains(std::string data, std::string instring)
{
	std::size_t found = 0;
	found = data.find(instring);
	if (found != std::string::npos) return true;
	return false;
}

std::string GetSporkValue(std::string sKey)
{
	boost::to_upper(sKey);
    const std::string key = "SPORK;" + sKey;
    const std::string& value = mvApplicationCache[key];
	return value;
}

double GetSporkDouble(std::string sName, double nDefault)
{
	double dSetting = cdbl(GetSporkValue(sName), 2);
	if (dSetting == 0) return nDefault;
	return dSetting;
}

/*

CAmount AmountFromValue(const UniValue& value)
{
    if (!value.isNum() && !value.isStr()) return 0;
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), 8, &amount)) return 0;
    if (!MoneyRange(amount)) return 0;
    return amount;
}

*/

/*
UniValue GetBusinessObject(std::string sType, std::string sPrimaryKey, std::string& sError)
{
	UniValue o(UniValue::VOBJ);
	std::string sIPFSHash = ReadCache(sType, sPrimaryKey);

	if (sIPFSHash.empty()) 
	{
		o.push_back(Pair("objecttype", "0"));
		sError = "Object not found";
		return o;
	}
	std::string sJson = GetJSONFromIPFS(sIPFSHash, sError);
	if (!sError.empty()) return o;
	try  
	{
	    UniValue o(UniValue::VOBJ);
	    o.read(sJson);
		LogPrintf("objecttype %s ",o["objecttype"].get_str().c_str());
		return o;
    }
    catch(std::exception& e) 
	{
        std::ostringstream ostr;
        ostr << "BusinessObject::LoadData Error parsing JSON" << ", e.what() = " << e.what();
        DBG( cout << ostr.str() << endl; );
        LogPrintf( ostr.str().c_str() );
        sError = e.what();
		return o;
    }
    catch(...) 
	{
        std::ostringstream ostr;
        ostr << "BusinessObject::LoadData Unknown Error parsing JSON";
        DBG( cout << ostr.str() << endl; );
        LogPrintf( ostr.str().c_str() );
		sError = "Unknown error while parsing JSON business object";
		return o;
    }
	return o;
}
*/

std::string ReadCache(std::string sSection, std::string sKey)
{
	boost::to_upper(sSection);
	boost::to_upper(sKey);
	if (sSection.empty() || sKey.empty()) return "";
	std::string sValue = mvApplicationCache[sSection + ";" + sKey];
	return sValue;
}

std::string TimestampToHRDate(double dtm)
{
	if (dtm == 0) return "1-1-1970 00:00:00";
	if (dtm > 9888888888) return "1-1-2199 00:00:00";
	std::string sDt = DateTimeStrFormat("%m-%d-%Y %H:%M:%S",dtm);
	return sDt;
}

std::string ExtractXML(std::string XMLdata, std::string key, std::string key_end)
{
	std::string extraction = "";
	std::string::size_type loc = XMLdata.find( key, 0 );
	if( loc != std::string::npos )
	{
		std::string::size_type loc_end = XMLdata.find( key_end, loc+3);
		if (loc_end != std::string::npos )
		{
			extraction = XMLdata.substr(loc+(key.length()),loc_end-loc-(key.length()));
		}
	}
	return extraction;
}


std::string AmountToString(const CAmount& amount)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
	std::string sAmount = strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder);
	return sAmount;
}

static CBlockIndex* pblockindexFBBHLast;
CBlockIndex* FindBlockByHeight(int nHeight)
{
    CBlockIndex *pblockindex;
	if (nHeight < 0 || nHeight > chainActive.Tip()->nHeight) return NULL;

    if (nHeight < chainActive.Tip()->nHeight / 2)
		pblockindex = mapBlockIndex[chainActive.Genesis()->GetBlockHash()];
    else
        pblockindex = chainActive.Tip();
    if (pblockindexFBBHLast && abs(nHeight - pblockindex->nHeight) > abs(nHeight - pblockindexFBBHLast->nHeight))
        pblockindex = pblockindexFBBHLast;
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;
    while (pblockindex->nHeight < nHeight)
		pblockindex = chainActive.Next(pblockindex);
    pblockindexFBBHLast = pblockindex;
    return pblockindex;
}

std::vector<std::string> Split(std::string s, std::string delim)
{
	size_t pos = 0;
	std::string token;
	std::vector<std::string> elems;
	while ((pos = s.find(delim)) != std::string::npos)
	{
		token = s.substr(0, pos);
		elems.push_back(token);
		s.erase(0, pos + delim.length());
	}
	elems.push_back(s);
	return elems;
}

/*
std::string DefaultRecAddress(std::string sType)
{
	std::string sDefaultRecAddress = "";
	BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CAddressBookData)& item, pwalletMain->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        std::string strName = item.second.name;
        bool fMine = IsMine(*pwalletMain, address.Get());
        if (fMine)
		{
		    sDefaultRecAddress=CBitcoinAddress(address).ToString();
			boost::to_upper(strName);
			boost::to_upper(sType);
			if (strName == sType) 
			{
				sDefaultRecAddress=CBitcoinAddress(address).ToString();
				return sDefaultRecAddress;
			}
		}
    }

	// IPFS-PODS - R ANDREWS - One biblepay public key is associated with each type of signed business object
	if (!sType.empty())
	{
		std::string sError = "";
		sDefaultRecAddress = GenerateNewAddress(sError, sType);
		if (sError.empty()) return sDefaultRecAddress;
	}
	
	return sDefaultRecAddress;
}
*/

/*
std::string CreateBankrollDenominations(double nQuantity, CAmount denominationAmount, std::string& sError)
{
	// First mark the denominations with the 1milliBBP TitheMarker (this saves them from being spent in PODC Updates):
	denominationAmount += ((.001) * COIN);
	CAmount nBankrollMask = .001 * COIN;

	CAmount nTotal = denominationAmount * nQuantity;

	CAmount curBalance = pwalletMain->GetUnlockedBalance();
	if (curBalance < nTotal)
	{
		sError = "Insufficient funds (Unlock Wallet).";
		return "";
	}
	std::string sTitheAddress = DefaultRecAddress("TITHES");
	CBitcoinAddress cbAddress(sTitheAddress);
	CWalletTx wtx;
	
    CScript scriptPubKey = GetScriptForDestination(cbAddress.Get());
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
	for (int i = 0; i < nQuantity; i++)
	{
		bool fSubtractFeeFromAmount = false;
	    CRecipient recipient = {scriptPubKey, denominationAmount, false, fSubtractFeeFromAmount, false, false, false, "", "", "", ""};
		recipient.Message = "";
		vecSend.push_back(recipient);
	}
	
	bool fUseInstantSend = false;
	double minCoinAge = 0;
    if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, sError, NULL, true, ONLY_NOT1000IFMN, fUseInstantSend, 0, minCoinAge, 0, nBankrollMask)) 
	{
		if (!sError.empty())
		{
			return "";
		}

        if (nTotal + nFeeRequired > pwalletMain->GetBalance())
		{
            sError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
			return "";
		}
    }
    if (!pwalletMain->CommitTransaction(wtx, reservekey, fUseInstantSend ? NetMsgType::TXLOCKREQUEST : NetMsgType::TX))
    {
		sError = "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
		return "";
	}

	std::string sTxId = wtx.GetHash().GetHex();
	return sTxId;
}
*/


/*
CAmount GetTitheTotal(CTransaction tx)
{
	CAmount nTotal = 0;
	const Consensus::Params& consensusParams = Params().GetConsensus();
    
	for (int i=0; i < (int)tx.vout.size(); i++)
	{
 		std::string sRecipient = PubKeyToAddress(tx.vout[i].scriptPubKey);
		if (sRecipient == consensusParams.FoundationAddress)
		{ 
			nTotal += tx.vout[i].nValue;
		}
	 }
	 return nTotal;
}
*/

std::string RetrieveMd5(std::string s1)
{
	try
	{
		const char* chIn = s1.c_str();
		unsigned char digest2[16];
		MD5((unsigned char*)chIn, strlen(chIn), (unsigned char*)&digest2);
		char mdString2[33];
		for(int i = 0; i < 16; i++) sprintf(&mdString2[i*2], "%02x", (unsigned int)digest2[i]);
 		std::string xmd5(mdString2);
		return xmd5;
	}
    catch (std::exception &e)
	{
		return "";
	}
}


std::string PubKeyToAddress(const CScript& scriptPubKey)
{
	CTxDestination address1;
    ExtractDestination(scriptPubKey, address1);
    CBitcoinAddress address2(address1);
    return address2.ToString();
}    

/*
void GetTxTimeAndAmountAndHeight(uint256 hashInput, int hashInputOrdinal, int64_t& out_nTime, CAmount& out_caAmount, int& out_height)
{
	CTransaction tx1;
	uint256 hashBlock1;
	if (GetTransaction(hashInput, tx1, Params().GetConsensus(), hashBlock1, true))
	{
		out_caAmount = tx1.vout[hashInputOrdinal].nValue;
		BlockMap::iterator mi = mapBlockIndex.find(hashBlock1);
		if (mi != mapBlockIndex.end())
		{
			CBlockIndex* pindexHistorical = mapBlockIndex[hashBlock1];              
			out_nTime = pindexHistorical->GetBlockTime();
			out_height = pindexHistorical->nHeight;
			return;
		}
		else
		{
			LogPrintf("\nUnable to find hashBlock %s", hashBlock1.GetHex().c_str());
		}
	}
	else
	{
		LogPrintf("\nUnable to find hashblock1 in GetTransaction %s ",hashInput.GetHex().c_str());
	}
}
*/

/*
bool IsTitheLegal(CTransaction ctx, CBlockIndex* pindex, CAmount tithe_amount)
{
	// R ANDREWS - BIBLEPAY - 12/19/2018 
	// We quote the difficulty params to the user as of the best block hash, so when we check a tithe to be legal, we must check it as of the prior block's difficulty params
	if (pindex==NULL || pindex->pprev==NULL || pindex->nHeight < 2) return false;

	uint256 hashInput = ctx.vin[0].prevout.hash;
	int hashInputOrdinal = ctx.vin[0].prevout.n;
	int64_t nTxTime = 0;
	CAmount caAmount = 0;
	int iHeight = 0;
	GetTxTimeAndAmountAndHeight(hashInput, hashInputOrdinal, nTxTime, caAmount, iHeight);
	
	double nTitheAge = (double)((pindex->GetBlockTime() - nTxTime) / 86400);
	if (nTitheAge >= pindex->pprev->nMinCoinAge && caAmount >= pindex->pprev->nMinCoinAmount && tithe_amount <= pindex->pprev->nMaxTitheAmount)
	{
		return true;
	}
	
	return false;
}
*/

/*
double GetTitheAgeAndSpentAmount(CTransaction ctx, CBlockIndex* pindex, CAmount& spentAmount)
{
	// R ANDREWS - BIBLEPAY - 1/4/2018 
	if (pindex==NULL || pindex->pprev==NULL || pindex->nHeight < 2) return false;
	uint256 hashInput = ctx.vin[0].prevout.hash;
	int hashInputOrdinal = ctx.vin[0].prevout.n;
	int64_t nTxTime = 0;
	int iHeight = 0;
	GetTxTimeAndAmountAndHeight(hashInput, hashInputOrdinal, nTxTime, spentAmount, iHeight);
	double nTitheAge = R2X((double)(pindex->GetBlockTime() - nTxTime) / 86400);
	return nTitheAge;
}
*/

/*
bool RPCSendMoney(std::string& sError, const CTxDestination &address, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, 
	bool fUseInstantSend=false, bool fUsePrivateSend=false, bool fUseSanctuaryFunds=false, double nMinCoinAge = 0, CAmount nMinCoinAmount = 0, std::string sSpecificTxId = "", int nSpecificOutput = 0)
{
    CAmount curBalance = pwalletMain->GetBalance();

    // Check amount
    if (nValue <= 0)
	{
        sError = "Invalid amount";
		return false;
	}

    if (nValue > curBalance)
	{
		sError = "Insufficient funds";
		return false;
	}
    // Parse Biblepay address
    CScript scriptPubKey = GetScriptForDestination(address);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::string strError = "";
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
	bool fForce = false;
    CRecipient recipient = {scriptPubKey, nValue, fForce, fSubtractFeeFromAmount, false, false, false, "", "", "", ""};
	vecSend.push_back(recipient);
	
    int nMinConfirms = 0;
	CAmount nBankrollMask = 0;
    if (!pwalletMain->CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, 
		fUsePrivateSend ? ONLY_DENOMINATED : (fUseSanctuaryFunds ? ALL_COINS : ONLY_NOT1000IFMN), fUseInstantSend, nMinConfirms, nMinCoinAge, nMinCoinAmount, nBankrollMask, sSpecificTxId, nSpecificOutput)) 
	{
        if (!fSubtractFeeFromAmount && nValue + nFeeRequired > pwalletMain->GetBalance())
		{
            sError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
			// The Snat - Reported this can crash POG on 12-30-2018 (resolved by handling this)
			return false;
		}
		sError = "Unable to Create Transaction: " + strError;
		return false;
    }
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey, fUseInstantSend ? NetMsgType::TXLOCKREQUEST : NetMsgType::TX))
	{
        sError = "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
		return false;
	}
	return true;
}
*/


/*

void RPCSendMoneyToDestinationWithMinimumBalance(const CTxDestination& address, CAmount nValue, CAmount nMinimumBalanceRequired, 
	double dMinCoinAge, CAmount caMinCoinValue, std::string sSpecificTxId, int nSpecificOutput,	CWalletTx& wtxNew, std::string& sError)
{
	if (pwalletMain->IsLocked())
	{
		sError = "Wallet Unlock Required";
		return;
	}
   
    if (pwalletMain->GetBalance() < nMinimumBalanceRequired || nValue > pwalletMain->GetBalance()) 
	{
		sError = "Insufficient funds";
		return;
	}
    RPCSendMoney(sError, address, nValue, false, wtxNew, false, false, false, dMinCoinAge, caMinCoinValue, sSpecificTxId, nSpecificOutput);
}

*/


/*
std::string SendTithe(CAmount caTitheAmount, double dMinCoinAge, CAmount caMinCoinAmount, CAmount caMaxTitheAmount, std::string sSpecificTxId, int nSpecificOutput, std::string& sError)
{
	const Consensus::Params& consensusParams = Params().GetConsensus();
    std::string sAddress = consensusParams.FoundationAddress;
	if (caMaxTitheAmount == (50*COIN))
	{
		sAddress = consensusParams.FoundationPODSAddress; // Special Rule for Testnet never to be encountered
	}
    CBitcoinAddress address(sAddress);
    if (!address.IsValid())
	{
		sError = "Invalid Destination Address";
		return sError;
	}

	if (fPOGEnabled && caTitheAmount > caMaxTitheAmount)
	{
		sError = "Sorry, your tithe exceeds the maximum allowed tithe for this difficulty level.  (See getmininginfo).";
		return sError;
	}
			
    CWalletTx wtx;
	std::string sTitheAddress = DefaultRecAddress("TITHES");
	
	// R ANDREWS - BIBLEPAY - HONOR UNLOCK PASSWORD IF WE HAVE ONE

	bool bTriedToUnlock = false;
	if (pwalletMain->IsLocked())
	{
		if (!msEncryptedString.empty())
		{
			bTriedToUnlock = true;
			if (!pwalletMain->Unlock(msEncryptedString, false))
			{
				static int nNotifiedOfUnlockIssue = 0;
				if (nNotifiedOfUnlockIssue == 0)
					LogPrintf("\nUnable to unlock wallet with SecureString.\n");
				nNotifiedOfUnlockIssue++;
				sError = "Unable to unlock wallet with POG password provided";
				return sError;
			}
		}
		else LogPrintf(" Encrypted string empty.");
	}

	// Allow tithes of 10BBP or less to send with any coin age in TestNet:
	if (caTitheAmount <= (10*COIN) && !fProd) dMinCoinAge = 0;
	// End of TestNet Rule

    RPCSendMoneyToDestinationWithMinimumBalance(address.Get(), caTitheAmount, caTitheAmount, dMinCoinAge, caMinCoinAmount, sSpecificTxId, nSpecificOutput, wtx, sError);
	
	if (bTriedToUnlock) pwalletMain->Lock();
	if (!sError.empty()) return "";
    return wtx.GetHash().GetHex().c_str();
}
*/

/*
CAmount GetTitheCap(const CBlockIndex* pindexLast)
{
	// NOTE: We must call GetTitheCap with nHeight because some calls from RPC look into the future - and there is no block index yet for the future - our GetBlockSubsidy figures the deflation harmlessly however
    const Consensus::Params& consensusParams = Params().GetConsensus();
	int nBits = 486585255;  // Set diff at about 1.42 for Superblocks (This preserves compatibility with our getgovernanceinfo cap)
	// The first call to GetBlockSubsidy calculates the future reward (and this has our standard deflation of 19% per year in it)
	if (pindexLast == NULL || pindexLast->nHeight < 2) return 0;
    CAmount nSuperblockPartOfSubsidy = GetBlockSubsidy(pindexLast, nBits, pindexLast->nHeight, consensusParams, true);
	// R ANDREWS - POG - 12/6/2018 - CRITICAL - If we go with POG + PODC, the Tithe Cap must be lower to ensure miner profitability
	// TestNet : POG+POBH with PODC enabled  = (100.5K miner payments, 50K daily pogpool tithe cap, deflating) = 0.003125 (4* the blocks per day in testnet)
	// Prod    : POG+POBH with PODC enabled  = (100.5K miner payments, 50K daily pogpool tithe cap, deflating) = 0.00075
    CAmount nPaymentsLimit = 0;
	double nTitheCapFactor = GetSporkDouble("tithecapfactor", 1);
	if (fProd)
	{
		if (PODCEnabled(pindexLast->nHeight))
		{
			nPaymentsLimit = nSuperblockPartOfSubsidy * consensusParams.nSuperblockCycle * .00075 * nTitheCapFactor; // Half of monthly charity budget - with deflation - per day
		}
		else
		{
			nPaymentsLimit = nSuperblockPartOfSubsidy * consensusParams.nSuperblockCycle * .005 * nTitheCapFactor; // Half of monthly charity budget - with deflation - per day
		}
	}
	else
	{
		nPaymentsLimit = nSuperblockPartOfSubsidy * consensusParams.nSuperblockCycle * .003125 * nTitheCapFactor; // Half of monthly charity budget - with deflation - per day
	}
	return nPaymentsLimit;
}
*/

CAmount R20(CAmount amount)
{
	double nAmount = amount / COIN; 
	nAmount = nAmount + 0.5 - (nAmount < 0); 
	int iAmount = (int)nAmount;
	return (iAmount * COIN);
}

double R2X(double var) 
{ 
    double value = (int)(var * 100 + .5); 
    return (double)value / 100; 
} 

double Quantize(double nFloor, double nCeiling, double nValue)
{
	double nSpan = nCeiling - nFloor;
	double nLevel = nSpan * nValue;
	double nOut = nFloor + nLevel;
	if (nOut > std::max(nFloor, nCeiling)) nOut = std::max(nFloor, nCeiling);
	if (nOut < std::min(nCeiling, nFloor)) nOut = std::min(nCeiling, nFloor);
	return nOut;
}

int GetHeightByEpochTime(int64_t nEpoch)
{
	if (!chainActive.Tip()) return 0;
	int nLast = chainActive.Tip()->nHeight;
	if (nLast < 1) return 0;
	for (int nHeight = nLast; nHeight > 0; nHeight--)
	{
		CBlockIndex* pindex = FindBlockByHeight(nHeight);
		if (pindex)
		{
			int64_t nTime = pindex->GetBlockTime();
			if (nEpoch > nTime) return nHeight;
		}
	}
	return -1;
}

/*

std::string GetActiveProposals()
{
	std::string strType = "proposals";
    int nStartTime = GetAdjustedTime() - (86400 * 32);
    LOCK2(cs_main, governance.cs);
    std::vector<CGovernanceObject*> objs = governance.GetAllNewerThan(nStartTime);

	std::string sXML = "";
	int id = 0;
	std::string sDelim = "|";
	std::string sZero = "\0";
	int nLastSuperblock = 0;
	int nNextSuperblock = 0;
	GetGovSuperblockHeights(nNextSuperblock, nLastSuperblock);


    BOOST_FOREACH(CGovernanceObject* pGovObj, objs)
    {
		if(strType == "proposals" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_PROPOSAL) continue;
        if(strType == "triggers" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) continue;
        if(strType == "watchdogs" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_WATCHDOG) continue;
		UniValue obj = pGovObj->GetJSONObject();
		std::string sHash = pGovObj->GetHash().GetHex();
			
		int64_t nEpoch = (int64_t)cdbl(obj["end_epoch"].get_str(), 0);
		int nEpochHeight = GetHeightByEpochTime(nEpoch);

		// First ensure the proposals gov height has not passed yet
		bool bIsPaid = nEpochHeight < nLastSuperblock;
		if (!bIsPaid)
		{
			int iYes = pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING);
			int iNo = pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING);
			int iAbstain = pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING);
			id++;
			// Attempt to retrieve the expense type from the JSON object
			std::string sCharityType = "";
			try
			{
				sCharityType = obj["expensetype"].get_str();
			}
			catch (const std::runtime_error& e) 
			{
				sCharityType = "NA";
			}
   			if (sCharityType.empty()) sCharityType = "N/A";
			std::string sEndEpoch = obj["end_epoch"].get_str();
			if (!sEndEpoch.empty())
			{
				std::string sProposalTime = TimestampToHRDate(cdbl(sEndEpoch,0));
				std::string sURL = obj["url"].get_str();
				if (id == 1) sURL += "&t=" + RoundToString(GetAdjustedTime(), 0);
				// proposal_hashes
				std::string sRow = "<proposal>" + sHash + sDelim 
					+ obj["name"].get_str() + sDelim 
					+ obj["payment_amount"].get_str() + sDelim
					+ sCharityType + sDelim
					+ sProposalTime + sDelim
					+ RoundToString(iYes,0) + sDelim
					+ RoundToString(iNo,0) + sDelim + RoundToString(iAbstain,0) 
					+ sDelim + sURL;
				sXML += sRow;
			}
		}
	}
	return sXML;
}
*/

/*
NOTES: Evolution cant find ProcessVoteAndRelay - Research

bool VoteManyForGobject(std::string govobj, std::string strVoteSignal, std::string strVoteOutcome, 
	int iVotingLimit, int& nSuccessful, int& nFailed, std::string& sError)
{
        uint256 hash(uint256S(govobj));
		vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
		if(eVoteSignal == VOTE_SIGNAL_NONE) 
		{
			sError = "Invalid vote signal (funding).";
			return false;
		}

        vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
        if(eVoteOutcome == VOTE_OUTCOME_NONE) 
		{
            sError = "Invalid vote outcome (yes/no/abstain)";
			return false;
	    }

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();
        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) 
		{
            std::string strError;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if(!darkSendSigner.GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode))
			{
                nFailed++;
                sError += "Masternode signing error, could not set key correctly. (" + mne.getAlias() + ")";
                continue;
            }

            uint256 nTxHash;
            nTxHash.SetHex(mne.getTxHash());

            int nOutputIndex = 0;
            if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) 
			{
                continue;
            }

            CTxIn vin(COutPoint(nTxHash, nOutputIndex));

            CMasternode mn;
            bool fMnFound = mnodeman.Get(vin, mn);

            if(!fMnFound) 
			{
                nFailed++;
				sError += "Can't find masternode by collateral output (" + mne.getAlias() + ")";
                continue;
            }

            CGovernanceVote vote(mn.vin, hash, eVoteSignal, eVoteOutcome);
            if(!vote.Sign(keyMasternode, pubKeyMasternode))
			{
                nFailed++;
				sError += "Failure to sign. (" + mne.getAlias() + ")";
                continue;
            }

            CGovernanceException exception;
            if(governance.ProcessVoteAndRelay(vote, exception)) 
			{
                nSuccessful++;
				if (iVotingLimit > 0 && nSuccessful >= iVotingLimit) break;
            }
            else 
			{
                nFailed++;
				sError += "Error (" + exception.GetMessage() + ")";
            }

        }

     	return (nSuccessful > 0) ? true : false;
}
*/


/*
bool AmIMasternode()
{
	 //if (!fMasterNode) return false;
	 CMasternode mn;
     if(mnodeman.Get(activeMasternode.vin, mn)) 
	 {
         CBitcoinAddress mnAddress(mn.pubKeyCollateralAddress.GetID());
		 if (mnAddress.IsValid()) return true;
     }
	 return false;
}
*/


/*
std::string CreateGovernanceCollateral(uint256 GovObjHash, CAmount caFee, std::string& sError)
{
	CWalletTx wtx;
	if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, GovObjHash, caFee, false)) 
	{
		sError = "Error creating collateral transaction for governance object.  Please check your wallet balance and make sure your wallet is unlocked.";
		return "";
	}
	if (sError.empty())
	{
		// -- make our change address
		CReserveKey reservekey(pwalletMain);
		pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX);
		DBG( cout << "gobject: prepare "
					<< " strData = " << govobj.GetDataAsString()
					<< ", hash = " << govobj.GetHash().GetHex()
					<< ", txidFee = " << wtx.GetHash().GetHex()
					<< endl; );
		return wtx.GetHash().ToString();
	}
	return "";
}
*/

int GetNextSuperblock()
{
	int nLastSuperblock, nNextSuperblock;
    // Get current block height
    int nBlockHeight = 0;
    {
        LOCK(cs_main);
        nBlockHeight = (int)chainActive.Height();
    }

    // Get chain parameters
    int nSuperblockStartBlock = Params().GetConsensus().nSuperblockStartBlock;
    int nSuperblockCycle = Params().GetConsensus().nSuperblockCycle;

    // Get first superblock
    int nFirstSuperblockOffset = (nSuperblockCycle - nSuperblockStartBlock % nSuperblockCycle) % nSuperblockCycle;
    int nFirstSuperblock = nSuperblockStartBlock + nFirstSuperblockOffset;

    if(nBlockHeight < nFirstSuperblock)
	{
        nLastSuperblock = 0;
        nNextSuperblock = nFirstSuperblock;
    }
	else 
	{
        nLastSuperblock = nBlockHeight - nBlockHeight % nSuperblockCycle;
        nNextSuperblock = nLastSuperblock + nSuperblockCycle;
    }
	return nNextSuperblock;
}

/*
std::string StoreBusinessObjectWithPK(UniValue& oBusinessObject, std::string& sError)
{
	std::string sJson = oBusinessObject.write(0,0);
	std::string sPK = oBusinessObject["primarykey"].getValStr();
	std::string sAddress = DefaultRecAddress(BUSINESS_OBJECTS);
	std::string sSignKey = oBusinessObject["signingkey"].getValStr();
	if (sSignKey.empty()) sSignKey = sAddress;
	std::string sOT = oBusinessObject["objecttype"].getValStr();
	std::string sSecondaryKey = oBusinessObject["secondarykey"].getValStr();
	std::string sIPFSHash = SubmitBusinessObjectToIPFS(sJson, sError);
	if (sError.empty())
	{
		double dStorageFee = 1;
		std::string sTxId = "";
		sTxId = SendBusinessObject(sOT, sPK + sSecondaryKey, sIPFSHash, dStorageFee, sSignKey, true, sError);
		WriteCache(sPK, sSecondaryKey, sIPFSHash, GetAdjustedTime());
		return sTxId;
	}
	return "";
}
*/


std::string ReadCacheWithMaxAge(std::string sSection, std::string sKey, int64_t nMaxAge)
{
	// This allows us to disregard old cache messages
	std::string sValue = ReadCache(sSection, sKey);
	std::string sFullKey = sSection + ";" + sKey;
	boost::to_upper(sFullKey);
	int64_t nTime = mvApplicationCacheTimestamp[sFullKey];
	if (nTime==0)
	{
		// LogPrintf(" ReadCacheWithMaxAge key %s timestamp 0 \n",sFullKey);
	}
	int64_t nAge = GetAdjustedTime() - nTime;
	if (nAge > nMaxAge) return "";
	return sValue;
}

bool LogLimiter(int iMax1000)
{
	 //The lower the level, the less logged
	 int iVerbosityLevel = rand() % 1000;
	 if (iVerbosityLevel < iMax1000) return true;
	 return false;
}

bool is_email_valid(const std::string& e)
{
	return (Contains(e, "@") && Contains(e,".") && e.length() > MINIMUM_EMAIL_LENGTH) ? true : false;
}

/*

std::string StoreBusinessObject(UniValue& oBusinessObject, std::string& sError)
{
	std::string sJson = oBusinessObject.write(0,0);
	std::string sAddress = DefaultRecAddress(BUSINESS_OBJECTS);
	std::string sPrimaryKey = oBusinessObject["objecttype"].getValStr();
	std::string sSecondaryKey = oBusinessObject["secondarykey"].getValStr();
	std::string sIPFSHash = SubmitBusinessObjectToIPFS(sJson, sError);
	if (sError.empty())
	{
		double dStorageFee = 1;
		std::string sTxId = "";
		sTxId = SendBusinessObject(sPrimaryKey, sAddress + sSecondaryKey, sIPFSHash, dStorageFee, sAddress, true, sError);
		return sTxId;
	}
	return "";
}
*/


/*
UniValue GetBusinessObjectList(std::string sType)
{
	UniValue ret(UniValue::VOBJ);
	boost::to_upper(sType);
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
	   	if (sKey.length() > sType.length())
		{
			if (sKey.substr(0,sType.length()) == sType)
			{
				std::string sPrimaryKey = sKey.substr(sType.length() + 1, sKey.length() - sType.length());
				std::string sIPFSHash = mvApplicationCache[ii.first];
				std::string sError = "";
				UniValue o = GetBusinessObject(sType, sPrimaryKey, sError);
				if (o.size() > 0)
				{
					bool fDeleted = o["deleted"].getValStr() == "1";
					if (!fDeleted)
							ret.push_back(Pair(sPrimaryKey + " (" + sIPFSHash + ")", o));
				}
			}
		}
	}
	return ret;
}
*/

/*

double GetBusinessObjectTotal(std::string sType, std::string sFieldName, int iAggregationType)
{
	UniValue ret(UniValue::VOBJ);
	boost::to_upper(sType);
	double dTotal = 0;
	double dTotalRows = 0;
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
	   	if (sKey.length() > sType.length())
		{
			if (sKey.substr(0,sType.length())==sType)
			{
				std::string sPrimaryKey = sKey.substr(sType.length() + 1, sKey.length() - sType.length());
				std::string sIPFSHash = mvApplicationCache[ii.first];
				std::string sError = "";
				UniValue o = GetBusinessObject(sType, sPrimaryKey, sError);
				if (o.size() > 0)
				{
					bool fDeleted = o["deleted"].getValStr() == "1";
					if (!fDeleted)
					{
						std::string sBOValue = o[sFieldName].getValStr();
						double dBOValue = cdbl(sBOValue, 2);
						dTotal += dBOValue;
						dTotalRows++;
					}
				}
			}
		}
	}
	if (iAggregationType == 1) return dTotal;
	double dAvg = 0;
	if (dTotalRows > 0) dAvg = dTotal / dTotalRows;
	if (iAggregationType == 2) return dAvg;
	return 0;
}




UniValue GetBusinessObjectByFieldValue(std::string sType, std::string sFieldName, std::string sSearchValue)
{
	UniValue ret(UniValue::VOBJ);
	boost::to_upper(sType);
	boost::to_upper(sSearchValue);
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
	   	if (sKey.length() > sType.length())
		{
			if (sKey.substr(0,sType.length())==sType)
			{
				std::string sPrimaryKey = sKey.substr(sType.length() + 1, sKey.length() - sType.length());
				std::string sIPFSHash = mvApplicationCache[ii.first];
				std::string sError = "";
				UniValue o = GetBusinessObject(sType, sPrimaryKey, sError);
				if (o.size() > 0)
				{
					bool fDeleted = o["deleted"].getValStr() == "1";
					if (!fDeleted)
					{
						std::string sBOValue = o[sFieldName].getValStr();
						boost::to_upper(sBOValue);
						if (sBOValue == sSearchValue)
							return o;
					}
				}
			}
		}
	}
	return ret;
}


std::string GetBusinessObjectList(std::string sType, std::string sFields)
{
	boost::to_upper(sType);
	std::vector<std::string> vFields = Split(sFields.c_str(), ",");
	std::string sData = "";
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
	   	if (sKey.length() > sType.length())
		{
			if (sKey.substr(0,sType.length()) == sType)
			{
				std::string sPrimaryKey = sKey.substr(sType.length() + 1, sKey.length() - sType.length());
				std::string sIPFSHash = mvApplicationCache[ii.first];
				std::string sError = "";
				UniValue o = GetBusinessObject(sType, sPrimaryKey, sError);
				if (o.size() > 0)
				{
					bool fDeleted = o["deleted"].getValStr() == "1";
					if (!fDeleted)
					{
						// 1st column is ID - objecttype - recaddress - secondarykey
						std::string sPK = sType + "-" + sPrimaryKey + "-" + sIPFSHash;
						std::string sRow = sPK + "<col>";
						for (int i = 0; i < (int)vFields.size(); i++)
						{
							sRow += o[vFields[i]].getValStr() + "<col>";
						}
						sData += sRow + "<object>";
					}
				}
			}
		}
	}
	LogPrintf("BOList data %s \n",sData.c_str());
	return sData;
}
*/


/*
std::string GetDataFromIPFS(std::string sURL, std::string& sError)
{
	std::string sPath = () + "guid";
	std::ofstream fd(sPath.c_str());
	std::string sEmptyData = "";
	fd.write(sEmptyData.c_str(), sizeof(char)*sEmptyData.size());
	fd.close();

	int i = ipfs_download(sURL, sPath, 15, 0, 0);
	if (i != 1) 
	{
		sError = "IPFS Download error.";
		return "";
	}
	boost::filesystem::path pathIn(sPath);
    std::ifstream streamIn;
    streamIn.open(pathIn.string().c_str());
	if (!streamIn) 
	{
		sError = "FileSystem Error.";
		return "";
	}
	std::string sLine = "";
	std::string sJson = "";
    while(std::getline(streamIn, sLine))
    {
		sJson += sLine;
	}
	streamIn.close();
	return sJson;
}


std::string GetJSONFromIPFS(std::string sHash, std::string& sError)
{
	std::string sURL = "http://ipfs.biblepay.org:8080/ipfs/" + sHash;
	return GetDataFromIPFS(sURL, sError);
}
*/


int64_t GetFileSize(std::string sPath)
{
	if (!boost::filesystem::exists(sPath)) return 0;
	return (int64_t)boost::filesystem::file_size(sPath);
}

/*
std::string AddBlockchainMessages(std::string sAddress, std::string sType, std::string sPrimaryKey, 
	std::string sHTML, CAmount nAmount, double minCoinAge, std::string& sError)
{
	CBitcoinAddress cbAddress(sAddress);
    if (!cbAddress.IsValid()) 
	{
		sError = "\nAddBlockchainMessages::Invalid Destination Address:" + sAddress;
		return "";
	}
    CWalletTx wtx;
 	std::string sMessageType      = "<MT>" + sType + "</MT>";  
    std::string sMessageKey       = "<MK>" + sPrimaryKey + "</MK>";
	std::string s1 = "<BCM>" + sMessageType + sMessageKey;
	CAmount curBalance = pwalletMain->GetUnlockedBalance();
	if (curBalance < nAmount)
	{
		sError = "Insufficient funds (Unlock Wallet).";
		return "";
	}

    CScript scriptPubKey = GetScriptForDestination(cbAddress.Get());
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    vector<CRecipient> vecSend;
    int nChangePosRet = -1;
	bool fSubtractFeeFromAmount = false;
	int iStepLength = (MAX_MESSAGE_LENGTH - 100) - s1.length();
	if (iStepLength < 1) 
	{
		sError = "Message length error.";
		return "";
	}
	bool bLastStep = false;
	// 3-8-2018 R ANDREWS - Ensure each UTXO charge is rounded up
	double dBasicParts = ((double)sHTML.length() / (double)iStepLength);
	int iParts = std::ceil(dBasicParts);
	if (iParts < 1) iParts = 1;
	int iPosition = 0;
	double dUTXOAmount = nAmount / COIN;
	for (int i = 0; i <= (int)sHTML.length(); i += iStepLength)
	{
		int iChunkLength = sHTML.length() - i;
		if (iChunkLength <= iStepLength) bLastStep = true;
		if (iChunkLength > iStepLength) 
		{
			iChunkLength = iStepLength;
		} 
		std::string sChunk = sHTML.substr(i, iChunkLength);
		if (bLastStep) sChunk += "</BCM>";
		double dLegAmount = dUTXOAmount / ((double)iParts);
		CAmount caLegAmount = dLegAmount * COIN;
	    CRecipient recipient = {scriptPubKey, caLegAmount, false, fSubtractFeeFromAmount, false, false, false, "", "", "", ""};
		s1 += sChunk;
		recipient.Message = s1;
		LogPrintf("\r\n AddBlockChainMessage::Creating TXID Amount %f, MsgLength %f, StepLen %f, BasicParts %f, Parts %f, vout %f, ResumePos %f, ChunkLen %f, with Chunk %s \r\n", 
			dLegAmount, (double)sHTML.length(), (double)iStepLength, (double)dBasicParts, (double)iParts, (double)iPosition, (double)i, (double)iChunkLength, s1.c_str());
		s1 = "";
		iPosition++;
		vecSend.push_back(recipient);
	}
	
    
	bool fUseInstantSend = false;
	// Never spend sanctuary funds - R ANDREWS - BIBLEPAY
	// PODC_Update: Addl params required to enforce coin_age: bool fUseInstantSend=false, int iMinConfirms = 0, double dMinCoinAge = 0, CAmount caMinCoinAmount = 0
	// Ensure we don't spend POG bankroll denominations
	CAmount nBankrollMask = .001 * COIN;

    if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet,
                                         sError, NULL, true, ONLY_NOT1000IFMN, fUseInstantSend, 0, minCoinAge, 0, nBankrollMask)) 
	{
		if (!sError.empty())
		{
			return "";
		}

        if (!fSubtractFeeFromAmount && nAmount + nFeeRequired > pwalletMain->GetBalance())
		{
            sError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
			return "";
		}
    }
    if (!pwalletMain->CommitTransaction(wtx, reservekey, fUseInstantSend ? NetMsgType::TXLOCKREQUEST : NetMsgType::TX))
    {
		sError = "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
		return "";
	}

	std::string sTxId = wtx.GetHash().GetHex();
	return sTxId;
}
*/

void ClearCache(std::string sSection)
{
	boost::to_upper(sSection);
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
		boost::to_upper(sKey);
		if (sKey.length() > sSection.length())
		{
			if (sKey.substr(0,sSection.length())==sSection)
			{
				mvApplicationCache[sKey]="";
				mvApplicationCacheTimestamp[sKey]=0;
			}
		}
	}
}


void WriteCache(std::string sSection, std::string sKey, std::string sValue, int64_t locktime, bool IgnoreCase)
{
	if (sSection.empty() || sKey.empty()) return;
	if (IgnoreCase)
	{
		boost::to_upper(sSection);
		boost::to_upper(sKey);
	}
	std::string temp_value = mvApplicationCache[sSection + ";" + sKey];
	mvApplicationCache[sSection + ";" + sKey] = sValue;
	// Record Cache Entry timestamp
	mvApplicationCacheTimestamp[sSection + ";" + sKey] = locktime;
}

void PurgeCacheAsOfExpiration(std::string sSection, int64_t nExpiration)
{
	boost::to_upper(sSection);
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
		boost::to_upper(sKey);
		if (sKey.length() > sSection.length())
		{
			if (sKey.substr(0,sSection.length())==sSection)
			{
				int64_t nTimestamp = mvApplicationCacheTimestamp[sKey];
				if (nTimestamp < nExpiration)
				{
					mvApplicationCache[sKey]="";
					mvApplicationCacheTimestamp[sKey]=0;
				}
			}
		}
	}
}


/*
double GetDifficultyN(const CBlockIndex* blockindex, double N)
{
	int nHeight = 0;
	if (blockindex == NULL)
	{
		if (chainActive.Tip() != NULL) nHeight = chainActive.Tip()->nHeight;
	}
	else
	{
		nHeight = blockindex->nHeight;
	}
	
	// Returns Difficulty * N (Most likely this will be used to display the Diff in the wallet, since the BibleHash is much harder to solve than an ASIC hash)
	if ((!fProd && nHeight >= 1) || (fProd && nHeight >= 7000))
	{
		return GetDifficulty(blockindex)*(N/10); //f7000 feature
	}
	else
	{
		return GetDifficulty(blockindex)*N;
	}
}
*/

std::string GetArrayElement(std::string s, std::string delim, int iPos)
{
	std::vector<std::string> vGE = Split(s.c_str(),delim);
	if (iPos > (int)vGE.size()) return "";
	return vGE[iPos];
}

void GetMiningParams(int nPrevHeight, bool& f7000, bool& f8000, bool& f9000, bool& fTitheBlocksActive)
{
	f7000 = ((!fProd && nPrevHeight > 1) || (fProd && nPrevHeight > F7000_CUTOVER_HEIGHT)) ? true : false;
    f8000 = ((!fProd && nPrevHeight >= F8000_CUTOVER_HEIGHT_TESTNET) || (fProd && nPrevHeight >= F8000_CUTOVER_HEIGHT));
	f9000 = ((!fProd && nPrevHeight >= F9000_CUTOVER_HEIGHT_TESTNET) || (fProd && nPrevHeight >= F9000_CUTOVER_HEIGHT));
	int nLastTitheBlock = fProd ? LAST_TITHE_BLOCK : LAST_TITHE_BLOCK_TESTNET;
    fTitheBlocksActive = ((nPrevHeight+1) < nLastTitheBlock);
}

/*
std::string RetrieveTxOutInfo(const CBlockIndex* pindexLast, int iLookback, int iTxOffset, int ivOutOffset, int iDataType)
{
	// When DataType == 1, returns txOut Address
	// When DataType == 2, returns TxId
	// When DataType == 3, returns Blockhash

    if (pindexLast == NULL || pindexLast->nHeight == 0) 
	{
        return "";
    }

    for (int i = 1; i < iLookback; i++) 
	{
        if (pindexLast->pprev == NULL) { break; }
        pindexLast = pindexLast->pprev;
    }
	if (iDataType < 1 || iDataType > 3) return "DATA_TYPE_OUT_OF_RANGE";

	if (iDataType == 3) return pindexLast ? pindexLast->GetBlockHash().GetHex() : "";	
	
	const Consensus::Params& consensusParams = Params().GetConsensus();
	CBlock block;
	if (ReadBlockFromDisk(block, pindexLast, consensusParams))
	{
		if (iTxOffset >= (int)block.vtx.size()) iTxOffset=block.vtx.size()-1;
		if (ivOutOffset >= (int)block.vtx[iTxOffset]->vout.size()) ivOutOffset=block.vtx[iTxOffset]->vout.size()-1;
		if (iTxOffset >= 0 && ivOutOffset >= 0)
		{
			if (iDataType == 1)
			{
				std::string sPKAddr = PubKeyToAddress(block.vtx[iTxOffset]->vout[ivOutOffset].scriptPubKey);
				return sPKAddr;
			}
			else if (iDataType == 2)
			{
				std::string sTxId = block.vtx[iTxOffset]->GetHash().ToString();
				return sTxId;
			}
		}
	}
	
	return "";

}
*/

std::string GetIPFromAddress(std::string sAddress)
{
	std::vector<std::string> vAddr = Split(sAddress.c_str(),":");
	if (vAddr.size() < 2) return "";
	return vAddr[0];
}

/*
bool SubmitProposalToNetwork(uint256 txidFee, int64_t nStartTime, std::string sHex, std::string& sError, std::string& out_sGovObj)
{
        if(!masternodeSync.IsBlockchainSynced()) 
		{
			sError = "Must wait for client to sync with masternode network. ";
			return false;
        }
        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS
        uint256 hashParent = uint256();
        int nRevision = 1;
	    CGovernanceObject govobj(hashParent, nRevision, nStartTime, txidFee, sHex);
        DBG( cout << "gobject: submit "
             << " strData = " << govobj.GetDataAsString()
             << ", hash = " << govobj.GetHash().GetHex()
             << ", txidFee = " << txidFee.GetHex()
             << endl; );

        std::string strHash = govobj.GetHash().ToString();
        if(!govobj.IsValidLocally(sError, true)) 
		{
            sError += "Object submission rejected because object is not valid.";
			LogPrintf("\n OBJECT REJECTED:\n gobject submit 0 1 %f %s %s \n", (double)nStartTime, sHex.c_str(), txidFee.GetHex().c_str());

			return false;
        }

        // RELAY THIS OBJECT - Reject if rate check fails but don't update buffer
        if(!governance.MasternodeRateCheck(govobj)) 
		{
            sError = "Object creation rate limit exceeded";
			return false;
        }
        // This check should always pass, update buffer
        if(!governance.MasternodeRateCheck(govobj, UPDATE_TRUE)) 
		{
            sError = "Object submission rejected because of rate check failure (buffer updated)";
			return false;
        }
        governance.AddSeenGovernanceObject(govobj.GetHash(), SEEN_OBJECT_IS_VALID);
        govobj.Relay();
        bool fAddToSeen = true;
        governance.AddGovernanceObject(govobj, fAddToSeen);
		out_sGovObj = govobj.GetHash().ToString();
		return true;
}
*/

std::vector<char> ReadBytesAll(char const* filename)
{
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    std::vector<char>  result(pos);
    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);
    return result;
}


std::string GetFileNameFromPath(std::string sPath)
{
	sPath = strReplace(sPath, "/", "\\");
	std::vector<std::string> vRows = Split(sPath.c_str(), "\\");
	std::string sFN = "";
	for (int i = 0; i < (int)vRows.size(); i++)
	{
		sFN = vRows[i];
	}
	return sFN;
}

std::string SubmitToIPFS(std::string sPath, std::string& sError)
{
	if (!boost::filesystem::exists(sPath)) 
	{
		sError = "IPFS File not found.";
		return "";
	}
	std::string sFN = GetFileNameFromPath(sPath);
	std::vector<char> v = ReadBytesAll(sPath.c_str());
	std::vector<unsigned char> uData(v.begin(), v.end());
	std::string s64 = EncodeBase64(&uData[0], uData.size());
	std::string sData; // BiblepayIPFSPost(sFN, s64);
	std::string sHash = ExtractXML(sData,"<HASH>","</HASH>");
	std::string sLink = ExtractXML(sData,"<LINK>","</LINK>");
	sError = ExtractXML(sData,"<ERROR>","</ERROR>");
	return sHash;
}	
/*
std::string SendBusinessObject(std::string sType, std::string sPrimaryKey, std::string sValue, double dStorageFee, std::string sSignKey, bool fSign, std::string& sError)
{
	const Consensus::Params& consensusParams = Params().GetConsensus();
    std::string sAddress = consensusParams.FoundationAddress;
    CBitcoinAddress address(sAddress);
    if (!address.IsValid())
	{
		sError = "Invalid Destination Address";
		return sError;
	}
    CAmount nAmount = AmountFromValue(dStorageFee);
	CAmount nMinimumBalance = AmountFromValue(dStorageFee);
    CWalletTx wtx;
	boost::to_upper(sPrimaryKey); // Following same pattern
	boost::to_upper(sType);
	std::string sNonceValue = RoundToString(GetAdjustedTime(), 0);
	// Add immunity to replay attacks (Add message nonce)
 	std::string sMessageType      = "<MT>" + sType  + "</MT>";  
    std::string sMessageKey       = "<MK>" + sPrimaryKey   + "</MK>";
	std::string sMessageValue     = "<MV>" + sValue + "</MV>";
	std::string sNonce            = "<NONCE>" + sNonceValue + "</NONCE>";
	std::string sBOSignKey        = "<BOSIGNER>" + sSignKey + "</BOSIGNER>";
	std::string sMessageSig = "";

	if (fSign)
	{
		std::string sSignature = "";
		bool bSigned = SignStake(sSignKey, sValue + sNonceValue, sError, sSignature);
		if (bSigned) 
		{
			sMessageSig = "<BOSIG>" + sSignature + "</BOSIG>";
		}
		else
		{
			LogPrintf(" signing business object failed %s key %s ",sPrimaryKey.c_str(), sSignKey.c_str());
		}
	}
	std::string s1 = sMessageType + sMessageKey + sMessageValue + sNonce + sBOSignKey + sMessageSig;
	RPCSendMoneyToDestinationWithMinimumBalance(address.Get(), nAmount, nMinimumBalance, 0, 0, "", 0, wtx, sError);
	if (!sError.empty()) return "";
    return wtx.GetHash().GetHex().c_str();
}
*/

int GetSignalInt(std::string sLocalSignal)
{
	boost::to_upper(sLocalSignal);
	if (sLocalSignal=="NO") return 1;
	if (sLocalSignal=="YES") return 2;
	if (sLocalSignal=="ABSTAIN") return 3;
	return 0;
}
/*
UserVote GetSumOfSignal(std::string sType, std::string sIPFSHash)
{
	// Get sum of business object type - ipfshash - signal by ipfshash/objecttype/voter
	boost::to_upper(sType);
	boost::to_upper(sIPFSHash);
	std::string sData = "";
	UserVote v = UserVote();
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
		std::string sValue = mvApplicationCache[ii.first];
		if (Contains(sKey, sType + ";" + sIPFSHash))
		{
			std::string sLocalSignal = ExtractXML(sValue, "<signal>", "</signal>");
			double dWeight = cdbl(ExtractXML(sValue, "<voteweight>", "</voteweight>"), 2);
			int iSignal = GetSignalInt(sLocalSignal);
			if (iSignal == 1)
			{
				v.nTotalNoCount++;
				v.nTotalNoWeight += dWeight;
			}
			else if (iSignal == 2)
			{
				v.nTotalYesCount++;
				v.nTotalYesWeight += dWeight;
			}
			else if (iSignal == 3)
			{
				v.nTotalAbstainCount++;
				v.nTotalAbstainWeight += dWeight;
			}
		}
	}
	return v;
}
*/

UniValue GetDataList(std::string sType, int iMaxAgeInDays, int& iSpecificEntry, std::string sSearch, std::string& outEntry)
{
	int64_t nEpoch = GetAdjustedTime() - (iMaxAgeInDays * 86400);
	if (nEpoch < 0) nEpoch = 0;
    UniValue ret(UniValue::VOBJ);
	boost::to_upper(sType);
	if (sType=="PRAYERS") sType="PRAYER";  // Just in case the user specified PRAYERS
	ret.push_back(Pair("DataList",sType));
	int iPos = 0;
	int iTotalRecords = 0;
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
	   	if (sKey.length() > sType.length())
		{
			if (sKey.substr(0,sType.length())==sType)
			{
				std::string sPrimaryKey = sKey.substr(sType.length() + 1, sKey.length() - sType.length());
		     	int64_t nTimestamp = mvApplicationCacheTimestamp[ii.first];
			
				if (nTimestamp > nEpoch || nTimestamp == 0)
				{
					iTotalRecords++;
					std::string sValue = mvApplicationCache[ii.first];
					std::string sLongValue = sPrimaryKey + " - " + sValue;
					if (iPos==iSpecificEntry) outEntry = sValue;
					std::string sTimestamp = TimestampToHRDate((double)nTimestamp);
					if (!sSearch.empty())
					{
						std::string sPK1 = sPrimaryKey;
						std::string sPK2 = sValue;
						boost::to_upper(sPK1);
						boost::to_upper(sPK2);
						boost::to_upper(sSearch);
						if (Contains(sPK1, sSearch) || Contains(sPK2, sSearch))
						{
							ret.push_back(Pair(sPrimaryKey + " (" + sTimestamp + ")", sValue));
						}
					}
					else
					{
						ret.push_back(Pair(sPrimaryKey + " (" + sTimestamp + ")", sValue));
					}
					iPos++;
				}
			}
		}
	}
	iSpecificEntry++;
	if (iSpecificEntry >= iTotalRecords) iSpecificEntry=0;  // Reset the iterator.
	return ret;
}

/*
double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (chainActive.Tip() == NULL)
            return 1.0;
        else
            blockindex = chainActive.Tip();
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}
*/

UniValue ContributionReport()
{
	const Consensus::Params& consensusParams = Params().GetConsensus();
	int nMaxDepth = chainActive.Tip()->nHeight;
    CBlock block;
	int nMinDepth = 1;
	double dTotal = 0;
	double dChunk = 0;
    UniValue ret(UniValue::VOBJ);
	int iProcessedBlocks = 0;
	int nStart = 1;
	int nEnd = 1;
	for (int ii = nMinDepth; ii <= nMaxDepth; ii++)
	{
   			CBlockIndex* pblockindex = FindBlockByHeight(ii);
			if (ReadBlockFromDisk(block, pblockindex, consensusParams))
			{
				iProcessedBlocks++;
				nEnd = ii;
				for (auto tx : block.vtx) 
				{
					 for (int i=0; i < (int)tx->vout.size(); i++)
					 {
				 		std::string sRecipient = PubKeyToAddress(tx->vout[i].scriptPubKey);
						double dAmount = tx->vout[i].nValue/COIN;
						bool bProcess = false;
						if (sRecipient == consensusParams.FoundationAddress)
						{ 
							bProcess = true;
						}
						else if (pblockindex->nHeight == 24600 && dAmount == 2894609)
						{
							bProcess=true; // This compassion payment was sent to Robs address first by mistake; add to the audit 
						}
						if (bProcess)
						{
								dTotal += dAmount;
								dChunk += dAmount;
						}
					 }
				 }
		  		 double nBudget = CSuperblock::GetPaymentsLimit(ii) / COIN;
				 if (iProcessedBlocks >= (BLOCKS_PER_DAY*7) || (ii == nMaxDepth-1) || (nBudget > 5000000))
				 {
					 iProcessedBlocks = 0;
					 std::string sNarr = "Block " + RoundToString(nStart, 0) + " - " + RoundToString(nEnd, 0);
					 ret.push_back(Pair(sNarr, dChunk));
					 dChunk = 0;
					 nStart = nEnd;
				 }

			}
	}
	
	ret.push_back(Pair("Grand Total", dTotal));
	return ret;
}

void SerializePrayersToFile(int nHeight)
{
	if (nHeight < 100) return;
	std::string sSuffix = fProd ? "_prod" : "_testnet";
	std::string sTarget = GetSANDirectory2() + "prayers2" + sSuffix;
	FILE *outFile = fopen(sTarget.c_str(), "w");
	LogPrintf("Serializing Prayers... %f ",GetAdjustedTime());
	for (auto ii : mvApplicationCache) 
	{
		std::string sKey = ii.first;
	   	int64_t nTimestamp = mvApplicationCacheTimestamp[sKey];
		std::string sValue = mvApplicationCache[sKey];
		bool bSkip = false;
		if (sKey.length() > 7 && sKey.substr(0,7)=="MESSAGE" && (sValue == "" || sValue==" ")) bSkip = true;
		if (!bSkip)
		{
			std::string sRow = RoundToString(nTimestamp, 0) + "<colprayer>" + RoundToString(nHeight, 0) + "<colprayer>" + sKey + "<colprayer>" + sValue + "<rowprayer>\r\n";
			fputs(sRow.c_str(), outFile);
		}
	}
	LogPrintf("...Done Serializing Prayers... %f ",GetAdjustedTime());
    fclose(outFile);
}

int DeserializePrayersFromFile()
{
	std::string sSuffix = fProd ? "_prod" : "_testnet";
	std::string sSource = GetSANDirectory2() + "prayers2" + sSuffix;

	boost::filesystem::path pathIn(sSource);
    std::ifstream streamIn;
    streamIn.open(pathIn.string().c_str());
	if (!streamIn) return -1;
	int nHeight = 0;
	std::string line;
	int iRows = 0;
    while(std::getline(streamIn, line))
    {
		std::vector<std::string> vRows = Split(line.c_str(),"<rowprayer>");
		for (int i = 0; i < (int)vRows.size(); i++)
		{
			std::vector<std::string> vCols = Split(vRows[i].c_str(),"<colprayer>");
			if (vCols.size() > 3)
			{
				int64_t nTimestamp = cdbl(vCols[0], 0);
				int cHeight = cdbl(vCols[1], 0);
				if (cHeight > nHeight) nHeight = cHeight;
				std::string sKey = vCols[2];
				std::string sValue = vCols[3];
				std::vector<std::string> vKeys = Split(sKey.c_str(), ";");
				if (vKeys.size() > 1)
				{
					WriteCache(vKeys[0], vKeys[1], sValue, nTimestamp, true); //ignore case
					iRows++;
				}
			}
		}
	}
    LogPrintf(" Processed %f prayer rows \n", iRows);
	streamIn.close();
	return nHeight;
}

CAmount GetTitheAmount(CTransaction ctx)
{
	const Consensus::Params& consensusParams = Params().GetConsensus();
	for (unsigned int z = 0; z < ctx.vout.size(); z++)
	{
		std::string sRecip = PubKeyToAddress(ctx.vout[z].scriptPubKey);
		if (sRecip == consensusParams.FoundationAddress) 
		{
			return ctx.vout[z].nValue;  // First Tithe amount found in transaction counts
		}
	}
	return 0;
}

std::string VectToString(std::vector<unsigned char> v)
{
     std::string s(v.begin(), v.end());
     return s;
}

CAmount StringToAmount(std::string sValue)
{
	if (sValue.empty()) return 0;
    CAmount amount;
    if (!ParseFixedPoint(sValue, 8, &amount)) return 0;
    if (!MoneyRange(amount)) throw std::runtime_error("AMOUNT OUT OF MONEY RANGE");
    return amount;
}

bool CompareMask(CAmount nValue, CAmount nMask)
{
	if (nMask == 0) return false;
	std::string sAmt = "0000000000000000000000000" + AmountToString(nValue);
	std::string sMask= AmountToString(nMask);
	std::string s1 = sAmt.substr(sAmt.length() - sMask.length() + 1, sMask.length() - 1);
	std::string s2 = sMask.substr(1, sMask.length() - 1);
	return (s1 == s2);
}

bool CopyFile(std::string sSrc, std::string sDest)
{
	boost::filesystem::path fSrc(sSrc);
	boost::filesystem::path fDest(sDest);
	try
	{
		#if BOOST_VERSION >= 104000
			boost::filesystem::copy_file(fSrc, fDest, boost::filesystem::copy_option::overwrite_if_exists);
		#else
			boost::filesystem::copy_file(fSrc, fDest);
		#endif
	}
	catch (const boost::filesystem::filesystem_error& e) 
	{
		LogPrintf("CopyFile failed - %s ",e.what());
		return false;
    }
	return true;
}

/*
bool PODCEnabled(int nHeight)
{
	if (nHeight == 0)
	{
		return GetAdjustedTime() < 1553975345; // 3-30-2019
	}
	bool fDC = (fProd && nHeight > F11000_CUTOVER_HEIGHT_PROD && nHeight < PODC_LAST_BLOCK_PROD) || (!fProd && nHeight > F11000_CUTOVER_HEIGHT_PROD && nHeight < PODC_LAST_BLOCK_TESTNET);
	return fDC;
}

bool POGEnabled(int nHeight, int64_t nTime)
{
	bool fPOG = ((nHeight > FPOG_CUTOVER_HEIGHT_TESTNET && !fProd) || (fProd && nHeight > FPOG_CUTOVER_HEIGHT_PROD));
	if (nTime > 0)
	{
		int64_t nAge = GetAdjustedTime() - nTime;
		bool fRecent = nAge < (60 * 60 * 2) ? true : false;
		if (!fRecent) return false;
	}
	return fPOG;
}
*/


std::string Caption(std::string sDefault)
{
	std::string sValue = ReadCache("message", sDefault);
	return sValue.empty() ? sDefault : sValue;		
}


double GetBlockVersion(std::string sXML)
{
	std::string sBlockVersion = ExtractXML(sXML,"<VER>","</VER>");
	sBlockVersion = strReplace(sBlockVersion, ".", "");
	if (sBlockVersion.length() == 3) sBlockVersion += "0"; 
	double dBlockVersion = cdbl(sBlockVersion, 0);
	return dBlockVersion;
}

