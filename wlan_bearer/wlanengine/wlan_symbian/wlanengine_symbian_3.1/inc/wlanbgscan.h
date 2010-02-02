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
* Description:  Class implementing WLAN background scan logic
*
*/

/*
* %version: 7 %
*/

#ifndef WLANBGSCAN_H
#define WLANBGSCAN_H

#include "awsinterface.h"
#include "awsenginebase.h"
#include "wlanscanproviderinterface.h"
#include "wlantimerservices.h"
#include "wlanbgscanawscomms.h"

/**
 *  WLAN Background Scan
 *  This class implements WLAN Background Scan logic.
 *  
 *  @since S60 S60 v5.2
 */
NONSHARABLE_CLASS( CWlanBgScan ) :
    public MWlanBgScanProvider,
    public MWlanTimerServiceCallback,
    public MWlanBgScanCommandListener
    {

public:
        
    /**
     * States for WLAN Background Scan.
     */
    enum TWlanBgScanState
        {
        EBgScanOff = 0,
        EBgScanOn,
        EBgScanAuto,
        EBgScanAutoAws
        };
    
    /**
     * States for Auto period.
     */
    enum TWlanBgScanAutoPeriod
        {
        EAutoPeriodNone = 0,
        EAutoPeriodNight,
        EAutoPeriodDay
        };
    
    /**
     * Describes relation of time to time range.
     */
    enum TRelation
        {
        ESmaller,
        EInsideRange,
        EGreater
        };

    /**
     * Two-phased constructor.
     */
    static CWlanBgScan* NewL( MWlanScanResultProvider& aProvider, CWlanTimerServices& aTimerServices );
    
    /**
    * Destructor.
    */
    virtual ~CWlanBgScan();

    /**
     * From MWlanBgScanProvider.
     * Called when Scan has been completed.
     * 
     * \msc
     * ScanResultProvider,CWlanBgScan;
     * --- [label="Scan is done"];
     * ScanResultProvider=>CWlanBgScan [label="ScanComplete()"];
     * ScanResultProvider<=CWlanBgScan [label="Scan()"];
     * --- [label="New scan request is placed to queue or"];
     * --- [label="existing request's scan time is updated"];
     * ScanResultProvider>>CWlanBgScan [label="return"];
     * ScanResultProvider<<CWlanBgScan [label="return"];
     * \endmsc
     * 
     *
     * @since S60 v5.2
     */
    void ScanComplete();
        
    /**
     * From MWlanBgScanProvider.
     * Issued when WLAN is disconnected.
     *
     * \msc
     * ScanResultProvider,CWlanBgScan;
     * --- [label="WLAN is disconnected"];
     * ScanResultProvider=>CWlanBgScan [label="NotConnected()"];
     * ScanResultProvider<=CWlanBgScan [label="Scan( aMaxDelay=0 )"];
     * ScanResultProvider>>CWlanBgScan [label="return"];
     * ScanResultProvider<<CWlanBgScan [label="return"];
     * \endmsc
     *
     * @since S60 v5.2
     */
    void NotConnected();
    
    /**
     * From MWlanBgScanProvider.
     * Whether background scan is enabled.
     *
     * @since S60 v5.2
     * @return True if background scan is enabled,
     *         False otherwise.
     */
    TBool IsBgScanEnabled();
        
    /**
     * From MWlanBgScanProvider.
     * Notification about changed settings.
     *
     * @since S60 v5.2
     * 
     * @param aSettings new settings to be taken into use
     */
    void NotifyChangedSettings( MWlanBgScanProvider::TWlanBgScanSettings& aSettings );
    
    /**
     * From MAwsBgScanProvider.
     * Set new background scan interval.
     * Asynchronous method to set new background scan interval, executed in
     * AWS thread.
     *
     * @since S60 v5.2
     * @param aNewInterval new interval to be taken into use
     * @param aStatus Status of the calling active object. On successful
     *                completion contains KErrNone, otherwise one of the
     *                system-wide error codes.
     */
    void SetInterval( TUint32 aNewInterval, TRequestStatus& aReportStatus );
    
    /**
     * From MWlanTimerServiceCallback.
     * OnTimeout.
     * Requested time has elapsed.
     *
     * @since S60 v5.2
     */
    void OnTimeout();
       
    /**
     * From MWlanBgScanCommandListener.
     * DoSetInterval.
     * This method is called by the command queue when
     * interval change request is processed.
     * @param aNewInterval new interval to be taken into use
     */
    void DoSetInterval( TUint32 aNewInterval );
    
private:
    
    /**
     * Constructor.
     */
    CWlanBgScan( MWlanScanResultProvider& aProvider, CWlanTimerServices& aTimerServices  );
    
    /**
     * Two-phased constructor.
     */
    void ConstructL();
        
    /**
     * Main state machine
     *
     * @since S60 v5.2
     * @param aNewBgScanSetting new background scan setting to be taken into use
     */
    void NextState( TUint32 aNewBgScanSetting );
    
    /**
     * State machine for Off state
     *
     * @since S60 v5.2
     * @param aState reference to state
     * @param aNewInterval new background scan setting to be taken into use
     */
    void InStateOff( TWlanBgScanState& aState, TUint32 aNewBgScanSetting );
    
    /**
     * State machine for On state
     *
     * @since S60 v5.2
     * @param aState reference to state
     * @param aNewBgScanSetting new background scan setting to be taken into use
     */
    void InStateOn( TWlanBgScanState& aState, TUint32 aNewBgScanSetting );

    /**
     * State machine for Auto state
     *
     * @since S60 v5.2
     * @param aState reference to state
     * @param aNewBgScanSetting new background scan setting to be taken into use
     */
    void InStateAuto( TWlanBgScanState& aState, TUint32 aNewBgScanSetting );
    
    /**
     * State machine for Auto with AWS state
     *
     * @since S60 v5.2
     * @param aState reference to state
     * @param aNewBgScanSetting new background scan setting to be taken into use
     */
    void InStateAutoAws( TWlanBgScanState& aState, TUint32 aNewBgScanSetting );
    
    /**
     * Request callback when interval change should take place.
     *
     * @since S60 v5.2
     */
    void ScheduleAutoIntervalChange();
    
    /**
     * Get current Auto Background Scan interval.
     *
     * @since S60 v5.2
     * 
     * @return interval
     */
    TUint32 CurrentAutoInterval();
    
    /**
     * Get current interval.
     *
     * @since S60 v5.2
     * @return current interval in use
     */
    TUint32 GetInterval();
    
    /**
     * Set current interval.
     *
     * @param aInterval interval to take into use
     * @since S60 v5.2
     */
    void SetInterval( TUint32 aInterval );
    
    /**
     * Get next time when to change Auto interval.
     *
     * @since S60 v5.2
     * 
     * @return time to change interval
     */
    TTime AutoIntervalChangeAt();
    
    /**
     * See how time is related to a time range.
     *
     * @param aTime time to be checked
     * @param aStart start of range
     * @param aEnd end of range
     * @return relation
     * @since S60 v5.2
     */
    TRelation TimeRelationToRange( const TTime& aTime, TUint aRangeStart, TUint aRangeEnd ) const;
    
    /**
     * Deliver new background interval.
     *
     * @since S60 v5.2
     * 
     * @param aNewInterval new interval to be taken into use
     */
    void IntervalChanged( TUint32 aNewInterval );
    
    /**
     * Is AWS present in system.
     *
     * @since S60 v5.2
     * @return ETrue if background scan is enabled,
     *         EFalse otherwise.
     */
    TBool IsAwsPresent();
    
    /**
     * Check the proposed settings are valid.
     *
     * @since S60 v5.2
     *
     * @param aSettingsToUse settings that can be used [OUT]
     * @param aProposedSettings proposed settings [IN]
     */
    void CheckSettings(
            MWlanBgScanProvider::TWlanBgScanSettings& aSettingsToUse,
            const MWlanBgScanProvider::TWlanBgScanSettings& aProposedSettings ) const;
    
    /**
     * Is interval change needed.
     *
     * @since S60 v5.2
     * @return ETrue if interval change is needed,
     *         EFalse otherwise.
     */
    TBool IsIntervalChangeNeeded();
    
private: // data
    
    /**
     * Reference to the Scan Result Provider
     */
    MWlanScanResultProvider& iProvider;
    
    /**
     * Actually used interval for backgroundscan. 
     */
    TUint32 iCurrentBgScanInterval;
    
    /**
     * Reference to BgScan <-> AWS communications object. 
     */
    CWlanBgScanAwsComms* iAwsComms;
    
    /**
     * Current background scan state. 
     */
    TWlanBgScanState iBgScanState;
    
    /**
     * Current Auto period. 
     */
    TWlanBgScanAutoPeriod iAutoPeriod;
    
    /**
     * Reference to WLAN Timer services. 
     */
    CWlanTimerServices& iTimerServices;

    /**
     * Id of the timer service request regarding 
     * background scan interval change.
     */
    TUint iIntervalChangeRequestId;

    /**
     * Background scan peak start time.
     */
    TUint iBgScanPeakStartTime;
    
    /**
     * Background scan peak end time.
     */
    TUint iBgScanPeakEndTime;
    
    /**
     * Peak time interval.
     */
    TUint iBgScanIntervalPeak;
    
    /**
     * Off-peak time interval.
     */
    TUint iBgScanIntervalOffPeak;
        
    };

#endif // WLANBGSCAN_H
