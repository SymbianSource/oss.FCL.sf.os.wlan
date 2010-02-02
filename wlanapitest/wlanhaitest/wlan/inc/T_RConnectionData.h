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




#ifndef T_RCONNECTIONDATA_H_
#define T_RCONNECTIONDATA_H_

//User Includes
#include "datawrapperbase.h"
 
//Epoc includes
#include <es_sock.h> //RConnection

class CT_RConnectionData: public CDataWrapperBase
	{
	public:
	static CT_RConnectionData* NewL();
	virtual ~CT_RConnectionData();

public:	
	virtual TAny* GetObject();
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);	

protected:
	CT_RConnectionData();
	void ConstructL();

private:
   void DoCmdOpen(const TTEFSectionName& aSection);
   void DoCmdStart(const TTEFSectionName& aSection);   
   void DoCmdClose();
   void Close();

private:
	/**
	 * Wrapped object
	 */
	RConnection* 			iConnection;
	};
#endif /*T_RCONNECTIONDATA_H_*/
