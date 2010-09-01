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




#ifndef T_RFILEDATA_H_
#define T_RFILEDATA_H_

//User Includes
#include "datawrapperbase.h"

class CT_RFileData: public CDataWrapperBase
	{
public:
	static CT_RFileData* NewL();
	virtual ~CT_RFileData();

public:	
	virtual TAny* GetObject();
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);			 

protected:
	CT_RFileData();
	void ConstructL();

private:
  	void DoCmdGenerateFile(const TTEFSectionName& aSection);

private:
 	/**
 	 * For create a file
 	 */
 	RFile*		iFile;

    /**
     * Handle for file server session
     */
 	RFs			iFs;
	};
	
#endif /*T_RFILEDATA_H_*/
