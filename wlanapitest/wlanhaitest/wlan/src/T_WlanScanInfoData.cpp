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


#include "t_wlanscaninfodata.h"
//class CWlanScanInfo
#include <wlanmgmtclient.h> 
#include <e32cmn.h>

/*@{*/
//LITs from the ini
_LIT(KSsidParam,					"DEFAULT_SSID_Ssid");
_LIT(KUid,							"WlanScanUid");
/*@}*/

/*@{*/
//LIT's for the commands
_LIT(KCmdNewL,						"NewL");
_LIT(KCmdDestructor,				"~");
_LIT(KCmdInformationElement,		"InformationElement");
/*@}*/

/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_WlanScanInfoData* CT_WlanScanInfoData::NewL()
	{
	CT_WlanScanInfoData* ret = new(ELeave) CT_WlanScanInfoData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/**
 * Public destructor
 */
CT_WlanScanInfoData::~CT_WlanScanInfoData()
	{
	DestroyData();
	}

/**
 * Private constructor. First phase construction
 */
CT_WlanScanInfoData::CT_WlanScanInfoData()
:	iData(NULL),
	iScanInfoInstanceIdentifier(KNullUid)
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
void CT_WlanScanInfoData::ConstructL()
	{
	}

/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_WlanScanInfoData::GetObject()
	{
	return iData;
	}


/**
 * Process a command read from the Ini file
 * @param aCommand 			The command to process
 * @param aSection			The section get from the *.ini file of the project T_Wlan
 * @param aAsyncErrorIndex	Command index dor async calls to returns errors to
 * @return TBool			ETrue if the command is process
 * @leave					system wide error
 */
TBool CT_WlanScanInfoData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
	{
	TBool ret = ETrue;
	
	if(aCommand == KCmdNewL)
		{
		DoCmdNewL(aSection);
		}
	else if(aCommand == KCmdDestructor)
		{
		DoCmdDestructor();
		}
	else if(aCommand == KCmdInformationElement)
		{
		DoCmdInformationElement(aSection);		
		}
	else
		{
		ret = EFalse;
		ERR_PRINTF1(_L("Unknown command"));
		}
	
	return ret;
	}


/**
 * Create an instance of CWlanScanInfo
 * @param
 * @return
 */
void CT_WlanScanInfoData::DoCmdNewL(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_WlanScanInfoData::DoCmdNewL"));
    DestroyData();
    
	TBool dataOk = ETrue;
	
    TInt wsUid;
    if(!GetHexFromConfig(aSection, KUid, wsUid ))
    	{
    	ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KUid);
		SetBlockResult(EFail);
		dataOk = EFalse;
    	}
    
    if (dataOk)
    	{
        const TUid WsUid = {wsUid};       
        TAny* interface = NULL;
        
        TRAPD(err, interface = REComSession::CreateImplementationL( WsUid, iScanInfoInstanceIdentifier ));
        if(err == KErrNone)
        	{
        	iData = reinterpret_cast<CWlanScanInfo*>( interface );

            if(iData == NULL)
            	{
            	ERR_PRINTF1(_L("CT_WlanScanInfoData::DoCmdNewL() Fail"));
            	SetBlockResult(EFail);
            	}
        	}
        else
    		{
    		ERR_PRINTF2(_L("Create Implementation left with error %d"), err);
    		SetError(err);
    		}	
    	}

    INFO_PRINTF1(_L("*END* CT_WlanScanInfoData::DoCmdNewL"));
	}

/**
 * Destructor for CWlanScanInfo
 * @param
 * @return
 */
void CT_WlanScanInfoData::DoCmdDestructor()
	{
	INFO_PRINTF1(_L("*START* CT_WlanScanInfoData::DoCmdDestructor"));
	DestroyData();
	INFO_PRINTF1(_L("*END* CT_WlanScanInfoData::DoCmdDestructor"));
	}

/**
 * called from DoCmdDestructor for destroy the object CWlanScanInfo
 * @param
 * @return
 */
void CT_WlanScanInfoData::DestroyData()
	{	
	// Cannot use "delete" directly because we use a member variable as an
	// ECom instance identifier	
	REComSession::DestroyedImplementation( iScanInfoInstanceIdentifier );
	iData = NULL;
	}
/**
 * Review if the IAP given in the ini file match with some Wireless Local Area Network (SelectScanInfo).
 * @param aSection				Section in the ini file for this command
 * @return
 */
void CT_WlanScanInfoData::DoCmdInformationElement(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_WlanScanInfoData::DoCmdInformationElement"));
	TBool dataOk = ETrue;
	
	TPtrC aSsid;
	if(!GetStringFromConfig(aSection,KSsidParam, aSsid))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KSsidParam);        
        SetBlockResult(EFail);
        dataOk = EFalse;
		}
	
	if(dataOk)
		{
		TInt err(KErrNone);	
		// Scan info gives data as "information elements"
		TUint8 ieLen(0);
		const TUint8* ieData;
		TWlanSsid ssid8;
		TBuf<KWlanMaxSsidLength> ssid;
		TBool match = EFalse;
		
	    INFO_PRINTF2(_L("SSID to be checked: %S"),&aSsid);
		for( iData->First(); !iData->IsDone(); iData->Next() )
	    	{
	        INFO_PRINTF1(_L("found scan info"));
			err = KErrNotReady;
			// Information Element ID for SSID as specified in 802.11.
			const TUint8 KWlan802Dot11SsidIE(0);
	    	err = iData->InformationElement( KWlan802Dot11SsidIE, ieLen, &ieData );
	        if(err != KErrNone)
	        	{
	        	ERR_PRINTF2(_L("CScanInfo::InformationElement err: [%d]"),err);
				SetError(err);
				break;
	        	}
	    	if(ieLen)
				{
				ssid8.Copy( ieData, ieLen );
				ssid.Copy( ssid8 );				
	            INFO_PRINTF2(_L("Current information element SSID: %S"),&ssid);
				// check if this is an expected SSID
				if( aSsid.Compare( ssid ) == 0 ) 
					{
					INFO_PRINTF1(_L("SSID match!"));					
					match = ETrue;
					break;
					}
				}
	    	}
		
		if (err == KErrNone && !match)
			{
			ERR_PRINTF2(_L("Given SSID %S NOT FOUND!"),&aSsid);
			SetBlockResult(EFail);
			}
		}
	
	INFO_PRINTF1(_L("*END* CT_WlanScanInfoData::DoCmdInformationElement"));
	}

