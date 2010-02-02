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



#ifndef T_CCOMMSDATABASEDATA_H_
#define T_CCOMMSDATABASEDATA_H_

//User Includes
#include "datawrapperbase.h"

//Epoc includes
#include <commdb.h>						

class CT_CCommsDatabaseData: public CDataWrapperBase
	{
public:
	static CT_CCommsDatabaseData* NewL();
	virtual ~CT_CCommsDatabaseData();

public:	
	virtual TAny* GetObject();
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);	

protected:
	CT_CCommsDatabaseData();
	void ConstructL();

private:
	void DoCmdNewL();
	void DoCmdDestructor();
	void DestroyData();
	
private:
	/**
	 * Wrapped object
	 */
	CCommsDatabase* iCommsDat;
	
	};

#endif /*T_CCOMMSDATABASEDATA_H_*/
