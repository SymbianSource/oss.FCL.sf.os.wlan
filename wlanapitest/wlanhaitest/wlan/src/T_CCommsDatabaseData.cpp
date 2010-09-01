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



#include "t_ccommsdatabasedata.h"

/*@{*/
//LIT's for commands of CT_CCommsDatabaseData
_LIT(KCmdNewL,					"NewL");
_LIT(KCmdDestructor,			"~");
/*@}*/

/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_CCommsDatabaseData* CT_CCommsDatabaseData::NewL()
	{
	CT_CCommsDatabaseData* ret = new (ELeave) CT_CCommsDatabaseData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/**
 * Public destructor
 */
CT_CCommsDatabaseData::~CT_CCommsDatabaseData()
	{
	DestroyData();
	}

/**
 * Private constructor. First phase construction
 */

CT_CCommsDatabaseData::CT_CCommsDatabaseData()
:	iCommsDat(NULL)
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
void CT_CCommsDatabaseData::ConstructL()
	{
	}

/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_CCommsDatabaseData::GetObject()
	{
	return iCommsDat;
	}

/**
 * Process a command read from the Ini file
 * @param aCommand 			The command to process
 * @param aSection			The section get from the *.ini file of the project T_Wlan
 * @param aAsyncErrorIndex	Command index dor async calls to returns errors to
 * @return TBool			ETrue if the command is process
 * @leave					system wide error
 */
TBool CT_CCommsDatabaseData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName&/* aSection*/, const TInt /*aAsyncErrorIndex*/)
	{
	TBool ret = ETrue;
	
	if(aCommand == KCmdNewL)
		{
		DoCmdNewL();
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
 * Command to create an Instance of CCommsDatabase class
 * @param 
 * @return 
 */
void CT_CCommsDatabaseData::DoCmdNewL()
	{
	INFO_PRINTF1(_L("*START* CT_CCommsDatabaseData::DoCmdNewL"));
	DestroyData();
	
	TRAPD(err,iCommsDat = CCommsDatabase::NewL(ETrue));	
	if(err!=KErrNone)
		 {
		 ERR_PRINTF2(_L("CCommsDatabase* commsDat = CCommsDatabase::NewL(ETrue) left with error %d"), err);
		 SetError(err);
		 }
	else
		{
		INFO_PRINTF1(_L("CCommsDatabase* commsDat = CCommsDatabase::NewL(ETrue) was create"));
		}
	
	INFO_PRINTF1(_L("*END* CT_CCommsDatabaseData::DoCmdNewL"));
	}

/**
 * Command to destroy an Instance of CCommsDatabase class
 * @param
 * @return
 */
void CT_CCommsDatabaseData::DoCmdDestructor()
	{
	INFO_PRINTF1(_L("*START* CT_CCommsDatabaseData::DoCmdDestructor"));
	DestroyData();
 	INFO_PRINTF1(_L("*END* CT_CCommsDatabaseData::DoCmdDestructor"));
	}

/**
 *Helper function to DoCmdDelete command
 * @param
 * @return
 */
void CT_CCommsDatabaseData::DestroyData()
	{
	if(iCommsDat)
		{
	    delete iCommsDat;
	    iCommsDat = NULL;
		}
	}
