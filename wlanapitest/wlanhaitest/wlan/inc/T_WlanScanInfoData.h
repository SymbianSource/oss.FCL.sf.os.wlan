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




#ifndef T_WLANSCANINFODATA_H_
#define T_WLANSCANINFODATA_H_

//User Includes
#include "datawrapperbase.h"

//forward class
class CWlanScanInfo;

class CT_WlanScanInfoData: public CDataWrapperBase
	{
public:
	static CT_WlanScanInfoData* NewL();
	~CT_WlanScanInfoData();
	
public:
	virtual TAny* GetObject();	
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);

protected:
	CT_WlanScanInfoData();
	void ConstructL();

private:
	 void DoCmdNewL(const TTEFSectionName& aSection);
	 void DoCmdDestructor();
	 void DestroyData();
	 void DoCmdInformationElement(const TTEFSectionName& aSection);

private:
	/**
	 * Wrapped object
	 */
	CWlanScanInfo* iData;
	/**
	 * For storing ECom instance UID (needed when destroying the instance)
	 */
	TUid iScanInfoInstanceIdentifier;
   };

#endif /*T_WLANSCANINFODATA_H_*/

   