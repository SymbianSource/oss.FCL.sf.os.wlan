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



#include "t_rfiledata.h"

/*@{*/
//LIT for the command DoCmdGenerateFile
_LIT(KCmdGenerateFile,				"GenerateFile");
/*@}*/

/*@{*/
//LITs for param reads from the ini file
_LIT(KFile,						"File");
_LIT(KSize,						"Size");
/*@}*/


/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_RFileData* CT_RFileData::NewL()
	{
	CT_RFileData* ret = new (ELeave) CT_RFileData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/**
 * Public destructor
 */
CT_RFileData::~CT_RFileData() 
	{
	iFs.Close();
	
	if (iFile)
		{
		delete iFile;
		iFile = NULL;
		}
	}

/**
 * Private constructor. First phase construction
 */
CT_RFileData::CT_RFileData()
:	iFile(NULL),
	iFs()
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
void CT_RFileData::ConstructL()
	{
	iFile = new (ELeave)RFile();
	}

/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_RFileData::GetObject()
	{
	return iFile;
	}

/**
 * Process a command read from the Ini file
 * @param aCommand 			The command to process
 * @param aSection			The section get from the *.ini file of the project T_Wlan
 * @param aAsyncErrorIndex	Command index dor async calls to returns errors to
 * @return TBool			ETrue if the command is process
 * @leave					system wide error
 */

TBool CT_RFileData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
	{
	TBool ret =ETrue;
	if(aCommand == KCmdGenerateFile())
		{
		 DoCmdGenerateFile(aSection);
		}
	else
		{
		ERR_PRINTF1(_L("Unknown command."));
		ret = EFalse;
		}
	return ret;
	}


/**
 * Command to generate a file for uploading in a host. If there are errors, SetBlockResult() and SetError() 
 * are used for management.
 * @param aSection				Section in the ini file for this command
 * @return
 */
void CT_RFileData::DoCmdGenerateFile(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RFileData::DoCmdGenerateFile"));
	
	TBool dataOk = ETrue;
	
	TPtrC file;
	if(!GetStringFromConfig(aSection, KFile, file))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KFile);       
        SetBlockResult(EFail);
        dataOk = EFalse;
		}

	TInt size;
	if(!GetIntFromConfig(aSection, KSize, size))
		{
        ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KSize);
        SetBlockResult(EFail);
        dataOk = EFalse;
		}

	if (dataOk)
		{
		INFO_PRINTF1(_L("Connect RFs"));

		CleanupClosePushL( iFs );
		CleanupClosePushL( *iFile );
		
		TInt err = iFs.Connect();
		if(err == KErrNone)
			{
			INFO_PRINTF1(_L("Replace file"));
			err = iFile->Replace( iFs, file, EFileShareAny|EFileWrite );
			if(err == KErrNone)
				{
				INFO_PRINTF1(_L("Set file size"));
				err = iFile->SetSize( size );
				if(err != KErrNone)
					{
					ERR_PRINTF2(_L("CT_RFileData::DoCmdGenerateFile: file.SetSize(...) Failed with error %d"), err);
					SetError(err);
					}
				}
			else
				{
				ERR_PRINTF2(_L("CT_RFileData::DoCmdGenerateFile: file.Replace(...) Failed with error %d"), err);
				SetError(err);
				}
			}
		else
			{
			ERR_PRINTF2(_L("CT_RFileData::DoCmdGenerateFile: fs.Connect() Failed with error %d"), err);
			SetError(err);
			}
		
		INFO_PRINTF1(_L("Close RFile handle"));
		CleanupStack::PopAndDestroy( iFile );
		INFO_PRINTF1(_L("Close RFs handle"));	
		CleanupStack::PopAndDestroy( &iFs );	
		}
	
	INFO_PRINTF1(_L("*END* CT_RFileData::DoCmdGenerateFile"));
	}
