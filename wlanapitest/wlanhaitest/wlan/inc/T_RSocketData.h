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



#ifndef T_RSOCKETDATA_H_
#define T_RSOCKETDATA_H_

//User Includes
#include "datawrapperbase.h"

//Epoc includes
#include <es_sock.h> // RSocketServ, RConnection
#include <in_sock.h> // KAfInet
#include <activecallback.h>
#include <f32file.h>


class CT_RSocketData: public CDataWrapperBase
	{
public:
	static CT_RSocketData* NewL();
	void RunL(CActive* aActive, TInt aIndex);
	virtual ~CT_RSocketData();

public:	
	virtual TAny* GetObject();
	void ErrorMessage(const TInt aMessage);
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);		

protected:
	CT_RSocketData();
	void ConstructL();

private:
	void DoCmdOpen(const TTEFSectionName& aSection);	
	void DoCmdConnect(const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	void DoCmdShutdown(const TInt aAsyncErrorIndex);			
	void DoCmdClose();
	void DoCmdHttpGet();
	void DoCmdCheckSupportedRates(const TTEFSectionName& aSection);
	void DoCmdUploadSendHTTPPost(const TTEFSectionName& aSection);
    void CreateHTTPHeaderStart(TDes8& aRequest, TInt aDataSize, TDesC& aFileServer,TDesC& clientID,TDesC& serverScript);
	void SendFileToSocketL(const TDesC& aFilename);
	TInt ReadFileSizeL(const TDesC& aFilename);
	void CreateHTTPHeaderEnd(TDes8& aRequest);
	TBool CheckSupportedRates(const TDesC8& aSupportedRates, const TUint8 aRate);
	void Shutdown();
	void DoCmdDownloadSendHTTPGet(const TTEFSectionName& aSection, const TInt aAsyncErrorIndex );
	void DoCmdRecvOneOrMore(const TTEFSectionName& aSection);	
	void Close();	
	TReal ThroughputInMegaBits( TTimeIntervalMicroSeconds aDuration, TInt aTotalTransferred );
	void RecvOneOrMore(TRequestStatus& status);	

private:
	/**
	 * Wrapped object
	 */
	RSocket*			iSocket;
	/**
	 * Used in the command DoCmdDownloadSendHTTPGet for RSocket::Write
	 */
	CActiveCallback* 	iActiveCallback;
	/**
	 * Used in the command DoCmdConnect for RSocket::Connect
	 */
	CActiveCallback* 	iActCallConnectSocket;
	/**
	 * Used in the command DoCmdConnectSocket for RSocket::Connect
	 */
	CActiveCallback* 	iActCallShutDownSocket;		
	/**
	 * Flag to review if the Socket is Open with RSocket::Connect
	 */
	TBool 				iSocketOpened;
	/**
	 * flag to review if the Socket was shutdown
	 */
	TBool 				iSocketStarted;		
	/**
	 * Async data
	 */
	TInt 			    iAsyncErrorIndex;
	/**
	 * Buffer for Download in DoCmdReceiveHTTPResponse command
	 */
	HBufC8* 			iDownloadBuffer;
	/**
	 * Buffer for Upload in SendFileToSocket 
	 */
	HBufC8* 			iUploadBuffer;
	/**
	 * Header for response HTPP
	 */
	RBuf8   			iHttpResponseHeader;
	/**
	 * Download throughput
	 */
	TReal 				iDownloadThroughput;
	/**
	 * Handle for Filse server session
	 */
	RFs 				iFs;
	/**
	 * Upload throughput
	 */
	TReal               iUploadThroughput;
	/**
	 * Bytes received in DoCmdHttpGet
	 */
	TInt                itotalReceived;
	};

#endif /*T_RSOCKETDATA_H_*/
