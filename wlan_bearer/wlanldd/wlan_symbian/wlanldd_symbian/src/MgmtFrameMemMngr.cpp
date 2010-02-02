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
* Description:   Implementation of the MgmtFrameMemMngr class.
*
*/

/*
* %version: 19 %
*/

#include "WlLddWlanLddConfig.h"
#include "MgmtFrameMemMngr.h"
#include "osachunk.h"
#include <kern_priv.h>

extern TAny* os_alloc( const TUint32 );
extern void os_free( const TAny* );

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TInt MgmtFrameMemMngr::DoAllocate( DChunk*& aSharedMemoryChunk )
    {
    TraceDump( INIT_LEVEL, 
        ("WLANLDD: MgmtFrameMemMngr::DoAllocate:") );

    // determine if cached memory shall be used
    TUint cacheOption = iUseCachedMemory ? 
        EMapAttrCachedMax : 
        EMapAttrFullyBlocking;

    // Start creating the chunk
    
    TChunkCreateInfo info;
    info.iType         = TChunkCreateInfo::ESharedKernelMultiple;
    info.iMaxSize      = iChunkSize;
    info.iMapAttr      = EMapAttrReadUser |
                         EMapAttrWriteUser |
                         EMapAttrShared |
                         cacheOption;
    info.iOwnsMemory   = ETrue;
    info.iDestroyedDfc = NULL;
    
    DChunk* chunk;

    TraceDump( INIT_LEVEL, 
        (("WLANLDD: MgmtFrameMemMngr::DoAllocate: create chunk of size: %d"),
        iChunkSize) );

    TUint32 chunkMapAttrNotNeeded ( 0 );

    // Enter critical section so we can't die and leak the object we are 
    // creating, i.e. DChunk (shared memory chunk)
    NKern::ThreadEnterCS();

    TInt r = Kern::ChunkCreate( 
        info, 
        chunk, 
        iChunkKernelAddr, 
        chunkMapAttrNotNeeded );

    if( r != KErrNone)
        {
        TraceDump( WARNING_LEVEL, 
            (("WLANLDD: MgmtFrameMemMngr::DoAllocate: create chunk failed. Status: %d"),
            r ) );

        NKern::ThreadLeaveCS();
        return r;
        }

    TraceDump(MEMORY, (("WLANLDD: new DChunk: 0x%08x"), 
        reinterpret_cast<TUint32>(chunk)));

    TraceDump(INIT_LEVEL, 
        (("MgmtFrameMemMngr::DoAllocate: Platform Hw Chunk create success with cacheOption: 0x%08x"), 
        cacheOption));

    TUint32 physicalAddressNotNeeded ( 0 );

    // Map our device's memory into the chunk (at offset 0)
    r = Kern::ChunkCommitContiguous( chunk, 0, iChunkSize, physicalAddressNotNeeded );

    if ( r != KErrNone)
        {
        TraceDump( WARNING_LEVEL, 
            (("WLANLDD: MgmtFrameMemMngr::DoAllocate: chunk commit failed. Status: %d"),
            r ) );

        // Commit failed so tidy-up.
        // Close chunk, which will then get deleted at some point
        Kern::ChunkClose( chunk );
        }
    else
        {
        TraceDump( INIT_LEVEL, 
            ("WLANLDD: MgmtFrameMemMngr::DoAllocate: chunk commit success") );

        // Commit succeeded so pass back the chunk pointer
        aSharedMemoryChunk = chunk;
        }

    // Can leave critical section now that we have saved pointer to created object
    NKern::ThreadLeaveCS();

    TraceDump( INIT_LEVEL, 
        (("WLANLDD: MgmtFrameMemMngr::DoAllocate: chunk map attr: %d"),
        chunkMapAttrNotNeeded ) );
    TraceDump( INIT_LEVEL, 
        (("WLANLDD: MgmtFrameMemMngr::DoAllocate: iChunkKernelAddr: 0x%08x"),
        static_cast<TUint32>(iChunkKernelAddr) ) );
    TraceDump( INIT_LEVEL, 
        (("WLANLDD: MgmtFrameMemMngr::DoAllocate: chunk physical address: 0x%08x"),
        physicalAddressNotNeeded ) );

    return r;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TInt MgmtFrameMemMngr::DoOpenHandle(
    DThread& aThread,
    TSharedChunkInfo& aSharedChunkInfo,
    DChunk* aSharedMemoryChunk )
    {
    TInt ret ( KErrGeneral );

    if ( aSharedMemoryChunk )
        {
        
        // Need to be in critical section while creating handles
        NKern::ThreadEnterCS();

        // Create handle to shared memory chunk for client thread
        TInt r = Kern::MakeHandleAndOpen( &aThread, aSharedMemoryChunk );

        // Leave critical section 
        NKern::ThreadLeaveCS();

        // positive r value is a handle, negative value is an error code
        if( r >= 0 )
            {
            // mapping success

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: MgmtFrameMemMngr::OnInitialiseMemory: Handle create & open ok: handle: %d"),
                r ) );
    
            // store the handle & the chunk size
            aSharedChunkInfo.iChunkHandle = r;
            aSharedChunkInfo.iSize = iChunkSize;

            // store the kernel addresses

            TUint8* start_of_mem = reinterpret_cast<TUint8*>(iChunkKernelAddr );

            const TUint KRxDataChunkSize( 
                iChunkSize 
                - ( sizeof( TDataBuffer )
                    + KMgmtSideTxBufferLength
                    + KProtocolStackSideTxDataChunkSize
                    + sizeof( RFrameXferBlock ) 
                    + sizeof( RFrameXferBlockProtocolStack ) ) );

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: MgmtFrameMemMngr::DoOpenHandle: KRxDataChunkSize: %d"),
                KRxDataChunkSize ) );

            iRxDataChunk = start_of_mem;

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: MgmtFrameMemMngr::DoOpenHandle: iRxDataChunk start addr: 0x%08x"),
                reinterpret_cast<TUint32>(iRxDataChunk) ) );
            TraceDump( INIT_LEVEL, 
                (("WLANLDD: MgmtFrameMemMngr::DoOpenHandle: iRxDataChunk end addr: 0x%08x"),
                reinterpret_cast<TUint32>(iRxDataChunk + KRxDataChunkSize) ) );

            // create the Rx frame memory pool manager
            iRxFrameMemoryPool = new WlanChunk( 
                iRxDataChunk, 
                iRxDataChunk + KRxDataChunkSize,
                iRxFrameBufAllocationUnit );
                
            if ( iRxFrameMemoryPool && iRxFrameMemoryPool->IsValid() )
                {
                ret = KErrNone;

                TraceDump(MEMORY, (("WLANLDD: new WlanChunk: 0x%08x"), 
                    reinterpret_cast<TUint32>(iRxFrameMemoryPool)));

                iTxDataBuffer = reinterpret_cast<TDataBuffer*>(
                    start_of_mem
                    + KRxDataChunkSize );

                TraceDump( INIT_LEVEL, 
                    (("WLANLDD: MgmtFrameMemMngr::DoOpenHandle: Engine TxDataBuf start addr: 0x%08x"),
                    reinterpret_cast<TUint32>(iTxDataBuffer) ) );

                // for the single Tx buffer the actual buffer memory immediately
                // follows the Tx frame meta header
                iTxDataBuffer->KeSetBufferOffset( sizeof( TDataBuffer ) );        

                iFrameXferBlock = reinterpret_cast<RFrameXferBlock*>(
                    start_of_mem
                    + KRxDataChunkSize
                    + sizeof( TDataBuffer )
                    + KMgmtSideTxBufferLength 
                    + KProtocolStackSideTxDataChunkSize );

                TraceDump( INIT_LEVEL, 
                    (("WLANLDD: MgmtFrameMemMngr::DoOpenHandle: Engine RFrameXferBlock addr: 0x%08x"),
                    reinterpret_cast<TUint32>(iFrameXferBlock) ) );

                // initiliase xfer block
                iFrameXferBlock->Initialize( KMgmtSideTxBufferLength );
                
                iRxBufAlignmentPadding = RxBufAlignmentPadding();
                
                iParent.SetRxBufAlignmentPadding( iRxBufAlignmentPadding );
                }
            else
                {
                // create failed
                delete iRxFrameMemoryPool;
                iRxFrameMemoryPool = NULL;
                // error is returned
                }
            }
        else
            {
            // handle creation & open failed. Error is returned

            TraceDump( INIT_LEVEL | ERROR_LEVEL, 
                (("WLANLDD: MgmtFrameMemMngr::OnInitialiseMemory: Handle create & open error: %d"),
                r ) );            
            }
        }
    else
        {
        // at this point the shared memory chunk should always exist. However,
        // as it doesn't exist in this case, we return an error

        TraceDump( INIT_LEVEL | ERROR_LEVEL, 
            ("WLANLDD: MgmtFrameMemMngr::OnInitialiseMemory: Error aSharedMemoryChunk is NULL") );
        }    
    
    return ret;    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TUint8* MgmtFrameMemMngr::DoGetNextFreeRxBuffer( TUint aLengthinBytes )
    {
    TraceDump( RX_FRAME, 
        ("WLANLDD: MgmtFrameMemMngr::DoGetNextFreeRxBuffer") );

    // if there are any Rx buffers which have been handled and
    // can already be re-used, free them first
    
    const TUint32* rxHandledBuffersArray ( NULL );
    TUint32 numOfHandled ( 0 );
    
    iFrameXferBlock->KeGetHandledRxBuffers(
        rxHandledBuffersArray, 
        numOfHandled );

    if ( numOfHandled )
        {
        // there are buffers which can be freed, so free them
        for ( TUint i = 0; i < numOfHandled; ++i )
            {
            // first free the actual Rx frame buffer

            TraceDump( RX_FRAME, 
                (("WLANLDD: MgmtFrameMemMngr::DoGetNextFreeRxBuffer: free Rx buf at addr: 0x%08x"),
                reinterpret_cast<TUint32>(reinterpret_cast<TDataBuffer*>(
                    iRxDataChunk 
                    + rxHandledBuffersArray[i])->KeGetBufferStart()) ) );

            iRxFrameMemoryPool->Free( 
                reinterpret_cast<TDataBuffer*>(
                    iRxDataChunk + rxHandledBuffersArray[i])->KeGetBufferStart()
                    // take into account the alignment padding 
                    - iRxBufAlignmentPadding );
            
            // then free the Rx frame meta header

            TraceDump( RX_FRAME, 
                (("WLANLDD: MgmtFrameMemMngr::DoGetNextFreeRxBuffer: free Rx meta header at addr: 0x%08x"),
                reinterpret_cast<TUint32>( iRxDataChunk + rxHandledBuffersArray[i])) );

            iRxFrameMemoryPool->Free( iRxDataChunk + rxHandledBuffersArray[i] );
            }

        // remove the buffers we freed above from the completed buffers of this
        // object so that they are not tried to be freed again once the Mgmt
        // Client issues the next Rx request

        iCountCompleted -= numOfHandled;
        assign( 
            iCompletedBuffers + numOfHandled, 
            iCompletedBuffers, 
            iCountCompleted );
        }

    // reserve a new Rx buffer. 
    
    TUint8* buffer ( NULL );
    
    if ( iRxFrameMemoryPool )
        {
        buffer = reinterpret_cast<TUint8*>(iRxFrameMemoryPool->Alloc( 
            aLengthinBytes
            + iRxBufAlignmentPadding,
            EFalse ) ); // no need to zero stamp the buffer content
        
        if ( buffer )
            {
            // allocation succeeded
            
            // the alignment padding is before the Rx buffer
            buffer += iRxBufAlignmentPadding;
            
            TraceDump(RX_FRAME, 
               (("WLANLDD: MgmtFrameMemMngr::DoGetNextFreeRxBuffer: addr of allocated Rx buf: 0x%08x"),
               reinterpret_cast<TUint32>(buffer)) );
            }
        else
            {
            TraceDump(RX_FRAME | WARNING_LEVEL, 
               ("WLANLDD: MgmtFrameMemMngr::DoGetNextFreeRxBuffer: WARNING: not enough free memory => failed"));            
            }
        }
    else
        {
         TraceDump(RX_FRAME | WARNING_LEVEL, 
            ("WLANLDD: MgmtFrameMemMngr::DoGetNextFreeRxBuffer: WARNING: no Rx mem pool mgr => failed"));       
        }
        
    return buffer;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool MgmtFrameMemMngr::DoEthernetFrameRxComplete( 
    const TDataBuffer*& aBufferStart, 
    TUint32 aNumOfBuffers )
    {
    TraceDump( RX_FRAME, 
        (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: aNumOfBuffers: %d"), 
        aNumOfBuffers) );

    if ( aNumOfBuffers + iCountTobeCompleted > KMaxToBeCompletedRxBufs )
        {
        // too little space reserved for Rx buffer handles
        os_assert( (TUint8*)("WLANLDD: panic"), (TUint8*)(WLAN_FILE), __LINE__ );
        }

    TBool ret( EFalse );

    if ( iReadStatus == EPending )
        {
        // read pending
        if ( !iCountTobeCompleted )
            {
            // no existing Rx buffers to complete in queue
            // we may complete these ones on the fly

            // note the completed Rx buffers first so that we can change
            // their addresses to offsets
            assign( 
                reinterpret_cast<TUint32*>(&aBufferStart), 
                iCompletedBuffers, 
                aNumOfBuffers );

            // update the new Rx buffer start addresses added above to be 
            // offsets from the Rx memory pool beginning
            for( TUint i = 0; i < aNumOfBuffers; ++i )
                {
                TraceDump( RX_FRAME, 
                    (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: supplied Rx buf addr: 0x%08x"), 
                    iCompletedBuffers[i]) );        
                
                iCompletedBuffers[i] 
                    -= reinterpret_cast<TUint32>(iRxDataChunk);

                TraceDump( RX_FRAME, 
                    (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: Rx buf offset addr: 0x%08x"), 
                    iCompletedBuffers[i]) );        
                }

            iCountCompleted = aNumOfBuffers;

            iFrameXferBlock->KeRxComplete( iCompletedBuffers, iCountCompleted);
            }
        else
            {
            // existing rx buffers to complete in queue.
            // We must append these at the rear and after that 
            // complete the existing read request
            assign( 
                reinterpret_cast<TUint32*>(&aBufferStart),
                iTobeCompletedBuffers + iCountTobeCompleted, 
                aNumOfBuffers );

            // update the new Rx buffer start addresses added above to be 
            // offsets from the Rx memory pool beginning
            for( TUint i = 0; i < aNumOfBuffers; ++i )
                {
                TraceDump( RX_FRAME, 
                    (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: supplied Rx buf addr: 0x%08x"), 
                    iTobeCompletedBuffers[iCountTobeCompleted + i]) );

                iTobeCompletedBuffers[iCountTobeCompleted + i] 
                    -= reinterpret_cast<TUint32>(iRxDataChunk);

                TraceDump( RX_FRAME, 
                    (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: Rx buf offset addr: 0x%08x"), 
                    iTobeCompletedBuffers[iCountTobeCompleted + i]) );
                }

            iCountCompleted = iCountTobeCompleted + aNumOfBuffers;

            iFrameXferBlock->KeRxComplete( 
                iTobeCompletedBuffers, 
                iCountCompleted );  

            // note the completed Rx buffers
            assign( iTobeCompletedBuffers, iCompletedBuffers, iCountCompleted );
            iCountTobeCompleted = 0;
            }

        ret = ETrue;
        }
    else
        {
        // no read pending
        // append at the rear
        assign( 
            reinterpret_cast<TUint32*>(&aBufferStart),
            iTobeCompletedBuffers + iCountTobeCompleted, 
            aNumOfBuffers );

        // update the new Rx buffer start addresses added above to be 
        // offsets from the Rx memory pool beginning
        for( TUint i = 0; i < aNumOfBuffers; ++i )
            {
            TraceDump( RX_FRAME, 
                (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: supplied Rx buf addr: 0x%08x"), 
                iTobeCompletedBuffers[iCountTobeCompleted + i]) );        
                
            iTobeCompletedBuffers[iCountTobeCompleted + i] 
                -= reinterpret_cast<TUint32>(iRxDataChunk);

            TraceDump( RX_FRAME, 
                (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: Rx buf offset addr: 0x%08x"), 
                iTobeCompletedBuffers[iCountTobeCompleted + i]) );        
            }
        
        iCountTobeCompleted += aNumOfBuffers;
        }

    TraceDump( RX_FRAME, 
        (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: end: iCountCompleted: %d"), 
        iCountCompleted) );

    TraceDump( RX_FRAME, 
        (("WLANLDD: MgmtFrameMemMngr::DoEthernetFrameRxComplete: end: iCountTobeCompleted: %d"), 
        iCountTobeCompleted) );

    return ret;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TUint32* MgmtFrameMemMngr::DoGetTobeCompletedBuffersStart()
    {
    return iTobeCompletedBuffers;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TUint32* MgmtFrameMemMngr::DoGetCompletedBuffersStart()
    {
    return iCompletedBuffers;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void MgmtFrameMemMngr::DoFreeRxBuffers()
    {
    if ( IsMemInUse() )
        {
        for ( TUint i = 0; i < iCountCompleted; ++i )
            {
            TDataBuffer* metaHdr ( reinterpret_cast<TDataBuffer*>(
                    iRxDataChunk + iCompletedBuffers[i]) );  
            
            // first free the actual Rx frame buffer
            TraceDump( RX_FRAME, 
                (("WLANLDD: MgmtFrameMemMngr::DoFreeRxBuffers: free Rx buf at addr: 0x%08x"),
                reinterpret_cast<TUint32>(metaHdr->KeGetBufferStart()) ) );
    
            iRxFrameMemoryPool->Free( 
                metaHdr->KeGetBufferStart()
                // take into account the alignment padding 
                - iRxBufAlignmentPadding );                            
            
            // free the Rx frame meta header
    
            TraceDump( RX_FRAME, 
                (("WLANLDD: MgmtFrameMemMngr::DoFreeRxBuffers: free Rx meta header at addr: 0x%08x"),
                reinterpret_cast<TUint32>(metaHdr)) );
    
            iRxFrameMemoryPool->Free( metaHdr );        
            }
        }
    else
        {
        // the whole Rx memory pool has already been deallocated, so nothing 
        // is done in this case
        TraceDump( RX_FRAME, 
            ("WLANLDD: MgmtFrameMemMngr::DoFreeRxBuffers: Rx memory pool already deallocated; no action needed") );        
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void MgmtFrameMemMngr::DoMarkRxBufFree( TUint8* aBufferToFree )
    {
    TraceDump( RX_FRAME, 
        (("WLANLDD: MgmtFrameMemMngr::DoMarkRxBufFree: free Rx buf at addr: 0x%08x"),
        reinterpret_cast<TUint32>(aBufferToFree) ) );

    if ( IsMemInUse() )
        {
        iRxFrameMemoryPool->Free( 
            aBufferToFree
            // take into account the alignment padding 
            - iRxBufAlignmentPadding );
        }
    else
        {
        // the whole Rx memory pool - including aBufferToFree - has already
        // been deallocated, so nothing is done in this case
        TraceDump( RX_FRAME, 
            ("WLANLDD: MgmtFrameMemMngr::DoMarkRxBufFree: Rx memory pool already deallocated; no action needed") );                
        }    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TInt MgmtFrameMemMngr::RxBufAlignmentPadding() const
    {
    const TInt KMemMgrHdrLen = iRxFrameMemoryPool->HeaderSize();
    const TInt KRemainder ( KMemMgrHdrLen % iRxFrameBufAllocationUnit );
    TInt padding = KRemainder ? 
        ( iRxFrameBufAllocationUnit - KRemainder ) : KRemainder;
        
    TraceDump(INIT_LEVEL, (("WLANLDD: MgmtFrameMemMngr::RxBufAlignmentPadding: %d"), 
        padding));
    
    return padding;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void MgmtFrameMemMngr::OnReleaseMemory()
    {
    TraceDump(INIT_LEVEL, ("WLANLDD: MgmtFrameMemMngr::OnReleaseMemory"));

    TraceDump(MEMORY, (("WLANLDD: delete WlanChunk: 0x%08x"), 
        reinterpret_cast<TUint32>(iRxFrameMemoryPool)));        

    delete iRxFrameMemoryPool;
    iRxFrameMemoryPool = NULL;

    if ( iParent.SharedMemoryChunk() )
        {
        TraceDump(MEMORY, (("WLANLDD: delete DChunk: 0x%08x"), 
            reinterpret_cast<TUint32>(iParent.SharedMemoryChunk())));        

        // schedule the shared memory chunk for destruction
        Kern::ChunkClose( iParent.SharedMemoryChunk() );
        iParent.SharedMemoryChunk() = NULL;
        MarkMemFree();      // mark as free            
        }    
    else
        {
        // nothing here
        }
    }
