/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Class implementing WLAN BgScan AWS communications
*
*/

/*
* %version: 1 %
*/

#ifndef WLANBGSCANAWSCOMMS_H
#define WLANBGSCANAWSCOMMS_H

#include "awsinterface.h"
#include "awsenginebase.h"
#include "wlantimerservices.h"
#include "wlanbgscancommandlistener.h"
#include "wlanbgscancommand.h"

// forward declarations
class CWlanBgScan;

/**
 *  WLAN Background Scan AWS communications
 *  This class implements WLAN Background Scan AWS communications.
 *  
 *  @since S60 S60 v5.2
 */
NONSHARABLE_CLASS( CWlanBgScanAwsComms ) :
    public CActive,
    public MAwsBgScanProvider,
    public MWlanBgScanCommandListener
    {

public:
    
    /**
     * AWS commands.
     */
    enum TAwsCommand
        {
        EStart = 0,
        EStop,
        ESetPowerSaveMode,
        EAwsCommandMax //not a real command
        };
    
    /**
     * AWS message.
     */
    struct TAwsMessage
        {
        TAwsCommand iCmd;
        TInt iParameter;
        };

    /**
     * Two-phased constructor.
     */
    static CWlanBgScanAwsComms* NewL( CWlanBgScan& aBgScan );
    
    /**
    * Destructor.
    */
    virtual ~CWlanBgScanAwsComms();
       
    /**
     * From MWlanBgScanCommandListener.
     * DoSetInterval.
     * This method is called by the command queue when
     * interval change request is processed.
     * @param aNewInterval new interval to be taken into use
     */
    void DoSetInterval( TUint32 aNewInterval );
    
    /**
     * From MAwsBgScanProvider.
     * Set new background scan interval.
     *
     * @since S60 v5.2
     * @param aNewInterval new interval to be taken into use
     * @param aStatus Status of the calling active object. On successful
     *                completion contains KErrNone, otherwise one of the
     *                system-wide error codes.
     */
    void SetInterval( TUint32 aNewInterval, TRequestStatus& aStatus );
    
    /**
     * Send or queue a message to be sent to AWS.
     *
     * @param aMessage Message to be sent
     * @since S60 v5.2
     */
    void SendOrQueueAwsCommand( TAwsMessage& aMessage );

    /**
     * Whether AWS is present in system.
     *
     * @since S60 v5.2
     * @return ETrue if present, EFalse if not
     */
    TBool IsAwsPresent();
    
private: // From CActive
    
    /**
     * Handles an active object's request completion event.
     */
    void RunL();
    
    /**
     * Cancels asynchronous request.
     */
    void DoCancel();
    
    /**
     * Handles a leave occurring in RunL().
     * @param aError Error that caused RunL to leave.
     */
    TInt RunError( TInt aError );
    
private:
    
    /**
     * Constructor.
     */
    CWlanBgScanAwsComms( CWlanBgScan& aBgScan );
    
    /**
     * Default Constructor (no implementation).
     */
    CWlanBgScanAwsComms();
    
    /**
     * Two-phased constructor.
     */
    void ConstructL();
    
    /**
     * Clean-up ECOM array
     *
     * @since S60 v5.2
     * @param aArray ECOM objec array
     */
    static void CleanupEComArray(TAny* aArray);
        
    /**
     * Start AWS in it's own thread.
     */
    void StartAwsThreadL();
    
    /**
     * Main function for AWS thread.
     * 
     * @param aThisPtr Pointer to CWlanBgScan instance.
     * @return Symbian specific error code.
     */
    static TInt AwsThreadEntryPoint( TAny* aThisPtr );
    
    /**
     * Instantiate AWS ECOM Plugin.
     */
    void InstantiateAwsPluginL();

    /**
     * Send message to AWS.
     *
     * @param aMessage Message to be sent
     * @since S60 v5.2
     */
    void SendAwsCommand( TAwsMessage& aMessage );
        
private: // data
    
    /**
     * Reference to BgScan component. 
     */
    CWlanBgScan& iBgScan;
    
    /**
     * Reference to Adaptive WLAN Scanning component. 
     */
    CAwsEngineBase* iAws;
    
    /**
     * Reference to AWS ECOM plugin implementation info. 
     */
    CImplementationInformation* iAwsImplInfo;

    /**
     * Command Handler.
     */
    CWlanBgScanCommand* iCommandHandler;
    
    /**
     * Version of AWS.
     */
    TUint iAwsVersion;
    
    /**
     * Command queue for commands to be sent to AWS.
     */
    RArray<TAwsMessage> iAwsMsgQueue;

    /**
     * Currently pending command that has been sent to AWS.
     */
    TAwsCommand iPendingCommand;
    
    /**
     * If AWS instantiation was successful.
     */
    TBool iAwsOk;
        
    };

#endif // WLANBGSCANAWSCOMMS_H
