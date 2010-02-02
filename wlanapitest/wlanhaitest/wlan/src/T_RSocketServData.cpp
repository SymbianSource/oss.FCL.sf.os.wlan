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



#include "t_rsocketservdata.h"
#include <commdb.h>				
#include <wdbifwlansettings.h>			
#include <apselect.h>     
#include <aplistitem.h>
#include <apdatahandler.h>
#include <apaccesspointitem.h>
/*@{*/
//LIT's params fron the ini file
_LIT(KWlanIap,							"WLANIAP");
_LIT(KWlanSsid,							"DEFAULT_SSID");
_LIT(KCommsDbTableView,     			"commsdbtableview");
_LIT(KCommsDatabase,     			    "commsdatabase");
/*@}*/

/*@{*/
//LIT's commands
_LIT(KCmdSetOutgoingIap,				"SetOutgoingIap");
_LIT(KCmdConnect,						"Connect");
_LIT(KCmdClose,							"Close");
/*@}*/


/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_RSocketServData* CT_RSocketServData::NewL()
	{
	CT_RSocketServData * ret = new (ELeave) CT_RSocketServData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/*
 * public destructor
 */
CT_RSocketServData::~CT_RSocketServData()
	{
	if(iSocketServConnected)
		{
		Close();
		}
	if(iSocketServ)
		{
		 delete iSocketServ;
		 iSocketServ = NULL;
		}
	}

/**
 * Private constructor. First phase construction
 */
CT_RSocketServData::CT_RSocketServData()
:	iSocketServ(NULL),
	iSocketServConnected(EFalse),
	iIapID(0)
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
void CT_RSocketServData::ConstructL()
	{
	iSocketServ = new (ELeave)RSocketServ();
	}


/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_RSocketServData::GetObject()
	{
	return iSocketServ;
	}

void CT_RSocketServData::SetIapID(TUint32 aIapID)
	{
	iIapID = aIapID;
	}

/**
 * Process a command read from the Ini file
 * @param aCommand 			The command to process
 * @param aSection			The section get from the *.ini file of the project T_Wlan
 * @param aAsyncErrorIndex	Command index dor async calls to returns errors to
 * @return TBool			ETrue if the command is process
 * @leave					system wide error
 */
TBool CT_RSocketServData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
	{
	TBool ret = ETrue;
	
	if(aCommand == KCmdSetOutgoingIap)
		{
		DoCmdSetOutgoingIap(aSection);
		}
	else if(aCommand == KCmdConnect)
		{
		DoCmdConnect();
		}
	else if(aCommand == KCmdClose)
		{
		DoCmdClose();
		}
	else
		{
		ERR_PRINTF1(_L("Unknown command."));
		ret = EFalse;
		}
	return ret;
	}


/**
 * Get IAP, matching the name given (KWlanIap parameter read from the ini file). Set SSID of the
 * IAP to the given value (KWlanSsid parameter read from the ini file).
 * Store the ID of the IAP locally to allow using the IAP for connecting.
 * If there are errors, are management for SetBlockResult() and SetError()
 * @param aSection				Section to review in the ini file for this command
 * @return void
 */
void CT_RSocketServData::DoCmdSetOutgoingIap(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketServData::DoCmdSetOutgoingIap"));
	TBool dataOk = ETrue;
	
    TPtrC aIapName;
	if(!GetStringFromConfig(aSection, KWlanIap, aIapName))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KWlanIap);        
        SetBlockResult(EFail);
        dataOk = EFalse;
		}
	
    TPtrC aSsid;    
	if(!GetStringFromConfig(aSection, KWlanSsid, aSsid))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KWlanSsid);
        SetBlockResult(EFail);
        dataOk = EFalse;
		}	
	
    TPtrC commsdbtableName;
	if(!GetStringFromConfig(aSection, KCommsDbTableView, commsdbtableName))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"),&KCommsDbTableView);
        SetBlockResult(EFail);
        dataOk = EFalse;
		}
	
    TPtrC commsdbName;
	if(!GetStringFromConfig(aSection, KCommsDatabase, commsdbName))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"),&KCommsDatabase);
        SetBlockResult(EFail);
        dataOk = EFalse;
		}
	
    if (dataOk)
		{
		// reset IAP id
		SetIapID(0);	
		
        TBool apFound = EFalse;
		// Open view to IAP table, select all outgoing IAPs.	
		CCommsDbTableView* searchView = static_cast<CCommsDbTableView*>(GetDataObjectL(commsdbtableName));
        CCommsDatabase* commsDatabase =  static_cast<CCommsDatabase*>(GetDataObjectL(commsdbName));
		
		// Make sure the view was available.
		if( searchView != NULL && commsDatabase != NULL )
			{
			CleanupStack::PushL(searchView);
			INFO_PRINTF1(_L("Start browsing through the IAPs."));
			TInt error = searchView->GotoFirstRecord();
			
			if( error == KErrNone )
				{
				// Buffer for reading IAP names from CommsDat. Buffer size is set to
				// maximum readable value from CommsDat.
				TBuf<KCommsDbSvrMaxColumnNameLength> iapName;
				TUint32 iapID = 0;
				INFO_PRINTF1(_L("CT_RSocketServData: CommsDat ready for searching, going through all outgoing IAPs"));
				TUint32 iapservice = 0;
				
				TBool failed = EFalse;
				
				// Go through all IAPs.
				while( error == KErrNone )
					{
					iapName.FillZ();
					
					// Read IAP ID and name from IAP table in CommsDat.
					TRAPD(err, searchView->ReadTextL( TPtrC( COMMDB_NAME ), iapName ));
					
				    if(err == KErrNone)
				    	{
						TRAP(err, searchView->ReadUintL( TPtrC( COMMDB_ID ), iapID ));
						
					    if(err == KErrNone)
					    	{
							INFO_PRINTF3(_L("CT_RSocketServData: IAP (ID = [%d]): %S"), iapID, &iapName );
							
							// Try to match the name with user input in the ini file.
							if( iapName.Match( aIapName ) == KErrNone )
								{
								INFO_PRINTF2(_L("CT_RSocketServData: Matching IAP name found with IAP ID = [%d]"), iapID );
								
                                apFound = ETrue;
                                
								// Return the found IAP ID			
								SetIapID(iapID);
								
								// Read IAP service from IAP table in CommsDat.
								TRAPD(err, searchView->ReadUintL( TPtrC( IAP_SERVICE ), iapservice ));
								
							    if(err == KErrNone)
							    	{
									INFO_PRINTF2(_L("Service of the AP: %d"),iapservice);
									
									// Write the ssid given as a parameter in WLANServiceTable in CommsDat
									INFO_PRINTF1(_L("CT_RSocketServData: Get WlanSettings from WLANServiceTable"));
									CWLanSettings* wlanset = new (ELeave) CWLanSettings();
									CleanupStack::PushL( wlanset );
									
									// Connect to CommsDat
									err = wlanset->Connect();
									
									if( err == KErrNone )
										{
										// Get wlan settings corresponding IAP service info from IAP table
										SWLANSettings wlanSettings;
										err = wlanset->GetWlanSettings( iapservice, wlanSettings );
										
										if( err == KErrNone )
											{
											INFO_PRINTF2(_L("CT_RSocketServData: CommsDat: wlanSettings.Name = %S"), &wlanSettings.Name );
											INFO_PRINTF2(_L("CT_RSocketServData: CommsDat: wlanSettings.SSID = %S"), &wlanSettings.SSID );								
											
											// Set the new ssid from the ini file
											wlanSettings.SSID = aSsid;
											INFO_PRINTF2(_L("CT_RSocketServData: New value for wlanSettings.SSID = %S"), &wlanSettings.SSID );
											
											// Write the new settings in CommsDat
											err = wlanset->WriteWlanSettings(wlanSettings );
											
											if( err == KErrNone )
												{
												INFO_PRINTF1(_L("CT_RSocketServData: WlanSettings saved in CommsDat"));
												wlanset->Disconnect();
												CleanupStack::PopAndDestroy( wlanset );
												}
											else
												{
												ERR_PRINTF2(_L("CT_RSocketServData: WriteWlanSettings error: [%d]"), err );
												SetError(err);
												failed = ETrue;
												break;
												}
											}
										else
											{
											ERR_PRINTF2(_L("CT_RSocketServData: Get WlanSettings error: [%d]"), err );			
											SetError(err);
											failed = ETrue;
											break;
											}
										}
									else
										{
										ERR_PRINTF2(_L("CT_RSocketServData: WLanSettings connect failed! [%d]"), err );				
										SetError(err);
										failed = ETrue;
										break;
										}
							    	}
							    else
									{
									ERR_PRINTF2(_L("searchView->ReadUintL left with error %d"), err);
									SetError(err);
									failed = ETrue;
									break;
									}	
								}
							
							error = searchView->GotoNextRecord();
							if(error == KErrNotFound)
								{
								INFO_PRINTF2(_L("searchView->GotoNextRecord() not found [%d]"), error);
								INFO_PRINTF1(_L("No more records to look for"));
								}
							else if(error != KErrNone)
								{
								ERR_PRINTF2(_L("searchView->GotoNextRecord() Failed with error = %d"),error);
								SetError(err);
								failed = ETrue;
								break;
								}
					    	}
					    else
							{
							ERR_PRINTF2(_L("searchView->ReadUintL left with error %d"), err);
							SetError(err);
							failed = ETrue;
							break;
							}	
				    	}
				    else
						{
						ERR_PRINTF2(_L("searchView->ReadTextL left with error %d"), err);
						SetError(err);
						failed = ETrue;
						break;
						}
					}	
				
				CleanupStack::Pop( searchView );
				
				//if( !failed && GetIapID() == 0 )
				//	{
				//	ERR_PRINTF1(_L("No valid IAP found"));
				//	SetBlockResult(EFail);
				//	}
				}
			else
				{
				INFO_PRINTF2(_L("CT_RSocketServData: No IAPs found [%d]"), error );		
				}
                
            if(apFound == EFalse)
                {
                CApAccessPointItem *wlan = CApAccessPointItem::NewLC();
                wlan->SetNamesL(aIapName);
                wlan->SetBearerTypeL(EApBearerTypeWLAN);
                wlan->WriteTextL(EApWlanNetworkName, aSsid);
                CApDataHandler *handler = CApDataHandler::NewLC(*commsDatabase);
                TUint32 apId = handler->CreateFromDataL(*wlan);
                INFO_PRINTF4(_L("Add new IAP ID: %d, name:%S, SSID: %S"), apId,&aIapName,&aSsid);
                SetIapID(apId);
                CleanupStack::PopAndDestroy(2);
                }
			}
		else
			{
			ERR_PRINTF1(_L("CT_RSocketServData: No IAPs found"));
			ERR_PRINTF1(_L("CommsDat, no view and database were available."));
			SetBlockResult(EFail);
			}	
		}
	
	INFO_PRINTF1(_L("*END* CT_RSocketServData::DoCmdSetOutgoingIap"));
	}

/**
 * Command to calls RSocketServ::Connect. The error is management for SetError() helper
 * @param
 * @return
 */
void CT_RSocketServData::DoCmdConnect()
	{
	INFO_PRINTF1(_L("*START* CT_RSocketServData::DoCmdConnect"));
	
	TInt err = iSocketServ->Connect();	
	if(err != KErrNone)
 		{
 		ERR_PRINTF1(_L("iSocketServ->Connect() Fail"));
		SetError(err);
 		}
	else
		{
		iSocketServConnected = ETrue;
		}
	
	INFO_PRINTF1(_L("*END* CT_RSocketServData::DoCmdConnect"));
	}
/**
 * Command to close RSocketServ instance
 * @param
 * @return
 */
void CT_RSocketServData::DoCmdClose()
	{
	INFO_PRINTF1(_L("*START* CT_RSocketServData::DoCmdClose"));
	Close();
	INFO_PRINTF1(_L("*END* CT_RSocketServData::DoCmdClose"));
	}

/**
 * Helper for the command DoCmdCloseSocketServ: RSocketServ::Close
 * @param
 * @return
 */
void CT_RSocketServData::Close()
	{
	iSocketServ->Close();
 	iSocketServConnected = EFalse; 		
	}
