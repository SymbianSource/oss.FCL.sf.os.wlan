/*
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:   Implementation of the RFrameXferBlock class.
*
*/

/*
* %version: 16 %
*/

#include "WlLddWlanLddConfig.h"
#include "FrameXferBlock.h"
#include "wlanlddcommon.h"
#include "algorithm.h"
#include <kernel.h> // for Kern::SystemTime()


// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void RFrameXferBlockBase::KeInitialize()
    {
    iRxDataChunk = NULL;

    for ( TUint32 i = 0; i < KMaxCompletedRxBufs; ++i )
        {
        iRxCompletedBuffers[i] = 0;
        }

    for ( TUint j = 0; j < TDataBuffer::KFrameTypeMax; ++j )
        {
        iTxOffset[j] = 0;
        }

    iNumOfCompleted = 0;
    iCurrentRxBuffer = 0;
    iFirstRxBufferToFree = 0;    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void RFrameXferBlockBase::KeRxComplete( 
    const TUint32* aRxCompletionBuffersArray, 
    TUint32 aNumOfCompleted )
    {
    if ( aNumOfCompleted > KMaxCompletedRxBufs )
        {
        os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
        }

    assign( aRxCompletionBuffersArray, iRxCompletedBuffers, aNumOfCompleted );
    iNumOfCompleted = aNumOfCompleted;
    iCurrentRxBuffer = 0;
    iFirstRxBufferToFree = 0;

    for ( TUint i = 0; i < iNumOfCompleted; ++i )
        {
        TraceDump( RX_FRAME, 
            (("WLANLDD: RFrameXferBlockBase::KeRxComplete: completed offset addr: 0x%08x"), 
            iRxCompletedBuffers[i]) );
        }    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void RFrameXferBlockBase::KeGetHandledRxBuffers( 
    const TUint32*& aRxHandledBuffersArray, 
    TUint32& aNumOfHandled )
    { 
    TUint32 numHandled ( iCurrentRxBuffer - iFirstRxBufferToFree );

    // make sure that if an Rx buffer is currently being processed by the user
    // side client, that buffer is not regarded as being already handled
    numHandled = numHandled ? numHandled - 1 : numHandled;
    
    if ( numHandled )
        {
        aRxHandledBuffersArray = &(iRxCompletedBuffers[iFirstRxBufferToFree]);
        aNumOfHandled = numHandled;
        
        iFirstRxBufferToFree += numHandled;
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void RFrameXferBlockBase::KeAllUserSideRxBuffersFreed()
    {
    iFirstRxBufferToFree = 0;
    // need to reset also the current index, so that the difference of these
    // two indexes is zero, and it then correctly indicates, that there are no
    // Rx buffers which could be freed incrementally
    iCurrentRxBuffer = 0;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void RFrameXferBlockBase::KeSetTxOffsets( 
    TUint32 aEthernetFrameTxOffset,
    TUint32 aDot11FrameTxOffset,
    TUint32 aSnapFrameTxOffset )
    {
    iTxOffset[TDataBuffer::KEthernetFrame] = aEthernetFrameTxOffset;
    iTxOffset[TDataBuffer::KDot11Frame] = aDot11FrameTxOffset;
    iTxOffset[TDataBuffer::KSnapFrame] = aSnapFrameTxOffset;
    iTxOffset[TDataBuffer::KEthernetTestFrame] = aEthernetFrameTxOffset;
        
    for ( TUint i = 0; i < TDataBuffer::KFrameTypeMax ; ++i )
        {
        TraceDump( RX_FRAME, 
            (("WLANLDD: RFrameXferBlockBase::KeSetTxOffsets: offset: %d"), 
            iTxOffset[i]) );        
        }
    }

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
//
void RFrameXferBlockProtocolStack::Initialise()
    {
    // perform base class initialization
    KeInitialize();
    
    iThisAddrKernelSpace = reinterpret_cast<TUint32>(this);
    
    iVoiceTxQueue.DoInit();
    iVideoTxQueue.DoInit();
    iBestEffortTxQueue.DoInit();
    iBackgroundTxQueue.DoInit();
    iFreeQueue.DoInit();

    for ( TUint i = 0; i < KTxPoolSizeInPackets; i++ )
        {
        // Set the default values
        
        iDataBuffers[i].FrameType( TDataBuffer::KEthernetFrame );
        iDataBuffers[i].KeClearFlags( TDataBuffer::KTxFrameMustNotBeEncrypted );
        iDataBuffers[i].SetLength( 0 );
        iDataBuffers[i].SetUserPriority( 0 );
        
        iFreeQueue.PutPacket( &iDataBuffers[i] );
        }
    }

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
//
TDataBuffer* RFrameXferBlockProtocolStack::AllocTxBuffer( 
    const TUint8* aTxBuf,
    TUint16 aBufLength )
    {
    TDataBuffer* packet( NULL );

    if ( aTxBuf )
        {
        // Get Packet from Free Queue
        packet = iFreeQueue.GetPacket();
        
        TraceDump( NWSA_TX_DETAILS, 
            (("WLANLDD: RFrameXferBlockProtocolStack::AllocTxBuffer: krn metahdr addr: 0x%08x"), 
            packet) );
        
        if ( packet )
            {
            packet->KeSetBufferOffset( 
                aTxBuf - reinterpret_cast<TUint8*>(packet) );
            
            packet->SetBufLength( aBufLength );
            
            // reserve appropriate amount of empty space before the Ethernet
            // frame so that there is space for all the necessary headers, including
            // the 802.11 MAC header 
            packet->KeSetOffsetToFrameBeginning( 
                iTxOffset[TDataBuffer::KEthernetFrame] );
            
            // return the user space address
            packet = reinterpret_cast<TDataBuffer*>(
                reinterpret_cast<TUint8*>(packet) - iUserToKernAddrOffset);
            }
        }
    
    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: RFrameXferBlockProtocolStack::AllocTxBuffer: user metahdr addr: 0x%08x"), 
        packet) );
    
    return packet;
    }

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
//
TBool RFrameXferBlockProtocolStack::AddTxFrame( 
    TDataBuffer* aPacketInUserSpace, 
    TDataBuffer*& aPacketInKernSpace,
    TBool aUserDataTxEnabled )
    {
    TBool ret( ETrue );
    aPacketInKernSpace = NULL;
    TDataBuffer* metaHdrInKernSpace ( reinterpret_cast<TDataBuffer*>(
        reinterpret_cast<TUint8*>(aPacketInUserSpace) + iUserToKernAddrOffset) );
    
    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: RFrameXferBlockProtocolStack::AddTxFrame: user metahdr addr: 0x%08x"), 
        aPacketInUserSpace));

    // put the packet to the correct Tx queue according to the priority to AC
    // mapping defined in WiFi WMM Specification v1.1
    
    if ( aPacketInUserSpace->UserPriority() == 7 || 
         aPacketInUserSpace->UserPriority() == 6 )
        {
        TraceDump( NWSA_TX_DETAILS, 
            (("WLANLDD: add to VO queue; krn metahdr addr: 0x%08x"), 
            reinterpret_cast<TUint32>(metaHdrInKernSpace)));
        
        ret = iVoiceTxQueue.PutPacket( metaHdrInKernSpace );
        
        if ( !ret )
            {
            aPacketInKernSpace = metaHdrInKernSpace;
            }
        
        ret = TxFlowControl( EVoice, aUserDataTxEnabled );
        }
    else if ( aPacketInUserSpace->UserPriority() == 5 || 
              aPacketInUserSpace->UserPriority() == 4 )
        {
        TraceDump( NWSA_TX_DETAILS, 
            (("WLANLDD: add to VI queue; krn metahdr addr: 0x%08x"), 
            reinterpret_cast<TUint32>(metaHdrInKernSpace)));
        
        ret = iVideoTxQueue.PutPacket( metaHdrInKernSpace );
        
        if ( !ret )
            {
            aPacketInKernSpace = metaHdrInKernSpace;
            }
        
        ret = TxFlowControl( EVideo, aUserDataTxEnabled );
        }
    else if ( aPacketInUserSpace->UserPriority() == 2 || 
              aPacketInUserSpace->UserPriority() == 1 )
        {
        TraceDump( NWSA_TX_DETAILS, 
            (("WLANLDD: add to BG queue; krn metahdr addr: 0x%08x"), 
            reinterpret_cast<TUint32>(metaHdrInKernSpace)));
        
        ret = iBackgroundTxQueue.PutPacket( metaHdrInKernSpace );
        
        if ( !ret )
            {
            aPacketInKernSpace = metaHdrInKernSpace;
            }
        
        ret = TxFlowControl( EBackGround, aUserDataTxEnabled );
        }
    else 
        {
        // user priority is 3 or 0 or invalid
        TraceDump( NWSA_TX_DETAILS, 
            (("WLANLDD: add to BE queue; krn metahdr addr: 0x%08x"), 
            reinterpret_cast<TUint32>(metaHdrInKernSpace)));
        
        ret = iBestEffortTxQueue.PutPacket( metaHdrInKernSpace );
        
        if ( !ret )
            {
            aPacketInKernSpace = metaHdrInKernSpace;
            }
        
        ret = TxFlowControl( ELegacy, aUserDataTxEnabled );
        }

    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: VO: %d packets"), 
        iVoiceTxQueue.GetLength()));
    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: VI: %d packets"), 
        iVideoTxQueue.GetLength()));
    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: BE: %d packets"), 
        iBestEffortTxQueue.GetLength()));
    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: BG: %d packets"), 
        iBackgroundTxQueue.GetLength()));
    
    return ret;
    }

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
//
TDataBuffer* RFrameXferBlockProtocolStack::GetTxFrame(
    const TWhaTxQueueState& aWhaTxQueueState,
    TBool& aMore )
    {
    TraceDump( NWSA_TX_DETAILS, 
        ("WLANLDD: RFrameXferBlockProtocolStack::GetTxFrame"));
    TraceDump( NWSA_TX_DETAILS, (("WLANLDD: VO: %d packets"), 
        iVoiceTxQueue.GetLength()));
    TraceDump( NWSA_TX_DETAILS, (("WLANLDD: VI: %d packets"), 
        iVideoTxQueue.GetLength()));
    TraceDump( NWSA_TX_DETAILS, (("WLANLDD: BE: %d packets"), 
        iBestEffortTxQueue.GetLength()));
    TraceDump( NWSA_TX_DETAILS, (("WLANLDD: BG: %d packets"), 
        iBackgroundTxQueue.GetLength()));
    
    TDataBuffer* packet = NULL;
    TQueueId queueId ( EQueueIdMax );
    
    if ( TxPossible( aWhaTxQueueState, queueId ) )
        {
        switch ( queueId )
            {
            case EVoice:
                packet = iVoiceTxQueue.GetPacket();
                break;
            case EVideo:
                packet = iVideoTxQueue.GetPacket();
                break;
            case ELegacy:
                packet = iBestEffortTxQueue.GetPacket();
                break;
            case EBackGround:
                packet = iBackgroundTxQueue.GetPacket();
                break;
#ifndef NDEBUG
            default:
                TraceDump(ERROR_LEVEL, (("WLANLDD: queueId: %d"), queueId));
                os_assert( 
                    (TUint8*)("WLANLDD: panic"), 
                    (TUint8*)(WLAN_FILE), 
                    __LINE__ );
#endif                
            }
        
        aMore = TxPossible( aWhaTxQueueState, queueId );
        }
    else
        {
        aMore = EFalse;        
        }

    TraceDump( NWSA_TX_DETAILS, (("WLANLDD: krn meta hdr: 0x%08x"), 
        reinterpret_cast<TUint32>(packet)));
    
    return packet;
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
TBool RFrameXferBlockProtocolStack::ResumeClientTx( 
    TBool aUserDataTxEnabled ) const
    {
    TBool ret ( EFalse );

    const TInt64 KTimeNow ( Kern::SystemTime() );

    if ( aUserDataTxEnabled )
        {
        // Note that in what follows below we want to consider resuming the
        // client Tx flow only based on the highest priority queue which is
        // currently active. In other words: if we find that some queue is
        // currently active but we don't want to resume the Tx flow yet
        // based on its situation, we must not consider resuming the Tx flow
        // based on any other queue with lower priority.
        
        if ( iVoiceTxQueue.IsActive( KTimeNow ) )
            {
            if ( iVoiceTxQueue.GetLength() < 
                 ( KVoiceTxQueueLen / 2 ) )
                {
                ret = ETrue;
                }
            }
        else if ( iVideoTxQueue.IsActive( KTimeNow ) )
            {
            if ( iVideoTxQueue.GetLength() < 
                 ( KVideoTxQueueLen / 2 ) )
                {
                ret = ETrue;
                }
            }
        else if ( iBestEffortTxQueue.IsActive( KTimeNow ) )
            {
            if ( iBestEffortTxQueue.GetLength() < 
                 ( KBestEffortTxQueueLen / 2 ) )
                {
                ret = ETrue;
                }
            }
        else if ( iBackgroundTxQueue.IsActive( KTimeNow ) )
            {
            if ( iBackgroundTxQueue.GetLength() < 
                 ( KBackgroundTxQueueLen / 2 ) )
                {
                ret = ETrue;
                }
            }
        else
            {
            // none of the Tx queues is currently active (meaning also that
            // they are all empty), but as this method was called, the
            // client Tx flow has to be currently stopped. So now - at the
            // latest - we need to resume the client Tx flow
            ret = ETrue;
            }
        }
    else
        {
        // as client Tx flow has been stopped and user data Tx is disabled
        // (which probably means that we are roaming), its not feasible to
        // resume the client Tx flow yet. So, no action needed; 
        // EFalse is returned
        }
    
#ifndef NDEBUG
    if ( ret )
        {
        TraceDump( NWSA_TX, 
            ("WLANLDD: RFrameXferBlockProtocolStack::ResumeClientTx: resume flow from protocol stack"));            
        }
#endif        
    
    return ret;
    }

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
//
TBool RFrameXferBlockProtocolStack::TxPossible(
    const TWhaTxQueueState& aWhaTxQueueState,
    TQueueId& aQueueId )
    {
    TBool txPossible ( ETrue );
    
    // In queue priority order, try to locate a Tx packet which is for a 
    // non-full WHA Tx queue
    
    if ( aWhaTxQueueState[EVoice] == ETxQueueNotFull && 
         !iVoiceTxQueue.IsEmpty() )
        {
        aQueueId = EVoice;
        
        TraceDump( NWSA_TX_DETAILS, 
            ("WLANLDD: RFrameXferBlockProtocolStack::TxPossible: from VO queue"));
        }
    else if ( aWhaTxQueueState[EVideo] == ETxQueueNotFull &&
              !iVideoTxQueue.IsEmpty() )
        {
        aQueueId = EVideo;
        
        TraceDump( NWSA_TX_DETAILS, 
            ("WLANLDD: RFrameXferBlockProtocolStack::TxPossible: from VI queue"));
        }
    else if ( aWhaTxQueueState[ELegacy] == ETxQueueNotFull &&
              !iBestEffortTxQueue.IsEmpty() )
        {
        aQueueId = ELegacy;
        
        TraceDump( NWSA_TX_DETAILS, 
            ("WLANLDD: RFrameXferBlockProtocolStack::TxPossible: from BE queue"));
        }
    else if ( aWhaTxQueueState[EBackGround] == ETxQueueNotFull &&
              !iBackgroundTxQueue.IsEmpty() )
        {
        aQueueId = EBackGround;
        
        TraceDump( NWSA_TX_DETAILS, 
            ("WLANLDD: RFrameXferBlockProtocolStack::TxPossible: from BG queue"));
        }
    else
        {
        txPossible = EFalse;
        
        TraceDump( NWSA_TX_DETAILS, 
            ("WLANLDD: RFrameXferBlockProtocolStack::TxPossible: no packet for a non-full wha queue"));
        }
    
    return txPossible;
    }

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
//
TBool RFrameXferBlockProtocolStack::TxFlowControl( 
    TQueueId aTxQueue, 
    TBool aUserDataTxEnabled )
    {
    TInt status ( ETrue );
    
    switch ( aTxQueue )
        {
        case EVoice:
            if ( iVoiceTxQueue.IsFull() )
                {
                status = EFalse;
                }
            break;
        case EVideo:
            if ( iVideoTxQueue.IsFull() )
                {
                if ( !aUserDataTxEnabled )
                    {
                    status = EFalse;
                    }
                else if ( !iVoiceTxQueue.IsActive( Kern::SystemTime() ) )
                    {
                    status = EFalse;
                    }
                }
            break;
        case ELegacy:
            {
            const TInt64 KTimeNow ( Kern::SystemTime() );
            
            if ( iBestEffortTxQueue.IsFull() )
                {
                if ( !aUserDataTxEnabled )
                    {
                    status = EFalse;
                    }
                else if ( !iVoiceTxQueue.IsActive( KTimeNow ) && 
                          !iVideoTxQueue.IsActive( KTimeNow ) )
                    {
                    status = EFalse;
                    }
                }
            break;
            }
        case EBackGround:
            {
            const TInt64 KTimeNow ( Kern::SystemTime() );
            
            if ( iBackgroundTxQueue.IsFull() )
                {
                if ( !aUserDataTxEnabled )
                    {
                    status = EFalse;
                    }
                else if ( !iVoiceTxQueue.IsActive( KTimeNow ) && 
                          !iVideoTxQueue.IsActive( KTimeNow ) && 
                          !iBestEffortTxQueue.IsActive( KTimeNow ) )
                    {
                    status = EFalse;
                    }
                }
            break;
            }
#ifndef NDEBUG
        default:
            TraceDump(ERROR_LEVEL, (("WLANLDD: aTxQueue: %d"), aTxQueue));
            os_assert( 
                (TUint8*)("WLANLDD: panic"), 
                (TUint8*)(WLAN_FILE), 
                __LINE__ );
#endif
        }
    
    return status;
    }
