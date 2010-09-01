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



#include "t_rconnectiondata.h"
#include "t_rsocketservdata.h"
#include <commdbconnpref.h>

/*@{*/
//LIT param read from the ini file
_LIT(KSocketServ,				"socketserv");
/*@}*/

/*@{*/
//LITs for commands
_LIT(KCmdOpen,					"Open");
_LIT(KCmdStart,					"Start");
_LIT(KCmdClose,					"Close");
/*@}*/


/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_RConnectionData* CT_RConnectionData::NewL()
	{
	CT_RConnectionData * ret = new (ELeave)CT_RConnectionData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/**
 * Public destructor
 */
CT_RConnectionData::~CT_RConnectionData()
	{
	if (iConnection)
		{
		delete iConnection;
		iConnection = NULL;
		}
	}

/**
 * Private constructor. First phase construction
 */
CT_RConnectionData::CT_RConnectionData()
:	iConnection(NULL)
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
void CT_RConnectionData::ConstructL()
	{
	iConnection = new (ELeave)RConnection();
	}

/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_RConnectionData::GetObject()
	{
	 return iConnection;
	}

/**
 * Process a command read from the Ini file
 * @param aCommand 			The command to process
 * @param aSection			The section get from the *.ini file of the project T_Wlan
 * @param aAsyncErrorIndex	Command index dor async calls to returns errors to
 * @return TBool			ETrue if the command is process
 * @leave					system wide error
 */
TBool CT_RConnectionData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
	{
	TBool ret = ETrue;
	if(aCommand == KCmdOpen)
		{
		DoCmdOpen(aSection);
		}
	else if(aCommand == KCmdStart)
		{
		DoCmdStart(aSection);
		}	
	else if(aCommand == KCmdClose)
		{
		DoCmdClose();
		}
	else
		{
		ERR_PRINTF1(_L("Unknown command."));
		ret= EFalse;
		}
	return ret;
	}


/**
 * Command to open a connection (RConnection::Open). The errors are management
 * with SetError() and SetBlockResult().
 * @param  aSection				Section in the ini file for this command
 * @return 
 */
void CT_RConnectionData::DoCmdOpen(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RConnectionData::DoCmdOpen"));
	TBool dataOk = ETrue;
	
	// read param from the ini file
	TPtrC socketServName;
	if(!GetStringFromConfig(aSection, KSocketServ, socketServName))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KSocketServ);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	
	if (dataOk)
		{
		RSocketServ* iSocketServ = static_cast<RSocketServ*>(GetDataObjectL(socketServName));

		// Open a connection
		TInt err = iConnection->Open(*iSocketServ);
		if(err ==  KErrNone)
			{
			INFO_PRINTF1(_L("The connection was opened"));
			}
		else
			{
			ERR_PRINTF2(_L("iConnection->Open( iSocketServ ) Failed with error %d"), err);
			SetError(err);		
			}
		
		INFO_PRINTF1(_L("*END* CT_RConnectionData::DoCmdOpen"));
		}
	}

/**
 * Command to Start a connection with the ID IAP given before in the wrapper CT_RSocketServData. The
 * errors are management with SetError() and SetBlockResult()
 * @param aSection				Section in the ini file for this command
 * @return
 */
void CT_RConnectionData::DoCmdStart(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RConnectionData::DoCmdStart"));
	TBool dataOk = ETrue;
	
	// read a param from the ini file
	TPtrC socketServName;
	if(!GetStringFromConfig(aSection, KSocketServ, socketServName))
		{        
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KSocketServ);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	
	if (dataOk)
		{
		TInt err(KErrNone);
		TUint32 id;
		
		// Get complete wrapper
		CT_RSocketServData* iSocketServ = static_cast<CT_RSocketServData*>(GetDataWrapperL(socketServName));
		INFO_PRINTF2(_L("iSocketServ: %S"), &socketServName);
		
		if(iSocketServ != NULL)
			{
			// Start the connection using the given (WLAN) access point
			id = iSocketServ->GetIapID();
			INFO_PRINTF2(_L("CT_RConnectionData::DoCmdStart: Start RConnection, using IAP [%d]"), id);
			
			TCommDbConnPref* connPref = new (ELeave) TCommDbConnPref;
			connPref->SetIapId(id);
			connPref->SetDialogPreference( ECommDbDialogPrefDoNotPrompt );
			connPref->SetDirection( ECommDbConnectionDirectionOutgoing );	
			connPref->SetBearerSet(KCommDbBearerUnknown);
			
			INFO_PRINTF1(_L("CT_RConnectionData: Starting connection"));
			// Wait before the connection is really made
			// Wait time is 8 seconds.
			err = iConnection->Start( *connPref ) ;
			if(err != KErrNone)
				{
				 ERR_PRINTF2(_L("iConnection->Start( connPref ) Fail: %d "),err);		 
				 SetError(err);
				}
			}
		else
			{
			ERR_PRINTF1(_L("CT_RConnectionData::DoCmdStart: iSocketServ is NULL"));
			SetBlockResult(EFail);
			}
		}
	
	INFO_PRINTF1(_L("*END* CT_RConnectionData::DoCmdStart"));
	}


/**
 * Command to close a connection(RConnection::Close)
 * @param
 * @return
 */
void CT_RConnectionData::DoCmdClose()
	{
	INFO_PRINTF1(_L("*START* CT_RConnectionData::DoCmdClose"));
	Close();
	INFO_PRINTF1(_L("*END* CT_RConnectionData::DoCmdClose"));
	}

/**
 * Helper function for the command DoCmdConnection
 * @param
 * @return
 */

void CT_RConnectionData::Close()
	{
	INFO_PRINTF1(_L("Closing connection"));
	iConnection->Close();
	}
