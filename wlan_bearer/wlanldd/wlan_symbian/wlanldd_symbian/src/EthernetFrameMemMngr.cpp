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
* Description:   Implementation of the DEthernetFrameMemMngr class.
*
*/

/*
* %version: 23 %
*/

#include "WlLddWlanLddConfig.h"
#include "DataFrameMemMngr.h"
#include "MgmtFrameMemMngr.h"
#include "osachunk.h"
#include <kernel.h> 
#include <kern_priv.h>

#include "EtherCardApi.h"

extern void os_free( const TAny* );

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
DEthernetFrameMemMngr* DEthernetFrameMemMngr::Init( 
    TInt aUnit, 
    DWlanLogicalChannel& aParent,
    WlanChunk*& aRxFrameMemoryPool,
    TBool aUseCachedMemory,
    TInt aFrameBufAllocationUnit )
    {
    DEthernetFrameMemMngr* ret = NULL;

    if ( aUnit == KUnitEthernet )
        {
        ret = new DataFrameMemMngr( 
            aParent, 
            aRxFrameMemoryPool,
            aFrameBufAllocationUnit );
        }
    else if ( aUnit == KUnitWlan )
        {
        ret = new MgmtFrameMemMngr( 
            aParent, 
            aRxFrameMemoryPool,
            aUseCachedMemory,
            aFrameBufAllocationUnit );
        }
    else
        {
        TraceDump(ERROR_LEVEL, 
        (("WLANLDD: DEthernetFrameMemMngr::Init: ERROR: unknown unit: %d"), 
            aUnit));
        // No action. NULL is returned
        }

    if ( ret )
        {
        TraceDump(MEMORY, (("WLANLDD: new FrameMemMngr: 0x%08x"), 
            reinterpret_cast<TUint32>(ret)));
        }

    return ret;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TInt DEthernetFrameMemMngr::OnInitialiseMemory( 
    DThread& aThread,
    TSharedChunkInfo* aSharedChunkInfo,
    TUint aVendorTxHdrLen,
    TUint aVendorTxTrailerLen )
    {    
    TraceDump( INIT_LEVEL, 
        (("WLANLDD: DEthernetFrameMemMngr::OnInitialiseMemory: aVendorTxHdrLen: %d"),
        aVendorTxHdrLen ) );
    TraceDump( INIT_LEVEL, 
        (("WLANLDD: aVendorTxTrailerLen: %d"),
        aVendorTxTrailerLen ) );
    
    TInt ret( KErrGeneral );
    
    iVendorTxHdrLen = aVendorTxHdrLen;
    iVendorTxTrailerLen = aVendorTxTrailerLen;    
    
    // Local info structure we will fill in
    TSharedChunkInfo info;

    if( !IsMemInUse() )
        {
        ret = DoAllocate( iParent.SharedMemoryChunk() );
        
        if ( ret == KErrNone )
            {
            ret = DoOpenHandle( aThread, info, iParent.SharedMemoryChunk() );
            
            if ( ret == KErrNone )
                {
                MarkMemInUSe();     // mark as in use
                ret = KErrNone;
                }
            else
                {
                // handle creation & open failed

                TraceDump( INIT_LEVEL, 
                    (("WLANLDD: DEthernetFrameMemMngr::OnInitialiseMemory: Handle create & open error: status: %d"),
                    ret ) );

                // zero contents of info structure.
                // (Zero is usually a safe value to return on error for most data types,
                // and for object handles this is same as KNullHandle)
                memclr( &info, sizeof( info ) );

                // need to enter critical section for chunk closing
                NKern::ThreadEnterCS();
                
                // schedule the shared memory chunk for destruction
                Kern::ChunkClose( iParent.SharedMemoryChunk() );

                // leave critical section 
                NKern::ThreadLeaveCS();
                }

            // write handle info to client memory
            ret = Kern::ThreadRawWrite( 
                &aThread, 
                aSharedChunkInfo, 
                &info,
                sizeof( info ) );
            if ( ret != KErrNone )
                {
                TraceDump(ERROR_LEVEL, ("WLANLDD: ThreadRawWrite panic"));
                TraceDump(ERROR_LEVEL, (("WLANLDD: ret: %d"), ret));
                os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
                }
            }
        else
            {
            // allocation failed; error code will be returned
            }
        }
    else
        {
        ret = KErrAlreadyExists;
        }

    return ret;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
DEthernetFrameMemMngr::~DEthernetFrameMemMngr()
    {
    OnReleaseMemory();
    
    iFrameXferBlock = NULL;
    iTxDataBuffer = NULL;
    iRxDataChunk = NULL;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DEthernetFrameMemMngr::OnReleaseMemory()
    {
    MarkMemFree();      // mark as free
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TDataBuffer* DEthernetFrameMemMngr::OnWriteEthernetFrame() const
    {
    if ( iTxDataBuffer->GetLength() >= sizeof( SEthernetHeader ) )
        {
        return iTxDataBuffer;
        }
    else
        {
        os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
        return NULL;
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool DEthernetFrameMemMngr::OnReadRequest()
    {
    TBool ret( EFalse );

    if ( IsMemInUse() )
        {
        if ( iCountCompleted )
            {
            // free relevant buffers
            DoFreeRxBuffers();
            iCountCompleted = 0; // no buffers anymore in process in user mode
            
            // make sure that the same buffers are not tried to be
            // freed again thru the incremental freeing method
            iFrameXferBlock->KeAllUserSideRxBuffersFreed();
            }

        if ( iCountTobeCompleted )
            {
            // there are Rx buffers to be completed
            
            iFrameXferBlock->KeRxComplete( DoGetTobeCompletedBuffersStart(), 
                iCountTobeCompleted );  
            // mark the completed buffers
            assign( DoGetTobeCompletedBuffersStart(), 
                DoGetCompletedBuffersStart(), 
                iCountTobeCompleted );
            iCountCompleted = iCountTobeCompleted;
            iCountTobeCompleted = 0;

            ret = ETrue;
             // the frame Rx request won't be pending as the callee shall 
             // complete it
            iReadStatus = ENotPending;
            }
        else
            {
            // there are no Rx buffers to be completed. The Rx request is
            // left pending
            iReadStatus = EPending;
            }
        }
    else
        {
        os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
        }

    return ret;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DEthernetFrameMemMngr::DoMarkRxBufFree( TUint8* /*aBufferToFree*/ )
    {
    // not supported in default handler
    os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DEthernetFrameMemMngr::SetTxOffsets( 
    TUint32 aEthernetFrameTxOffset,
    TUint32 aDot11FrameTxOffset,
    TUint32 aSnapFrameTxOffset )
    {
    if ( IsMemInUse() )
        {
        iFrameXferBlock->KeSetTxOffsets(
            aEthernetFrameTxOffset,
            aDot11FrameTxOffset,
            aSnapFrameTxOffset );
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TUint8* DEthernetFrameMemMngr::OnGetEthernetFrameRxBuffer( 
    TUint aLengthinBytes )
    {
    TUint8* buffer ( NULL );

    if ( IsMemInUse() )
        {
        buffer = DoGetNextFreeRxBuffer( aLengthinBytes );
        }
    else
        {
        // we are trying to acquire an Rx buffer but our user mode client 
        // has not asked for the memorybuffer pool to be initialized. In this
        // case NULL is returned, as no buffers are available
         TraceDump(RX_FRAME, 
            ("WLANLDD: DEthernetFrameMemMngr::OnGetEthernetFrameRxBuffer: not initialized => failed"));       
        }

    return buffer;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TDataBuffer* DEthernetFrameMemMngr::GetRxFrameMetaHeader()
    {
    TDataBuffer* buffer ( NULL );

    if ( IsMemInUse() )
        {
        buffer = reinterpret_cast<TDataBuffer*>(
            iRxFrameMemoryPool->Alloc( sizeof( TDataBuffer ), ETrue ) );

        TraceDump(RX_FRAME, 
           (("WLANLDD: DEthernetFrameMemMngr::GetRxFrameMetaHeader: addr: 0x%08x"),
           reinterpret_cast<TUint32>(buffer)) );        
        }
    else
        {
        // we are trying to acquire memory for Rx frame meta header but our 
        // user mode client has not asked for the memorybuffer pool to be 
        // initialized. In this case NULL is returned, as no memory is 
        // available
         TraceDump(RX_FRAME, 
            ("WLANLDD: DEthernetFrameMemMngr::GetRxFrameMetaHeader: not initialized => failed"));       
        }

    return buffer;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DEthernetFrameMemMngr::FreeRxFrameMetaHeader( TDataBuffer* aMetaHeader )
    {
    if ( IsMemInUse() )
        {
        iRxFrameMemoryPool->Free( aMetaHeader );
        }
    else
        {
        // the whole Rx memory pool - including aMetaHeader - has already
        // been deallocated, so nothing is done in this case
        TraceDump( RX_FRAME, 
            ("WLANLDD: MgmtFrameMemMngr::FreeRxFrameMetaHeader: Rx memory pool already deallocated; no action needed") );                
        }    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool DEthernetFrameMemMngr::OnEthernetFrameRxComplete( 
    const TDataBuffer*& aBufferStart, 
    TUint32 aNumOfBuffers )
    {
    TBool ret( EFalse );

    if ( IsMemInUse() && 
         DoEthernetFrameRxComplete( aBufferStart, aNumOfBuffers ) )
        {
        iReadStatus = ENotPending;
        ret = ETrue;
        }

    return ret;
    }

// ---------------------------------------------------------------------------
// this default implementation always returns KErrNone
// ---------------------------------------------------------------------------
//
TInt DEthernetFrameMemMngr::DoAllocate( DChunk*& /*aSharedMemoryChunk*/ )
    {
    TraceDump( INIT_LEVEL, 
        ("WLANLDD: DEthernetFrameMemMngr::DoAllocate") );

    return KErrNone;
    }

// ---------------------------------------------------------------------------
// this default implementation always returns NULL
// ---------------------------------------------------------------------------
//
TUint8* DEthernetFrameMemMngr::DoGetNextFreeRxBuffer( 
    TUint /*aLengthinBytes*/ )
    {
    TraceDump( RX_FRAME, 
        ("WLANLDD: DEthernetFrameMemMngr::DoGetNextFreeRxBuffer") );

    return NULL;
    }

// ---------------------------------------------------------------------------
// This default implementation always returns NULL
// ---------------------------------------------------------------------------
//
TDataBuffer* DEthernetFrameMemMngr::AllocTxBuffer( TUint aLength )
    {
    return NULL;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool DEthernetFrameMemMngr::AddTxFrame( 
    TDataBuffer* aPacketInUserSpace, 
    TDataBuffer*& aPacketInKernSpace,
    TBool aUserDataTxEnabled )
    {
    return (static_cast<RFrameXferBlockProtocolStack*>(
        iFrameXferBlock))->AddTxFrame( 
            aPacketInUserSpace, 
            aPacketInKernSpace,
            aUserDataTxEnabled );
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TDataBuffer* DEthernetFrameMemMngr::GetTxFrame( 
    const TWhaTxQueueState& aTxQueueState,
    TBool& aMore )
    {
    if ( IsMemInUse() )
        {
        return (static_cast<RFrameXferBlockProtocolStack*>(
            iFrameXferBlock))->GetTxFrame( aTxQueueState, aMore );
        }
    else
        return NULL;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DEthernetFrameMemMngr::FreeTxPacket( TDataBuffer*& /*aPacket*/ )
    {
    // not suported in this default implementation 
    os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool DEthernetFrameMemMngr::ResumeClientTx( TBool aUserDataTxEnabled ) const 
    {
    return (static_cast<RFrameXferBlockProtocolStack*>(
        iFrameXferBlock))->ResumeClientTx( aUserDataTxEnabled );
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool DEthernetFrameMemMngr::AllTxQueuesEmpty() const
    {
    return (static_cast<RFrameXferBlockProtocolStack*>(
            iFrameXferBlock))->AllTxQueuesEmpty();
    }
