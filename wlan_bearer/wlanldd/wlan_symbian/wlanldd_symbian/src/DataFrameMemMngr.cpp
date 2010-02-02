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
* Description:   Implementation of the DataFrameMemMngr class.
*
*/

/*
* %version: 17 %
*/

#include "WlLddWlanLddConfig.h"
#include "DataFrameMemMngr.h"
#include "osachunk.h"
#include <kernel.h> 
#include <kern_priv.h>

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TInt DataFrameMemMngr::DoOpenHandle(
    DThread& aThread,
    TSharedChunkInfo& aSharedChunkInfo,
    DChunk* aSharedMemoryChunk )
    {
    TInt ret ( KErrGeneral );

    if ( aSharedMemoryChunk )
        {
        
        // Need to be in critical section while creating handles
        NKern::ThreadEnterCS();

        // Make handle to shared memory chunk for client thread
        TInt r = Kern::MakeHandleAndOpen( &aThread, aSharedMemoryChunk );

        // Leave critical section 
        NKern::ThreadLeaveCS();

        // r: positive value is a handle, negative value is an error code
        if( r >= 0 )
            {
            // mapping success

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: Handle create & open ok: handle: %d"),
                r ) );
    
            // store the handle & chunk size
            aSharedChunkInfo.iChunkHandle = r;
            aSharedChunkInfo.iSize = aSharedMemoryChunk->Size();

            // store the kernel addresses

            TLinAddr start_of_mem_linear( 0 );
            aSharedMemoryChunk->Address( 0, aSharedChunkInfo.iSize, start_of_mem_linear );

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: chunk kernel mode start addr: 0x%08x"),
                start_of_mem_linear ) );

            TUint8* start_of_mem = reinterpret_cast<TUint8*>(start_of_mem_linear );

            const TUint KRxDataChunkSize( 
                aSharedChunkInfo.iSize 
                - ( sizeof( TDataBuffer )
                    + KMgmtSideTxBufferLength
                    + KProtocolStackSideTxDataChunkSize
                    + sizeof( RFrameXferBlock )
                    + sizeof( RFrameXferBlockProtocolStack ) ) );

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: KRxDataChunkSize: %d"),
                KRxDataChunkSize ) );

            iRxDataChunk = start_of_mem;

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: iRxDataChunk start addr: 0x%08x"),
                reinterpret_cast<TUint32>(iRxDataChunk) ) );
            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: iRxDataChunk end addr: 0x%08x"),
                reinterpret_cast<TUint32>(iRxDataChunk + KRxDataChunkSize) ) );

            iTxDataChunk = 
                start_of_mem  
                + KRxDataChunkSize 
                + sizeof( TDataBuffer ) 
                + KMgmtSideTxBufferLength;

            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: iTxDataChunk start addr: 0x%08x"),
                reinterpret_cast<TUint32>(iTxDataChunk) ) );
            TraceDump( INIT_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::DoOpenHandle: iTxDataChunk end addr: 0x%08x"),
                reinterpret_cast<TUint32>(
                    iTxDataChunk + KProtocolStackSideTxDataChunkSize) ) );

            // create the Tx frame memory pool manager to manage the Tx Data
            // chunk
            iTxFrameMemoryPool = new WlanChunk( 
                iTxDataChunk, 
                iTxDataChunk + KProtocolStackSideTxDataChunkSize,
                iTxFrameBufAllocationUnit );
            
            if ( iTxFrameMemoryPool && iTxFrameMemoryPool->IsValid() )
                {
                TraceDump(MEMORY, (("WLANLDD: new WlanChunk: 0x%08x"), 
                    reinterpret_cast<TUint32>(iTxFrameMemoryPool)));

                iFrameXferBlock = reinterpret_cast<RFrameXferBlock*>(
                    start_of_mem  
                    + KRxDataChunkSize 
                    + sizeof( TDataBuffer ) 
                    + KMgmtSideTxBufferLength
                    + KProtocolStackSideTxDataChunkSize
                    + sizeof( RFrameXferBlock ) );
                
                iFrameXferBlockProtoStack = 
                    static_cast<RFrameXferBlockProtocolStack*>(iFrameXferBlock);
                
                TraceDump( INIT_LEVEL, 
                    (("WLANLDD: DataFrameMemMngr::DoOpenHandle: Nif RFrameXferBlock addr: 0x%08x"),
                    reinterpret_cast<TUint32>(iFrameXferBlockProtoStack) ) );
    
                // initiliase xfer block
                iFrameXferBlockProtoStack->Initialise();
                
                iRxBufAlignmentPadding = iParent.RxBufAlignmentPadding();
                
                ret = KErrNone;
                }
            else
                {
                // create failed
                delete iTxFrameMemoryPool;
                iTxFrameMemoryPool = NULL;
                // error is returned
                }
            }
        else
            {
            // handle creation & open failed. Error is returned

            TraceDump( INIT_LEVEL | ERROR_LEVEL, 
                (("WLANLDD: DataFrameMemMngr::OnInitialiseMemory: Handle create & open error: %d"),
                r ) );            
            }
        }
    else
        {
        // at this point the shared memory chunk should always exist. However,
        // as it doesn't exist in this case, we return an error

        TraceDump( INIT_LEVEL | ERROR_LEVEL, 
            ("WLANLDD: DataFrameMemMngr::OnInitialiseMemory: Error aSharedMemoryChunk is NULL") );
        }    
    
    return ret;    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DataFrameMemMngr::DoFreeRxBuffers()
    {
    for ( TUint i = 0; i < iCountCompleted; ++i )
        {
        TDataBuffer* metaHdr ( reinterpret_cast<TDataBuffer*>(
            iRxDataChunk + iCompletedBuffers[i]) );  
        
        // first free the actual Rx frame buffer if relevant
        if ( metaHdr->KeFlags() & TDataBuffer::KDontReleaseBuffer )
            {
            // this buffer shall not be freed yet, so no action here
            
            TraceDump( RX_FRAME, 
                (("WLANLDD: DataFrameMemMngr::DoFreeRxBuffers: don't free yet Rx buf at addr: 0x%08x"),
                reinterpret_cast<TUint32>(metaHdr->KeGetBufferStart()) ) );            
            }
        else
            {
            TraceDump( RX_FRAME, 
                (("WLANLDD: DataFrameMemMngr::DoFreeRxBuffers: free Rx buf at addr: 0x%08x"),
                reinterpret_cast<TUint32>(metaHdr->KeGetBufferStart()) ) );
    
            iRxFrameMemoryPool->Free( 
                metaHdr->KeGetBufferStart()
                // take into account the alignment padding 
                - iRxBufAlignmentPadding );
            }
        
        // free the Rx frame meta header

        TraceDump( RX_FRAME, 
            (("WLANLDD: DataFrameMemMngr::DoFreeRxBuffers: free Rx meta header at addr: 0x%08x"),
            reinterpret_cast<TUint32>(metaHdr)) );

        iRxFrameMemoryPool->Free( metaHdr );        
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool DataFrameMemMngr::DoEthernetFrameRxComplete( 
    const TDataBuffer*& aBufferStart, 
    TUint32 aNumOfBuffers )
    {
    TraceDump( RX_FRAME, 
        (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: aNumOfBuffers: %d"), 
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
                    (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: supplied Rx buf addr: 0x%08x"), 
                    iCompletedBuffers[i]) );        
                
                iCompletedBuffers[i] 
                    -= reinterpret_cast<TUint32>(iRxDataChunk);

                TraceDump( RX_FRAME, 
                    (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: Rx buf offset addr: 0x%08x"), 
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
                    (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: supplied Rx buf addr: 0x%08x"), 
                    iTobeCompletedBuffers[iCountTobeCompleted + i]) );

                iTobeCompletedBuffers[iCountTobeCompleted + i] 
                    -= reinterpret_cast<TUint32>(iRxDataChunk);

                TraceDump( RX_FRAME, 
                    (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: Rx buf offset addr: 0x%08x"), 
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
                (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: supplied Rx buf addr: 0x%08x"), 
                iTobeCompletedBuffers[iCountTobeCompleted + i]) );        
                
            iTobeCompletedBuffers[iCountTobeCompleted + i] 
                -= reinterpret_cast<TUint32>(iRxDataChunk);

            TraceDump( RX_FRAME, 
                (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: Rx buf offset addr: 0x%08x"), 
                iTobeCompletedBuffers[iCountTobeCompleted + i]) );        
            }
        
        iCountTobeCompleted += aNumOfBuffers;
        }
    
    TraceDump( RX_FRAME, 
        (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: end: iCountCompleted: %d"), 
        iCountCompleted) );

    TraceDump( RX_FRAME, 
        (("WLANLDD: DataFrameMemMngr::DoEthernetFrameRxComplete: end: iCountTobeCompleted: %d"), 
        iCountTobeCompleted) );

    return ret;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TUint32* DataFrameMemMngr::DoGetTobeCompletedBuffersStart()
    {
    return iTobeCompletedBuffers;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TUint32* DataFrameMemMngr::DoGetCompletedBuffersStart()
    {
    return iCompletedBuffers;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TDataBuffer* DataFrameMemMngr::AllocTxBuffer( TUint aLength )
    {
    TraceDump( NWSA_TX_DETAILS, 
        (("WLANLDD: DataFrameMemMngr::AllocTxBuffer: aLength: %d"), 
        aLength) );
    
    TDataBuffer* metaHdr ( NULL );

    if ( aLength > KMaxEthernetFrameLength )
        {
#ifndef NDEBUG
        TraceDump( NWSA_TX_DETAILS, 
            ("WLANLDD: DataFrameMemMngr::AllocTxBuffer: WARNING: max size exceeded; req. denied") );
        os_assert( 
            (TUint8*)("WLANLDD: panic"), 
            (TUint8*)(WLAN_FILE), 
            __LINE__ );                    
#endif        
        
        return metaHdr;
        }
    
    const TUint bufLen ( Align4(
        iVendorTxHdrLen +
        KHtQoSMacHeaderLength +  
        KMaxDot11SecurityEncapsulationLength +
        sizeof( SSnapHeader ) +
        aLength +
        KSpaceForAlignment +
        iVendorTxTrailerLen ) ); 
    
    TUint8* buf = reinterpret_cast<TUint8*>(
        iTxFrameMemoryPool->Alloc( bufLen, EFalse ));
    
    if ( buf )
        {
        TraceDump( NWSA_TX_DETAILS, 
            (("WLANLDD: DataFrameMemMngr::AllocTxBuffer: tx buf kern addr: 0x%08x"), 
            reinterpret_cast<TUint32>(buf) ) );
        
        metaHdr = iFrameXferBlockProtoStack->AllocTxBuffer( 
            buf, 
            static_cast<TUint16>(aLength) );
        
        if ( !metaHdr )
            {
            iTxFrameMemoryPool->Free( buf );
            }
        }
    
    return metaHdr;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DataFrameMemMngr::FreeTxPacket( TDataBuffer*& aPacket )
    {
    if ( IsMemInUse() )
        {
        // free the actual Tx buffer
        iTxFrameMemoryPool->Free( aPacket->KeGetBufferStart() );
        // free the meta header
        iFrameXferBlockProtoStack->FreeTxPacket( aPacket );
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void DataFrameMemMngr::OnReleaseMemory()
    {
    TraceDump(INIT_LEVEL, ("WLANLDD: DataFrameMemMngr::OnReleaseMemory"));

    if ( iTxFrameMemoryPool )
        {
        TraceDump(MEMORY, (("WLANLDD: delete WlanChunk: 0x%08x"), 
            reinterpret_cast<TUint32>(iTxFrameMemoryPool)));        
    
        delete iTxFrameMemoryPool;
        iTxFrameMemoryPool = NULL;
        iTxDataChunk = NULL;
        
        MarkMemFree();            
        }
    }
