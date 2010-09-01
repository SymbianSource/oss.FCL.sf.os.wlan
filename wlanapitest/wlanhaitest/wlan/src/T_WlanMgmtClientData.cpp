/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/




#include "t_wlanmgmtclientdata.h"
#include <wlanmgmtclient.h>
#include <wdbifwlansettings.h>			

/*@{*/
//LIT's for Constant
_LIT(KScanInfo,						"scaninfo");
/*@}*/

/*@{*/
//LIT's for WlanMgmtClientData
_LIT(KCmdInstantiateMgmtClient,		"NewL");
_LIT(KCmdGetScanResults,			"GetScanResults");
_LIT(KCmdDestructor,				"~");
/*@}*/


/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_WlanMgmtClientData* CT_WlanMgmtClientData::NewL()
	{
	CT_WlanMgmtClientData* ret = new (ELeave) CT_WlanMgmtClientData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/**
 * Public destructor
 */
CT_WlanMgmtClientData::~CT_WlanMgmtClientData()
	{
	DestroyData();
	}

/**
 * Private constructor. First phase construction
 */
CT_WlanMgmtClientData::CT_WlanMgmtClientData()
:	iData(NULL)	
	{	
	}

/**
 * Second phase construction
 *
 * @internalComponent
 *
 * @return	N/A
 *
 * @pre		None
 * @post	None
 *
 * @leave	system wide error
 */
void CT_WlanMgmtClientData::ConstructL()
	{	
	}

/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_WlanMgmtClientData::GetObject()
	{
	return iData;
	}


/**
* Process a command read from the Ini file
* @param aCommand 			The command to process
* @param aSection			The section get from the *.ini file of the project T_Wlan
* @param aAsyncErrorIndex	Command index dor async calls to returns errors to
* @return TBool			    ETrue if the command is process
* @leave					system wide error
*/
TBool CT_WlanMgmtClientData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
	{
	TBool ret = ETrue;	
	if(aCommand == KCmdInstantiateMgmtClient)
		{
		DoCmdNewL();		
		}	
	else if(aCommand == KCmdGetScanResults)
		{
		DoCmdGetScanResults(aSection);		
		}		
	else if(aCommand == KCmdDestructor)
		{
		DoCmdDestructor();		
		}
	else
		{
		ERR_PRINTF1(_L("Unknown command."));
		ret = EFalse;
		}
	return ret;
	}


/**
 * Creates an Instance of CWlanMgmtClient
 * @param
 * @return
 */
void CT_WlanMgmtClientData::DoCmdNewL()
	{
	INFO_PRINTF1(_L("*START* CT_WlanMgmtClientData::DoCmdNewL"));
	
	DestroyData();
	
	TRAPD(err,iData = CWlanMgmtClient::NewL());
	if(err != KErrNone)
		{
		ERR_PRINTF2(_L("CWlanMgmtClient was not constructed, err=%d" ),err);
		SetError(err);
		}
	
	INFO_PRINTF1(_L("*END* CT_WlanMgmtClientData::DoCmdNewL"));
	}


/**
 * Test getting Wlan scan info from Wlan management interface. Call
 * DoCmdNewL for instantiate the CWlanMgmtClient and DoCmdNewL of CWlanScanInfo first.
 * @param aSection				Section to read from the ini file
 * @return
 */
void CT_WlanMgmtClientData::DoCmdGetScanResults(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_WlanMgmtClientData::DoCmdGetScanResults"));
	TBool dataOk = ETrue;
	
	TPtrC scanInfoName;
	if(!GetStringFromConfig(aSection, KScanInfo, scanInfoName))
		{        
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KScanInfo);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	if (dataOk)
		{
		CWlanScanInfo* iScanInfo = static_cast<CWlanScanInfo*>(GetDataObjectL(scanInfoName));	
		if ( iScanInfo != NULL )
			{
			TInt err = iData->GetScanResults( *iScanInfo ) ;
			if(err == KErrNone)
				{
				INFO_PRINTF2(_L("iScanInfo size [%d]"),iScanInfo->Size());
				}
			else
				{
				ERR_PRINTF2(_L("iData->GetScanResults( *iScanInfo ) Failed with error %d"), err);
				SetError(err);
				}
			}
		else
		    {
		    ERR_PRINTF1(_L("iScanInfo is NULL"));
		    SetBlockResult(EFail);
		    } 
		}
	
	INFO_PRINTF1(_L("*END* CT_WlanMgmtClientData::DoCmdGetScanResults"));
	}	

/**
 * Destructor for CWlanMgmtClient
 * @param
 * @return
 */
void CT_WlanMgmtClientData::DoCmdDestructor()
	{
	INFO_PRINTF1(_L("*START* CT_WlanMgmtClientData::DoCmdDestructor"));
	DestroyData();
	INFO_PRINTF1(_L("*END* CT_WlanMgmtClientData::DoCmdDestructor"));
	}

/**
 * Destroy the object of CWlanMgmtClient, call made from DoCmdDestructor()
 * @param
 * @return
 */
void CT_WlanMgmtClientData::DestroyData()
	{
	if(iData)
		{
		delete iData;
		iData = NULL;
		}
	}
