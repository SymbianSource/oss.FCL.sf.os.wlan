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



#include <wlanscaninfo.h>
#include "t_rsocketdata.h"

/*@{*/
// LITs from the ini file
_LIT( KSocketServ,				"socketserv");
_LIT( KConnection,				"connection");
_LIT( KScanInfo,				"scaninfo");
_LIT( KAddress,					"Ip");
_LIT( KPort,					"Port");
_LIT( KFile,					"File");
_LIT( KSave,					"Save");
_LIT( KARates,					"rate");
_LIT( KFileServer,				"FileServer");
/*@}*/

/*@{*/
// Upload
_LIT(KNullFile,				    "KNullDesC");
/*@}*/

/*@{*/
// LITs for the commands
_LIT( KCmdOpen,					"Open");
_LIT( KCmdConnect,				"Connect");
_LIT( KCmdHttpGet,				"HttpGet");
_LIT( KCmdDownloadSendHTTPGet,	"DownloadSendHTTPGet");
_LIT( KCmdRecvOneOrMore,     	"RecvOneOrMore");
_LIT( KCmdUploadSendHTTPPost,	"UploadSendHTTPPost");
_LIT( KCmdCheckSupportedRates,	"CheckSupportedRates");
_LIT( KCmdShutdown,				"Shutdown");
_LIT( KCmdClose,				"Close");
/*@}*/

/*@{*/
// Constants for creating a HTTP request in the command DoCmdDownloadSendHTTPGet
_LIT8( KHTTPGET, 				"GET");
_LIT8( KHTTPSeparator, 			" ");
_LIT8( KHTTPSuffix, 			"HTTP/1.1");
_LIT( KHostS, 					"Host");
_LIT8( KLineFeed,				"\r\n");
_LIT8( KEmptyLine, 				"\r\n\r\n");
_LIT8( KHeaderEndMark, 			"\r\n\r\n" );
_LIT8( KContentLengthField,		"Content-Length: ");
_LIT8( KFieldEnd, 				"\r\n" );
_LIT8( KGETHTTP, 				"GET / HTTP/1.0\r\n\r\n" );
/*@}*/

/*@{*/
// Constants for CreateHTTPHeaderStart
_LIT8(KHTTPPOST, 				"POST");
_LIT8(KLineBreak,				"\r\n");
_LIT(KClientID,					"clientID");
_LIT(KServerScript,				"serverScript"); 
_LIT8(KFrom,					"From:");
_LIT8(KHosts,					"Host:");
_LIT8(KContentType,				"Content-Type:");
_LIT8(KContentLength,			"Content-Length:");
_LIT8(KContentDisposition,		"Content-Disposition:");
_LIT8(KMultipartType,			"multipart/form-data;");
_LIT8(KOctetType,				"application/octet-stream");
_LIT8(KBoundary,				"boundary=---------------------------sg976436h73");
_LIT8(KBoundaryStart,			"-----------------------------sg976436h73");
_LIT8(KDisposition,				"form-data; name=\"userfile\"; filename=");
_LIT8(KBackS,					"\"");
_LIT8(KBoundaryEnd,				"-----------------------------sg976436h73--");
/*@}*/


const TInt KHttpHeaderBufferIncrement = 4096;
// Const for supported rates
// The first bit includes information about BSSBasicRateSet,
// mask it out

const TUint32 KBasicRateMask = 0x7F;
// 802.11g supported speed rate
const TUint8 K80211Rate1Mbit = 2;
const TUint8 K80211Rate2Mbit = 4;            
const TUint8 K80211Rate5Mbit = 11;
const TUint8 K80211Rate11Mbit = 22;
const TUint8 K80211Rate12Mbit = 24;
const TUint8 K80211Rate18Mbit = 36;
const TUint8 K80211Rate22Mbit = 44;
const TUint8 K80211Rate24Mbit = 48;
const TUint8 K80211Rate33Mbit = 66;
const TUint8 K80211Rate36Mbit = 72;
const TUint8 K80211Rate48Mbit = 96;
const TUint8 K80211Rate54Mbit = 108;







/**
 * Two phase constructor
 *
 * @leave	system wide error
 */
CT_RSocketData* CT_RSocketData::NewL()
	{
	CT_RSocketData * ret = new (ELeave)CT_RSocketData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

/*
 *RunL method for management Active callbacks
 * @param aActive	param to review which active call back is being fished
 * @param aIndex
 * @return void
 */
void CT_RSocketData::RunL(CActive* aActive, TInt /*aIndex*/)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::RunL"));
    DecOutstanding(); // One of the async calls has completed 
    TInt err(KErrNone);
    if(aActive == iActiveCallback)
    	{
        INFO_PRINTF1(_L("active call back for Write Socket."));
    	err = iActiveCallback->iStatus.Int();
    	if(err != KErrNone)
    		{
    		ERR_PRINTF1(_L("iSocket->Write(...) Fail"));
    		SetError(err);
    		}
    	else
    		{
    		INFO_PRINTF2(_L("CT_RSocketData::SendHTTPGet [%d]"), iActiveCallback->iStatus.Int());	
        	INFO_PRINTF1(_L("Asynchronous task has completed. RunL  called"));
    		}    	
    	}
    else if(aActive == iActCallConnectSocket)
    	{
        INFO_PRINTF1(_L("active call back for Connect Socket."));
    	err = iActCallConnectSocket->iStatus.Int();
    	if(err != KErrNone)
    		{
    		ERR_PRINTF1(_L("iSocket->Connect(...) Fail"));
    		SetError(err);
    		}
    	else
    		{
    		INFO_PRINTF1(_L("CT_RSocketData::DoCmdConnect(...) success"));
    		iSocketOpened = ETrue;
    		}
    	}
    else if(aActive == iActCallShutDownSocket)
    	{
        INFO_PRINTF1(_L("active call back for Shutdown Socket."));
    	err = iActCallShutDownSocket->iStatus.Int();
    	if(err != KErrNone)
    		{
    		 ERR_PRINTF2(_L("iSocket->Shutdown(...): [%d] Fail"),iActCallShutDownSocket->iStatus.Int());
    		 SetError(err);
    		}
    	else
    		{
    		INFO_PRINTF1(_L("CT_RSocketData::Shutdown success"));
    		iSocketStarted = EFalse;	
    		}
    	}
    else
    	{
    	ERR_PRINTF1(_L("An unchecked active object completed"));
    	SetBlockResult(EFail);
    	}
    
    INFO_PRINTF1(_L("*END* CT_RSocketData::RunL"));
	}
/*
 * public destructor
 */
CT_RSocketData::~CT_RSocketData()
	{
	if (iSocketStarted)
		{
		INFO_PRINTF1(_L("CT_RSocketData: Shutting down socket"));
		Shutdown();
		}
	if (iSocketOpened)
		{
		Close();
		}
	if (iDownloadBuffer)
		{
		delete iDownloadBuffer;
		iDownloadBuffer = NULL;
		}
	if (iSocket)
		{
		delete iSocket;
		iSocket = NULL;	
		}
	if (iActiveCallback)
		{
		delete iActiveCallback;
		iActiveCallback = NULL;	
		}
	if (iActCallShutDownSocket)
		{
		delete iActCallShutDownSocket;
		iActCallShutDownSocket = NULL;
		}
	if (iActCallConnectSocket)
		{
		delete iActCallConnectSocket;
		iActCallConnectSocket =	 NULL;
		}
	
	iFs.Close();

	if (iUploadBuffer)
		{
		delete iUploadBuffer;
		iUploadBuffer = NULL;
		}
	}

/**
 * Private constructor. First phase construction
 * 
 */
CT_RSocketData::CT_RSocketData()
:	iSocket(NULL),
	iActiveCallback(NULL),
	iActCallConnectSocket(NULL),
	iActCallShutDownSocket(NULL),	
	iSocketOpened(EFalse),
	iSocketStarted(EFalse),	
	iAsyncErrorIndex(0),
	iDownloadBuffer(NULL),
	iUploadBuffer(NULL),
	iHttpResponseHeader(),
	iDownloadThroughput(0.0),	
	iFs(),
	iUploadThroughput(0.0),
	itotalReceived(0)
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
void CT_RSocketData::ConstructL()
	{	 
   	 const TInt KDefaultBufferSize = 4096;
   	 TInt err(KErrNone);
   	 iSocket = new (ELeave) RSocket(); 
	 iActiveCallback = CActiveCallback::NewL(*this);
	 iActCallConnectSocket = CActiveCallback::NewL(*this);
	 iActCallShutDownSocket = CActiveCallback::NewL(*this);		
	 iDownloadBuffer = HBufC8::NewL( KDefaultBufferSize);		
	 iUploadBuffer = HBufC8::NewL(KDefaultBufferSize);		 
	 err = iFs.Connect();
	 if(err != KErrNone)
		 {
		 SetError(err);
		 }
	}


/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
TAny* CT_RSocketData::GetObject()
	{
	return iSocket;
	}



/**
 * Process a command read from the Ini file
 * @param aCommand 			The command to process
 * @param aSection			The section get from the *.ini file of the project T_Wlan
 * @param aAsyncErrorIndex	Command index for async calls to returns errors to
 * @return TBool			ETrue if the command is process
 * @leave					system wide error
 * 
 */	
TBool CT_RSocketData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret =  ETrue;		
	if(aCommand == KCmdOpen )
		{
		DoCmdOpen(aSection);
		}
	else if(aCommand == KCmdConnect)
		{
		DoCmdConnect(aSection,aAsyncErrorIndex);
		}
	else if(aCommand == KCmdDownloadSendHTTPGet)
		{
		DoCmdDownloadSendHTTPGet(aSection,aAsyncErrorIndex);
		}
	else if(aCommand == KCmdRecvOneOrMore)
		{
		DoCmdRecvOneOrMore(aSection);
		}
	else if(aCommand == KCmdUploadSendHTTPPost)
		{
		DoCmdUploadSendHTTPPost(aSection);
		}
	else if(aCommand == KCmdShutdown)
		{
		DoCmdShutdown(aAsyncErrorIndex);
		}
	else if(aCommand == KCmdClose)
		{
		DoCmdClose();
		}
	else if(aCommand == KCmdHttpGet)
		{
		DoCmdHttpGet();
		}
	else if(aCommand == KCmdCheckSupportedRates)
		{
		DoCmdCheckSupportedRates(aSection);
		}	
	else
		{
		ERR_PRINTF1(_L("Unknown command."));
		ret = EFalse;
		}
	return ret;
	}




/**
 * Open the Socket from RSocket. The errors are management with SetError() and SetBlockResult().
 * @param aSection				Section in the ini file for this command.
 * @return 
 */
void CT_RSocketData::DoCmdOpen(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdOpen"));
	TBool dataOk = ETrue;
	
	TPtrC connectionName;
	if(! GetStringFromConfig(aSection, KConnection, connectionName))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KConnection);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	TPtrC socketServName;
	if(! GetStringFromConfig(aSection, KSocketServ, socketServName))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KSocketServ);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	if (dataOk)
		{
		INFO_PRINTF1(_L("Opening a TCP/IP socket"));
		
		RConnection* rConnection = static_cast<RConnection*>(GetDataObjectL(connectionName)); 	
		RSocketServ* rSocketServ = static_cast<RSocketServ*>(GetDataObjectL(socketServName));
		
		if(rConnection != NULL && rSocketServ != NULL)
			{
			TInt error = iSocket->Open( *rSocketServ, KAfInet, KSockStream, KProtocolInetTcp, *rConnection );
			
			if(error == KErrNone)
				{
				iSocketOpened = ETrue;
				}
			else
				{
				ERR_PRINTF2(_L("Socket opening failed [%d]"), error);
				SetError(error);
				}
			}
		else
			{
			if(rConnection == NULL)
				{
				ERR_PRINTF2(_L("rConnection is NULL: %S"),rConnection);
				SetBlockResult(EFail);
				}

			if(rSocketServ == NULL) 
				{
				INFO_PRINTF2(_L("rSocketServ is NULL: %S"),rSocketServ);
				SetBlockResult(EFail);
				}
			}
		}

	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdOpen"));
	}

/**
 * Command to Connect a Socket of RSocket.
 * @param aSection				Section to read from the ini file
 * @param aAsyncErrorIndex      Command index for async calls to returns errors to
 * @return
 */
void CT_RSocketData::DoCmdConnect(const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdConnect"));
	TBool dataOk = ETrue;
	
	//Getting from the .ini the IP Address
	TPtrC aIpAddr;
	if(!GetStringFromConfig( aSection, KAddress, aIpAddr ))
    	{
    	ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KAddress);
    	SetBlockResult(EFail);
    	dataOk = EFalse;
    	}
	
	//Getting the port from the file ini
	TInt aPort;
	if(!GetIntFromConfig( aSection, KPort,aPort ))
    	{
    	ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KPort);
    	SetBlockResult(EFail);
    	dataOk = EFalse;
    	}
	
	if(dataOk)
		{
		// Set the IP Address
		TInetAddr inetAddr;
		TInt err = inetAddr.Input( aIpAddr ) ;
		if(err == KErrNone)
			{
			INFO_PRINTF2(_L("Remote IP: %S"), &aIpAddr );
			INFO_PRINTF2( _L("Port: %d"), aPort );
			// Set the port
			inetAddr.SetPort( aPort );	
			// Connect an IP through the Port 80
			iSocket->Connect( inetAddr, iActCallConnectSocket->iStatus );
			iActCallConnectSocket->Activate(aAsyncErrorIndex);
			IncOutstanding();
			}
		else
			{
			 ERR_PRINTF2(_L("inetAddr.Input( aIpAddr ) Failed with error %d"), err);
			 SetError(err);
			}
		}
	
	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdConnect"));
	}

/**
 * Command to send the HTTP Get, using the socket Write.
 * @param aSection				Section to read from the ini file
 * @param aAsyncErrorIndex		Command index for async calls to returns errors to
 * @return 
 */
void CT_RSocketData::DoCmdDownloadSendHTTPGet(const TTEFSectionName& aSection, const TInt aAsyncErrorIndex )
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdDownloadSendHTTPGet"));
	TBool dataOk = ETrue;
	
	// Read params from the ini file
	TPtrC aHost;	
	if(!GetStringFromConfig( aSection, KHostS, aHost))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KHostS);
    	SetBlockResult(EFail);
    	dataOk = EFalse;
		}

	TPtrC aFilename;
	if(!GetStringFromConfig( aSection, KFile, aFilename ))
    	{
    	ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KFile);
    	SetBlockResult(EFail);
    	dataOk = EFalse;
    	}

	if (dataOk)
		{
		const TInt KMaxHostNameLength(256);
		if( aHost.Length() > KMaxHostNameLength )
			{
			ERR_PRINTF1(_L("Host is too long, cannot send HTTP request"));
	    	SetBlockResult(EFail);
			}
		else if( aFilename.Length() > KMaxFileName )
			{
			ERR_PRINTF1(_L("Filename is too long, cannot send HTTP request"));
	    	SetBlockResult(EFail);
			}
		else
			{
			INFO_PRINTF1(_L("Create HTTP GET request"));
			// Buffer that will hold the request.
			TBuf8<	sizeof( KHTTPGET ) +
					sizeof( KHTTPSeparator ) +
					KMaxFileName +
					sizeof( KHTTPSeparator ) +
					sizeof( KHTTPSuffix ) +
					sizeof( KLineFeed ) +
					sizeof( KHosts ) +
					KMaxHostNameLength +
					sizeof( KEmptyLine ) > request;
			// Construct the final request.
			request.Copy( KHTTPGET );
			request.Append( KHTTPSeparator );
			request.Append( aFilename );
			request.Append( KHTTPSeparator );
			request.Append( KHTTPSuffix );
			request.Append( KLineFeed );
			request.Append( KHosts );
			request.Append( aHost );
			request.Append( KEmptyLine );
			
			INFO_PRINTF1(_L("Write to socket"));
		    // Send the request through socket
			iSocket->Write(request, iActiveCallback->iStatus);
			iActiveCallback->Activate(aAsyncErrorIndex);
			IncOutstanding();
			}
		}
	
	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdDownloadSendHTTPGet"));
	}

/**
 * Command to receive an HTTP Response for Upload and Download of files.
 * @param aSection				Section to read from the ini file
 * @return
 */
void CT_RSocketData::DoCmdRecvOneOrMore(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdRecvOneOrMore"));
	TBool dataOk = ETrue;
	
	// Read from the ini file
	TPtrC aFilename;
	if(!GetStringFromConfig( aSection, KSave,aFilename ))
    	{
    	ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KSave);
    	SetBlockResult(EFail);
    	dataOk = EFalse;
    	}
	
	if (dataOk)
		{
		RFile file;
		TInt error = KErrNone;	
		
		//if KNullFile then Upload
		TBool discardData = ( aFilename == KNullFile);
		INFO_PRINTF2(_L("File and path to Download: %S"),&aFilename);
		if( !discardData )
			{
			INFO_PRINTF1( _L("Data is not discarded, creating file") );
			error = file.Replace( iFs, aFilename, EFileShareAny|EFileWrite );	
			}
		else
			{
			INFO_PRINTF1( _L("Discarding downloaded data") );
			}
		
		if( error == KErrNone )
			{
			TSockXfrLength received;
			TInt totalReceived = 0;
			TInt contentReceived = 0;
			TInt timedReceived = 0;
			TInt contentLength = 0;
		    TRequestStatus status;
		    TPtr8 downloadBufferPtr( iDownloadBuffer->Des() );
			
		    downloadBufferPtr.SetMax();
		    INFO_PRINTF2( _L("Using buffer size [%d]"), downloadBufferPtr.MaxSize() );

			INFO_PRINTF1(_L("Set time stamps for download"));
			TTime endTime;
			TTime startTime;

		    INFO_PRINTF1( _L("Receiving data"));
		    
		    // Let's assume that we receive a HTTP header first
		    TBool header( ETrue );	
			TBool timerStarted( EFalse );
			TBool failure = EFalse; // a flag to delete multiple returns
			
			iHttpResponseHeader.Zero();	
			// receive until RecvOneOrMore fails or all content is received
			do
				{
				if( !timerStarted && !header)
					{
					startTime.HomeTime();
					endTime.HomeTime();
					timerStarted = ETrue;
					}
				
				iSocket->RecvOneOrMore( downloadBufferPtr, 0, status, received );
				User::WaitForRequest( status );
				if( !header )
					{
					timedReceived += received();
					}
				
				if( KErrNone == status.Int() )
					{
					// Check if we are still receiving the HTTP header
					if( header )
						{
						//Increase httpResponseheader size if needed
						if(iHttpResponseHeader.Length() + downloadBufferPtr.Length() > iHttpResponseHeader.MaxLength())
							{
							error = iHttpResponseHeader.ReAlloc(iHttpResponseHeader.MaxLength() + KHttpHeaderBufferIncrement);
							if(error != KErrNone)
								{	
								ERR_PRINTF2( _L("iHttpResponseHeader.ReAlloc(...) Failed with error %d"), error);
								SetError( error );
								failure = ETrue;
								break;
								}
							}
						
						//Append the donwloaded content to headerbuffer
						iHttpResponseHeader.Append(downloadBufferPtr);								
						TInt headerEndIndex = iHttpResponseHeader.Find( KHeaderEndMark );
						if( headerEndIndex != KErrNotFound )
							{
							INFO_PRINTF1( _L("Header end mark found"));
							//Parse Content-Length field and extract content length					
							TInt contentLengthStart = iHttpResponseHeader.Find( KContentLengthField );
							//If Content-Length field is found
							if( contentLengthStart != KErrNotFound )
								{
							    INFO_PRINTF1(_L("Content-Length field found from HTTP response"));
								contentLengthStart += KContentLengthField().Length();					
								TPtrC8 contentLengthDes;
								contentLengthDes.Set(iHttpResponseHeader.Mid( contentLengthStart ));											
								TInt contentLengthEnd = contentLengthDes.Find( KFieldEnd );
								contentLengthDes.Set(contentLengthDes.Mid(0, contentLengthEnd));					
								TLex8 lex;
								lex.Assign( contentLengthDes );
								lex.Val(contentLength);						
								INFO_PRINTF2( _L("Content-Length: [%d]"), contentLength );						
								}
							else
								{
								INFO_PRINTF1( _L("No Content-Length field found from HTTP response"));
								INFO_PRINTF1( _L("Assuming Content-Length: 0"));
								contentLength = 0;
								file.Close();
								error = iFs.Delete(aFilename);
								if(error != KErrNone)
									{
									INFO_PRINTF3(_L("Error [%d] for delete the file %S"), &aFilename,error);
									SetError(error);
									failure = ETrue;
									break;
									}
								ERR_PRINTF2(_L("File %S was not found"), &aFilename);
								SetBlockResult(EFail);
								failure = ETrue;
								break;
								}															
							// Header was found
							headerEndIndex += KHeaderEndMark().Length();
							//Convert the headerEndIndex in httpResponseheader to index in downloadBuffer
							headerEndIndex -= totalReceived;					
							//Delete remaining parts of the HTTP header from the download buffer
							downloadBufferPtr.Delete( 0, headerEndIndex );					
							header = EFalse;
							}
						}

					// Increase the total received amount as we receive more data.
					// Note: received data count also counts headers, this is taken
					// into account in timing (startTime)
					totalReceived += received();			
					if(!header)
						{
						contentReceived += downloadBufferPtr.Length();
						}
					
					if( !discardData )
						{
						error = file.Write( *iDownloadBuffer );
						if( KErrNone != error )
							{
							ERR_PRINTF2( _L("Failed to write local file [%d]"), error );
							file.Close();
							SetError(error);
							failure = ETrue;
							break;
							}
						}
					}
				else
					{
					INFO_PRINTF1(_L("Set end time"));
					endTime.HomeTime();			
					INFO_PRINTF2( _L("Receiving err [%d]"), status.Int());
					break;
					}
				}
			while( KErrNone == status.Int() && contentReceived < contentLength );
			
			if (!failure)
				{
				endTime.HomeTime();	
				INFO_PRINTF2( _L("Received total of [%d] bytes (inc headers)"), totalReceived );
				INFO_PRINTF2( _L("Content received [%d] bytes"), contentReceived );

				//Set this printing optional
				//Print only if any amount of datatransfer was timed (skipped in the case of very short data transfers)
				if( timerStarted )
					{
					INFO_PRINTF1(_L("Calculate duration of the transfer"));
					TTimeIntervalMicroSeconds duration = endTime.MicroSecondsFrom( startTime );
					INFO_PRINTF2( _L("Duration for the timed data transfer was [%Ld] microseconds"), duration.Int64() );		
					INFO_PRINTF2( _L("Received [%d] bytes during timed data transfer"), timedReceived);		
					iDownloadThroughput = ThroughputInMegaBits( duration, timedReceived );
					}
				else
					{
					INFO_PRINTF1( _L("Data transfer too short for throughput calculation"));
					}
				
				// We allow any response to our reply at the moment.
				if( !discardData )
					{
					file.Close();
					}
				}
			}
		else
			{
			ERR_PRINTF2( _L("Failed to open local file [%d]"), error );
			SetError(error);
			}
		}
	
	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdRecvOneOrMore"));	
	}

/**
 * Create an HTTP Post for uploading files.
 * @param aSection  Section to read from the ini file
 * @return 
 */
void CT_RSocketData::DoCmdUploadSendHTTPPost(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdUploadSendHTTPPost"));
	TBool dataOk = ETrue;
	
	INFO_PRINTF1( _L("Write to socket"));

	TPtrC aFilename;
	if(!GetStringFromConfig(aSection,KFile,aFilename))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KFile);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	TPtrC fileServer;
	if(!GetStringFromConfig(aSection,KFileServer,fileServer))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KFileServer);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	TPtrC clientID;
	if(!GetStringFromConfig(aSection,KClientID,clientID))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KClientID);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	TPtrC serverScript;
	if(!GetStringFromConfig(aSection,KServerScript,serverScript))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KServerScript);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	
	if (dataOk)
		{
		const TInt KMaxTag = 256;
		// KHeaderWithoutData will change if you alter the header in any way that changes the 
		// amount of characters in it! SO REMEMBER to calclulate header size again.
		const TInt KHeaderWithoutData = 200;
		TBuf8<KMaxTag + KHeaderWithoutData> request;
		TRequestStatus status;
		
		CreateHTTPHeaderStart(request, ReadFileSizeL(aFilename),fileServer, clientID, serverScript);
			
		iSocket->Write( request,status);
		User::WaitForRequest( status );
		if(status.Int() == KErrNone)
			{
			INFO_PRINTF1( _L("HTTP POST request send, sending payload next"));
			// Send file to iSocket
			SendFileToSocketL(aFilename);
			request.SetLength( 0 );
			CreateHTTPHeaderEnd(request);

			// Send the rest of the header
			INFO_PRINTF1(_L("Sending boundary end"));
			iSocket->Write( request, status );
			User::WaitForRequest( status );
			if(status.Int() != KErrNone)
				{
				 ERR_PRINTF2(_L("CT_RSocketData::DoCmdUploadSendHTTPPost: iSocket->Write( request,status) Failed with error %d"), status.Int());
				 SetError(status.Int());
				}
			}
		else
			{
			 ERR_PRINTF2(_L("CT_RSocketData::DoCmdUploadSendHTTPPost: iSocket->Write( request,status) Failed with error %d"), status.Int());
			 SetError(status.Int());
			}
		}

	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdUploadSendHTTPPost"));
	}

/**
 *  Create or build the header for POST.
 * @param aRequest				Descriptor with a lenght of 456 that contain the parameters for the POST
 * @param aDataSize             Size of the file
 * @return
 */
void CT_RSocketData::CreateHTTPHeaderStart(TDes8& aRequest, TInt aDataSize,TDesC& aFileServer, TDesC& clientID,TDesC& serverScript)
	{
	// Manually created HTTP Post request is difficult to maintain.
	// Request and server responce is logged into file during test run.
	
	// KHeaderWithoutData will change if you alter the header in any way
	// that changes the amount of characters in it! SO REMEMBER to calclulate
	// header size again.
	const TInt KHeaderWithoutData = 200;	
	INFO_PRINTF1( _L("Set socket remote name"));
	TSockAddr address;
	iSocket->RemoteName( address );

	// Construct request
	aRequest.Append(KHTTPPOST);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(serverScript);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(KHTTPSuffix);
	aRequest.Append(KLineBreak);

	aRequest.Append(KHosts);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(address);
	aRequest.Append(KLineBreak);

	aRequest.Append(KFrom);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(clientID);
	aRequest.Append(KLineBreak);

	aRequest.Append(KContentType);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(KMultipartType);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(KBoundary);
	aRequest.Append(KLineBreak);

	aRequest.Append(KContentLength);
	aRequest.Append(KHTTPSeparator);
	// aRequest size + size of the data to be sent. Server must know how much
	// data is coming.
	aRequest.AppendNum(KHeaderWithoutData+aDataSize);
	aRequest.Append(KLineBreak);

	// extra line break
	aRequest.Append(KLineBreak);

	aRequest.Append(KBoundaryStart);
	aRequest.Append(KLineBreak);

	aRequest.Append(KContentDisposition);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(KDisposition);
	aRequest.Append(KBackS);
	aRequest.Append(aFileServer);
	aRequest.Append(KBackS);
	aRequest.Append(KLineBreak);

	aRequest.Append(KContentType);
	aRequest.Append(KHTTPSeparator);
	aRequest.Append(KOctetType);
	aRequest.Append(KLineBreak);
	
	aRequest.Append(KLineBreak);
	}

/**
 * Send aFilename parameter to the Socket with RSocket::Write
 * @param aFilename			name of the file send to the Socket
 * @return
 */
void CT_RSocketData::SendFileToSocketL(const TDesC& aFilename)
	{
	TInt err(KErrNone);
	TPtr8 buffer( iUploadBuffer->Des() );
	buffer.SetMax();
    INFO_PRINTF2( _L("Using buffer size [%d]"), buffer.MaxSize() );
    TInt bytesSent = 0;

	INFO_PRINTF1( _L("Open file"));
    RFile file;
    
    err = file.Open(iFs, aFilename, EFileShareAny|EFileRead);
    
    if(err == KErrNone)
    	{
        CleanupClosePushL( file );
        INFO_PRINTF1(_L("Read file size"));
        TInt fileSize = ReadFileSizeL(aFilename);

        INFO_PRINTF1( _L("Set time stamps for upload"));
    	TTime endTime;
    	endTime.HomeTime();
    	TTime startTime;
    	startTime.HomeTime();

    	INFO_PRINTF1( _L("Send file"));
        // Loop while enough bytes are sent to socket
        while( bytesSent < fileSize )
            {
            TInt err = file.Read( buffer );

            if( err == KErrEof )
    			{
    			INFO_PRINTF1(_L("File sending finished"));
    			INFO_PRINTF2( _L("Upload buffer length is [%d]"), buffer.Length());
    			break;
    			}
    		else if( err != KErrNone )
    			{
    			ERR_PRINTF2( _L("Failed to read file [%d]"), err );
    			SetError( err );
    			break;
    			}

    	    TRequestStatus status(KRequestPending);
    		iSocket->Write( buffer, status );
    		User::WaitForRequest( status );		
    		err = status.Int();
    		if(err != KErrNone)
    			{
    			ERR_PRINTF2(_L("CT_RSocketData::SendFileToSocketL:iSocket->Write(...) Fail [%d] "),err);
    			SetError(err);
    			break;
    			}
    		
            bytesSent += ( buffer.Length() );
            }

        if (err == KErrNone || err == KErrEof)
        	{
        	INFO_PRINTF1( _L("Set end time"));
        	endTime.HomeTime();
        	INFO_PRINTF2( _L("Sent [%d] bytes to server"), bytesSent);

        	INFO_PRINTF1( _L("Calculate duration of the transfer"));
        	TTimeIntervalMicroSeconds duration = endTime.MicroSecondsFrom( startTime );
        	INFO_PRINTF2( _L("Duration for the data transfer was [%Ld] microseconds"), duration.Int64() );
        	iUploadThroughput = ThroughputInMegaBits( duration, bytesSent );
            CleanupStack::PopAndDestroy( &file );
        	}
    	}
    else
    	{
    	ERR_PRINTF2(_L("CT_RSocket::SendFileToSocketL::file.Open(...) Failed with error %d"), err);
    	SetError(err);
    	}
	}

/**
 * Calculated the throughput based on duration of a data transfer and total transferred bytes.
 * @param aDuration				Duration of the transfer
 * @param aBytes				Total transferred in bytes
 * @return 						Throughput in MBps
 */
TReal CT_RSocketData::ThroughputInMegaBits(TTimeIntervalMicroSeconds aDuration, TInt aBytes )
	{
	const TReal KBitsInByte(8.0);
	TReal throughput = ( KBitsInByte * (TReal) aBytes ) / (TReal) aDuration.Int64();
	return throughput;
	}

/**
 * Read the lenght of the file (aFileName)
 * @param aFileName  file to read the lenght
 * @return
 */
TInt CT_RSocketData::ReadFileSizeL(const TDesC& aFilename)
	{
	RFile file;
    TInt error = file.Open(iFs, aFilename, EFileShareAny|EFileRead);    
    if ( error != KErrNone)
    	{
    	ERR_PRINTF2( _L("Failed to open local file [%d]"), error);
    	SetError(error);
    	return error;
    	}

    TInt fileSize = 0;
    error = file.Size(fileSize);
    
    if (error!= KErrNone)
    	{
    	ERR_PRINTF2(_L("Failed to read file size [%d]"), error);
    	file.Close();
    	SetError(error);
    	return error;
    	}

    file.Close();
    return fileSize;
	}

/**
 * Build the final header to POST for uploading files
 * @param aRequest				Descriptor with 456 of lenght that contain the final POST request
 * @return
 */
void CT_RSocketData::CreateHTTPHeaderEnd(TDes8& aRequest)
	{	
	//TRequestStatus status;
	aRequest.SetLength( 0 );
	//Create the rest of the header data
	aRequest.Append( KLineBreak );
	aRequest.Append( KBoundaryEnd );
	aRequest.Append( KLineBreak );
	}


/**
 * Make a HTTP request to the socket
 * @param
 * @return
 */
void CT_RSocketData::DoCmdHttpGet()
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdHttpGet"));
	
	 TInt err(KErrNone);
	//Constant for creating a HTTP request.
	const TInt KHTTPSize = 128;
	// Buffer that will hold the request.
	TBuf8 <KHTTPSize> request;
	// Construct the final request.
	request.Append( KGETHTTP );
	
	INFO_PRINTF1( _L("Write to socket") );
    TRequestStatus status( KRequestPending );	
	iSocket->Write( request, status);
    User::WaitForRequest( status );    
	INFO_PRINTF2( _L("CT_RSocketData::DoCmdHttpGet: Write done: [%d]"), status.Int() );
    err = status.Int();
    
    if(err == KErrNone)
    	{
    	INFO_PRINTF1( _L("CT_RSocketData::DoCmdHttpGet: Receive from socket") );
    	// receive until RecvOneOrMore fails
    	do
    		{
    		RecvOneOrMore(status);
    		}
    	while( status.Int() == KErrNone );

    	INFO_PRINTF2( _L("CT_RSocketData::DoCmdHttpGet: Receiving finished. Received [%d] bytes in total"), itotalReceived );

    	// Currently all error codes returned by the server are accepted.
    	// Should only KErrEof be accepted?
    	INFO_PRINTF2( _L("Ignoring error code from RSocket::RecvOneOrMore [%d]"), status.Int());
    	}
    else
    	{
    	 ERR_PRINTF2(_L("CT_RSocketData::DoCmdHttpGet: iSocket.Write(...) Failed with error %d"), err);
    	 SetError(err);
    	}
    	
	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdHttpGet"));
	}

/**
 * Receive data from a remote host.
 * @param status				Indicates the complexion status of the request
 * @return 
 */
void CT_RSocketData::RecvOneOrMore(TRequestStatus &status)
	{
	TInt err(KErrNone);
	// Create variables for receive buffer and received data counting variables.	
	const TInt KBufferSize(1024);
	TBuf8<KBufferSize> buffer;
	TSockXfrLength received;
	iSocket->RecvOneOrMore( buffer, 0, status, received);
	User::WaitForRequest( status );			
	err = status.Int();
	if( err == KErrNone )
		{
		INFO_PRINTF2( _L("CWlanTestWrapper: Received [%d] bytes"), received() );
		itotalReceived += received();
		}			
	else if( err == KErrEof )
		{
		INFO_PRINTF1(_L("End of File reached"));
		}
	else
		{
		ERR_PRINTF2(_L("RecvOneOrMore async call failed with error %d"), err);
		SetError(err);
		}	
	}



/**
 * Check the supported rates for the IAP.
 * @param aSection				Section to read from the ini file
 * @return
 */
void CT_RSocketData::DoCmdCheckSupportedRates(const TTEFSectionName& aSection)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdCheckSupportedRates"));
	TBool dataOk = ETrue;
	
    // Read from the ini file
    TInt aRate;
	if(!GetIntFromConfig(aSection,KARates,aRate))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KARates);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	
	// Check if a scan has been made
	TPtrC iScanInfoName;
	if(!GetStringFromConfig(aSection,KScanInfo,iScanInfoName ))
		{
		ERR_PRINTF2(_L("Error in getting parameter %S from INI file"), &KScanInfo);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	
	if (dataOk)
		{
		CWlanScanInfo* iScanInfo = static_cast<CWlanScanInfo*>(GetDataObjectL(iScanInfoName));
		
		// Check if a scan has been made
		if( iScanInfo != NULL )
			{
			const TUint8 KTemp80211SupRatesId = 1;
			const TUint8 KTemp80211SupRatesMaxLen = 18;
			// Scan info gives data as "information elements"
			TUint8 ieLen(0);
			const TUint8* ieData(0);
			
			TInt err = iScanInfo->InformationElement( KTemp80211SupRatesId, ieLen, &ieData );
			
			// Check supported rate if the information element was available
			if(err == KErrNone)
				{
				TBuf8<KTemp80211SupRatesMaxLen> supRates8;
				supRates8.Copy( ieData, ieLen );
				TBool supported = CheckSupportedRates( supRates8, aRate );
				if(!supported)
					{
					ERR_PRINTF2( _L("%d rate not supportedRates"), aRate );
					SetError(KErrNotSupported);
					}
				}
			else
				{
				ERR_PRINTF2( _L("err: [%d]"), err );		
			    SetError(err);
				}
			}
		else
			{
			ERR_PRINTF1(_L("Failed to get CWlanScanInfo object"));
			SetBlockResult(EFail);
			}
		}

	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdCheckSupportedRates"));
	}

/**
 * Review if the rate its supported.
 * @param aSupportedRates				Rate to calculate and if match with the desired rate
 * @param aRate							rate to verify if is supporrted, The rate to be checked in 0.5Mb/s units.
 *                                      Ie. 2 = 2 * 0.5Mb/s = 1Mb/s.
 * @return								Etrue if the rate is supported
 */
TBool CT_RSocketData::CheckSupportedRates(const TDesC8& aSupportedRates, const TUint8 aRate)
	{
	// Supported rates information element format is the following:
	// | element id (1 octet) | length (1 octet) | supported rates (1-8 octets) |
	// where each octet of supported rates contains one supported rate in
	// units of 500 kb/s. The first bit of supported rates field is always 1
	// if the rate belongs to the BSSBasicRateSet, if the rate does not belong
	// to the BSSBasicRateSet the first bit is 0.

	// For example Supported rates information element with value
	// 0x01,0x02,0x82,0x84
	// would mean that BSSBasicRateSet rates 1Mb/s and 2Mb/s are supported

	TBool supported( EFalse );
	
	for ( TInt i( 0 ); i < aSupportedRates.Length(); i++ )
	    {
	    TUint8 rate = aSupportedRates[i] & KBasicRateMask;
	    if( rate == aRate ) supported = ETrue;	    
		//INFO_PRINTF2( _L("speed rate [%d]"), rate);
    	switch( rate )
    		{
    		case K80211Rate1Mbit:
				INFO_PRINTF1( _L("AP can support Speed Rate 1Mbit") );
	    	    break;
    		case K80211Rate2Mbit:
    		    INFO_PRINTF1( _L("AP can support Speed Rate 2Mbit") );
	     	    break;
    		case K80211Rate5Mbit:
         		INFO_PRINTF1( _L("AP can support Speed Rate 5Mbit") );
	    	    break;
    		case K80211Rate11Mbit:
				INFO_PRINTF1( _L("AP can support Speed Rate 11Mbit") );
	    	    break;
			case K80211Rate12Mbit:
                INFO_PRINTF1( _L("AP can support Speed Rate 12Mbit") );
            	break;          	
            case K80211Rate18Mbit:
            	INFO_PRINTF1( _L("AP can support Speed Rate 18Mbit") );
            	break;            
            case K80211Rate22Mbit:
            	INFO_PRINTF1( _L("AP can support Speed Rate 22Mbit") );
            	break;            	
            case K80211Rate24Mbit:
            	INFO_PRINTF1( _L("AP can support Speed Rate 24Mbit") );
            	break;            
            case K80211Rate36Mbit:
            	INFO_PRINTF1( _L("AP can support Speed Rate 36Mbit") );
            	break;            
            case K80211Rate48Mbit:
            	INFO_PRINTF1( _L("AP can support Speed Rate 48Mbit") );
            	break;
    		case K80211Rate54Mbit:
				INFO_PRINTF1( _L("AP can support Speed Rate 54Mbit") );
	    	    break;

    		default:
	    	    break;
    		}
	    }

	return supported;
	}

/**
 * Shutdown the socket (RSocket::Shutdown).
 * @param aAsyncErrorIndex		Command index for async calls to returns errors to
 * @return 
 */
void CT_RSocketData::DoCmdShutdown( const TInt aAsyncErrorIndex)
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdShutdown"));
	INFO_PRINTF1(_L("Starting to shutdown Socket"));
	iSocket->Shutdown( RSocket::ENormal, iActCallShutDownSocket->iStatus);				
	iActCallShutDownSocket->Activate(aAsyncErrorIndex);
	IncOutstanding();
	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdShutdown"));
	}
/**
 * Helper function calling from the destroyer.
 * @param 			
 * @return
 */
void CT_RSocketData::Shutdown()
	{
	TInt err(KErrNone);
	TRequestStatus status;
	iSocket->Shutdown(RSocket::ENormal, status);
	User::WaitForRequest( status );			
	err = status.Int();
	if( err != KErrNone )
		{
		ERR_PRINTF2( _L("CT_RSocketData::Shutdown(): error[%d]"), err);
		SetError(err);
		}
	}

/**
 * Close de socket.
 * @param
 * @return
 */
void CT_RSocketData::DoCmdClose()
	{
	INFO_PRINTF1(_L("*START* CT_RSocketData::DoCmdClose"));
	Close();
	INFO_PRINTF1(_L("*END* CT_RSocketData::DoCmdClose"));
	}
/**
 * Helper function to close the socket.
 * @param
 * @return
 */
void CT_RSocketData::Close()
	{
	iSocket->Close();		
    iSocketOpened = EFalse;	
	}
