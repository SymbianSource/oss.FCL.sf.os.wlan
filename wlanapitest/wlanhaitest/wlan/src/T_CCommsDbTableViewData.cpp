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




#include "t_ccommsdbtableviewdata.h"

// CommsDat preferences setting class.
#include <cdbpreftable.h>
#include <commdbconnpref.h>

/*@{*/
//LIT for the data read from the ini file
_LIT(KCommsDb,				"commsdb");
/*@}*/

/*@{*/
//LIT's for the commands
_LIT(KCmdNewL,				"NewL");
_LIT(KCmdDestructor,		"~");
/*@}*/

/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_CCommsDbTableViewData* CT_CCommsDbTableViewData::NewL()
	{
	CT_CCommsDbTableViewData* ret = new (ELeave) CT_CCommsDbTableViewData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/**
 * Public destructor
 */
CT_CCommsDbTableViewData::~CT_CCommsDbTableViewData()
	{
	DestroyData();
	}

/**
 * Private constructor. First phase construction
 */
CT_CCommsDbTableViewData::CT_CCommsDbTableViewData()
:	iSearchView(NULL)
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
void CT_CCommsDbTableViewData::ConstructL()
	{
	}

/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_CCommsDbTableViewData::GetObject()
	{
	return iSearchView;
	}

/**
* Process a command read from the Ini file
* @param aCommand 			The command to process
* @param aSection			The section get from the *.ini file of the project T_Wlan
* @param aAsyncErrorIndex	Command index dor async calls to returns errors to
* @return TBool			    ETrue if the command is process
* @leave					system wide error
*/

TBool CT_CCommsDbTableViewData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt/* aAsyncErrorIndex*/)
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
	else
		{
		ERR_PRINTF1(_L("Unknown command."));
		ret = EFalse;
		}
	return ret;
	}




/**
 * Command to create an instance of CCommsDbTableView class
 * @param aSection			The section in the ini file for this command
 * @return 
 */
void CT_CCommsDbTableViewData::DoCmdNewL(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_CCommsDbTableViewData::DoCmdNewL"));
	DestroyData();
	
	TPtrC commsDbName;
	const TUint32 KIAPMask = 0xffffffff;
	TBool	dataOk = ETrue;
	
	//param from the ini file
	if(!GetStringFromConfig(aSection, KCommsDb, commsDbName))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KCommsDb);
        SetBlockResult(EFail);
    	dataOk = EFalse;
		}
	
	if (dataOk)
		{
		CCommsDatabase* acommsDat = static_cast<CCommsDatabase*>(GetDataObjectL(commsDbName));
	  	//CCommsDatabase* acommsDat = CCommsDatabase::NewL(ETrue);
		//iSearchView = acommsDat->OpenIAPTableViewMatchingBearerSetLC( KIAPMask, ECommDbConnectionDirectionOutgoing );
		iSearchView = acommsDat->OpenIAPTableViewMatchingBearerSetLC( KIAPMask, ECommDbConnectionDirectionOutgoing );
		CleanupStack::Pop();
		}
  	
  	INFO_PRINTF1(_L("*END* CT_CCommsDbTableViewData::DoCmdNewL"));
	}
/**
 * Command for delete an instance of CCommsDbTableView class
 * @param
 * @return
 */
void CT_CCommsDbTableViewData::DoCmdDestructor()
	{
	INFO_PRINTF1(_L("*START* CT_CCommsDbTableViewData::DoCmdDestructor"));
	
	//CleanupStack::Pop(iSearchView);
	DestroyData();
	
	INFO_PRINTF1(_L("*END* CT_CCommsDbTableViewData::DoCmdDestructor"));
	}

/**
 * Helper function for the command DoCmdDelete
 * @param
 * @return
 */
void CT_CCommsDbTableViewData::DestroyData()
	{
	if (iSearchView)
		{
		delete iSearchView;
		iSearchView = NULL;
		}
	}
