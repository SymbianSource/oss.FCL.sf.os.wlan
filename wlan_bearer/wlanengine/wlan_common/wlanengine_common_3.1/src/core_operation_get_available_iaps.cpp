/*
* Copyright (c) 2005-2010 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  State machine for IAP availability.
*
*/

/*
* %version: 40.1.2 %
*/

#include "core_operation_get_available_iaps.h"
#include "core_server.h"
#include "core_tools.h"
#include "core_tools_parser.h"
#include "core_frame_dot11.h"
#include "core_frame_dot11_ie.h"
#include "am_debug.h"

/** Defining this enables IAP related traces. */
//#define WLAN_CORE_DEEP_DEBUG 1

/** The channel time used in the long passive scan. */
const u32_t LONG_PASSIVE_SCAN_CHANNEL_TIME = 210;

/**
 * If the phone has at least this many IAPs marked as hidden,
 * limiting algorithm will be used for detecting available IAPs.
 */
const u8_t MIN_HIDDEN_IAPS_FOR_LIMITING_ALGORITHM = 15;

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
core_operation_get_available_iaps_c::core_operation_get_available_iaps_c(
    u32_t request_id,
    core_server_c* server,        
    abs_core_driverif_c* drivers,
    abs_core_server_callback_c* adaptation,
    bool_t is_active_scan_allowed,
    core_type_list_c<core_iap_data_s>& iap_data_list,
    core_type_list_c<u32_t>& iap_id_list,
    core_type_list_c<core_ssid_entry_s>* iap_ssid_list,
    ScanList& scan_data ) :
    core_operation_base_c( core_operation_get_available_iaps, request_id, server, drivers, adaptation,
        core_base_flag_drivers_needed ),    
    is_active_scan_allowed_m( is_active_scan_allowed ),
    is_limiting_algorithm_used_m( false_t ),
    iap_data_list_m( iap_data_list ),
    iap_id_list_m( iap_id_list ),
    iap_ssid_list_m( iap_ssid_list ),
    client_scan_data_m( scan_data ),
    active_channels_m( ),
    broadcast_channels_m( ),
    long_broadcast_count_m( 0 ),
    non_found_iaps_list_m( ),
    is_split_scan_m( false_t ),
    bss_count_m( 0 ),
    direct_scanned_ssid_m( ),
    region_from_ap_m( core_wlan_region_fcc )
    {
    DEBUG( "core_operation_get_available_iaps_c::core_operation_get_available_iaps_c()" );
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
core_operation_get_available_iaps_c::~core_operation_get_available_iaps_c()
    {
    DEBUG( "core_operation_get_available_iaps_c::~core_operation_get_available_iaps_c()" );

    server_m->unregister_event_handler( this );
    server_m->unregister_frame_handler( this );
    non_found_iaps_list_m.clear();
    iap_ssid_list_m = NULL;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
core_error_e core_operation_get_available_iaps_c::next_state()
    {
    DEBUG( "core_operation_get_available_iaps_c::next_state()" );

    switch ( operation_state_m )
        {
        case core_state_init:
            {
            operation_state_m = core_state_long_broadcast_scan_start;

#ifdef WLAN_CORE_DEEP_DEBUG
            DEBUG( "core_operation_get_available_iaps_c::next_state() - IAP data list:" );

            core_iap_data_s* iter_data = iap_data_list_m.first();            
            while( iter_data )
                {
                DEBUG1( "core_operation_get_available_iaps_c::next_state() - ID: %u",
                    iter_data->id );
                DEBUG1S( "core_operation_get_available_iaps_c::next_state() - SSID: ",
                    iter_data->ssid.length, iter_data->ssid.ssid );

                iter_data = iap_data_list_m.next();
                }

            if ( iap_ssid_list_m )
                {
                DEBUG( "core_operation_get_available_iaps_c::next_state()" );
                DEBUG( "core_operation_get_available_iaps_c::next_state() - Secondary SSID list:" );

                core_ssid_entry_s* iter_entry = iap_ssid_list_m->first();
                while( iter_entry )
                    {
                    DEBUG1( "core_operation_get_available_iaps_c::next_state() - ID: %u",
                        iter_entry->id );
                    DEBUG1S( "core_operation_get_available_iaps_c::next_state() - SSID: ",
                        iter_entry->ssid.length, iter_entry->ssid.ssid );
                    DEBUG1S( "core_operation_get_available_iaps_c::next_state() - Used SSID: ",
                        iter_entry->used_ssid.length, iter_entry->used_ssid.ssid );

                    iter_entry = iap_ssid_list_m->next();
                    }
                DEBUG( "core_operation_get_available_iaps_c::next_state()" );
                }
#endif // WLAN_CORE_DEEP_DEBUG

            server_m->get_scan_list().remove_entries_by_age(
                server_m->get_device_settings().scan_list_expiration_time );

            server_m->get_scan_list().set_tag(
                core_scan_list_tag_scan );

            if ( server_m->get_core_settings().is_connected() )
                {
                is_split_scan_m = true_t;
                DEBUG( "core_operation_get_available_iaps_c::next_state() - using a split-scan" );
                }
            else
                {
                DEBUG( "core_operation_get_available_iaps_c::next_state() - using a regular scan" );
                }

            /**
             * Limiting algorithm is only used when there's a certain amount of IAPs
             * marked as hidden.
             */
            if ( hidden_iap_count( iap_data_list_m ) >= MIN_HIDDEN_IAPS_FOR_LIMITING_ALGORITHM )
                {
                DEBUG1( "core_operation_get_available_iaps_c::next_state() - %u hidden IAPs defined, using limiting algorithm",
                    hidden_iap_count( iap_data_list_m ) );

                is_limiting_algorithm_used_m = true_t;
                }

            /**
             * Construct a channel mask of channels that have previously contained
             * APs with long beacon intervals.
             */
            core_long_beacon_interval_channels_s& long_beacon_interval_channels(
                server_m->get_core_settings().long_beacon_interval_channels() );

            for ( u8_t idx( 0 ); idx < SCAN_BAND_2DOT4GHZ_MAX_CHANNEL_EURO; ++idx )
                {
                if ( long_beacon_interval_channels.channel_scan_count[idx] )
                    {
                    long_beacon_interval_channels.channel_scan_count[idx]--;
                    const u8_t channel( idx + 1);

                    if ( server_m->get_core_settings().mcc_known() )
                    	{
                    	if ( server_m->get_core_settings().is_valid_channel(
                            SCAN_BAND_2DOT4GHZ,
                            channel ) )
                            {
                            long_broadcast_count_m++;

                            broadcast_channels_m.add(
                                SCAN_BAND_2DOT4GHZ,
                                channel );
                            }
                    	}
                    else
                    	{
                    	long_broadcast_count_m++;
                    	broadcast_channels_m.add(
                                SCAN_BAND_2DOT4GHZ,
                                channel );
                        }
                    }
                }

            /**
             * If we can active scan or the channel mask is empty, we can jump
             * directly to the main broadcast scan.             
             */
            if ( is_active_scan_allowed_m ||
                 !long_broadcast_count_m )
                {
                long_broadcast_count_m = 0;

                return goto_state( core_state_long_broadcast_scan_done );
                }

            const core_scan_channels_s channels(
                broadcast_channels_m.channels() );

            DEBUG2( "core_operation_get_available_iaps_c::next_state() - requesting long passive broadcast scan on channels 0x%02X%02X",
                channels.channels2dot4ghz[1],
                channels.channels2dot4ghz[0] );

            server_m->register_event_handler( this );
            server_m->register_frame_handler( this );

            drivers_m->scan(
                request_id_m,
                core_scan_mode_passive,
                BROADCAST_SSID,
                server_m->get_device_settings().scan_rate,
                channels,
                LONG_PASSIVE_SCAN_CHANNEL_TIME,
                LONG_PASSIVE_SCAN_CHANNEL_TIME,
                is_split_scan_m );

            break;
            }
        case core_state_long_broadcast_scan_start:
            {
            DEBUG( "core_operation_get_available_iaps_c::next_state() - long broadcast scan request completed, waiting for scan completion" );

            break;
            }
        case core_state_long_broadcast_scan_done:
            {
            operation_state_m = core_state_broadcast_scan_start;

            if ( !long_broadcast_count_m )
                {
                broadcast_channels_m.set(
                    server_m->get_core_settings().all_valid_scan_channels() );
                }
            else
                {
                broadcast_channels_m.invert_channels();
                }

            const core_scan_channels_s channels(
                server_m->get_core_settings().valid_scan_channels( broadcast_channels_m.channels() ) );


            if ( !is_active_scan_allowed_m ||
                 is_limiting_algorithm_used_m )
                {
                DEBUG2( "core_operation_get_available_iaps_c::next_state() - requesting passive broadcast scan on channels 0x%02X%02X",
                    channels.channels2dot4ghz[1],
                    channels.channels2dot4ghz[0] );

                server_m->register_event_handler( this );
                server_m->register_frame_handler( this );

                drivers_m->scan(
                    request_id_m,
                    core_scan_mode_passive,
                    BROADCAST_SSID,
                    server_m->get_device_settings().scan_rate,
                    channels,
                    server_m->get_device_settings().passive_scan_min_ch_time,
                    server_m->get_device_settings().passive_scan_max_ch_time,
                    is_split_scan_m );
                }
            else
                {
                DEBUG2( "core_operation_get_available_iaps_c::next_state() - requesting active broadcast scan on channels 0x%02X%02X",
                    channels.channels2dot4ghz[1],
                    channels.channels2dot4ghz[0] );

                server_m->register_event_handler( this );
                server_m->register_frame_handler( this );

                drivers_m->scan(
                    request_id_m,
                    core_scan_mode_active,
                    BROADCAST_SSID,
                    server_m->get_device_settings().scan_rate,
                    channels,
                    server_m->get_device_settings().active_scan_min_ch_time,
                    server_m->get_device_settings().active_scan_max_ch_time,
                    is_split_scan_m );
                }

            break;
            }
        case core_state_broadcast_scan_start:
            {
            DEBUG( "core_operation_get_available_iaps_c::next_state() - broadcast scan request completed, waiting for scan completion" );

            break;            
            }
        case core_state_broadcast_scan_done:
            {            
            if ( server_m->get_core_settings().mcc_known() )
            	{
            	/* When WLAN region is known, the allowed scan channels are known and 
            	 * handled already */
            	return goto_state( core_state_broadcast_scan_done_handle_result );
            	}
            
            operation_state_m = core_state_broadcast_scan_start_unknown_region;

            broadcast_channels_m.set(
                    server_m->get_core_settings().all_valid_scan_channels() );
            
            const core_scan_channels_s channels(
                    server_m->get_core_settings().invalid_scan_channels( broadcast_channels_m.channels() ) );

            server_m->register_event_handler( this );
            server_m->register_frame_handler( this );

            drivers_m->scan(
                    request_id_m,
                    core_scan_mode_passive,
                    BROADCAST_SSID,
                    server_m->get_device_settings().scan_rate,
                    channels,
                    server_m->get_device_settings().passive_scan_min_ch_time,
                    server_m->get_device_settings().passive_scan_max_ch_time,
                    is_split_scan_m );
            
            break;
            }
        case core_state_broadcast_scan_start_unknown_region:
        	{
            DEBUG( "core_operation_get_available_iaps_c::next_state() - broadcast scan request for channels 12 and 13 completed, waiting for scan completion" );

        	break;
        	}
        case core_state_broadcast_scan_complete_unknown_region:
        	{
            operation_state_m = core_state_store_country_info;

            /* If WLAN region was not known before the scan, then check if country information is present
             * in the scan results. APs on channels 12 and 13 must be ignored if country information indicates this. 
             */
            core_scan_list_iterator_by_tag_c iter_country_beacon(
                server_m->get_scan_list(),
                core_scan_list_tag_scan );

            core_wlan_region_e found_region = core_wlan_region_undefined;
            bool_t inconsistent_info( false_t );
            for ( core_ap_data_c* ap_data = iter_country_beacon.first(); ap_data; ap_data = iter_country_beacon.next() )
                {
                core_country_string_s country_info = ap_data->country_info();
                core_wlan_region_e ap_region = core_wlan_region_undefined;
                if ( country_info.country[0] != 0 )                   
                    {
                    ap_region = core_tools_c::convert_country_to_region( country_info );
                    if ( found_region != core_wlan_region_undefined )
                       	{
                       	if ( ap_region != found_region )
                       		{
                       	    inconsistent_info = true_t; 
                       		}
                       	}
                    else
                       	{
                       	found_region = ap_region;
                       	}                        
                    }
                }
            if ( found_region != core_wlan_region_undefined )
               	{
               	if ( !inconsistent_info )
               		{	
               	    region_from_ap_m = found_region;
               		}
               	else
               		{
               		region_from_ap_m = core_wlan_region_etsi;
               		}
               	adaptation_m->store_ap_country_info( request_id_m, region_from_ap_m, inconsistent_info );
               	}
            else
               	{
               	return goto_state( core_state_broadcast_scan_done_handle_result );
               	}
        	break;
        	}
        case core_state_store_country_info:
          	{
            operation_state_m = core_state_broadcast_scan_done_handle_result;
            	
            /* Set the new region information also to core settings */
            DEBUG1( "core_operation_get_available_iaps_c::next_state() - current region %u",
                region_from_ap_m );
            server_m->get_core_settings().set_regional_domain(
                region_from_ap_m );
            server_m->get_core_settings().set_mcc_known( true_t );
            	
            return goto_state( core_state_broadcast_scan_done_handle_result );
            }

        case core_state_broadcast_scan_done_handle_result:
        	{
        	operation_state_m = core_state_direct_scan_start;
        	/* If region information is not known, channels 12 and 13 are now been scanned in passive mode in any case */

        	/* If region is FCC, then ignore and remove the APs from channels 12 and 13 */
            if ( server_m->get_core_settings().regional_domain() == core_wlan_region_fcc )
            	{
            	DEBUG( "core_operation_get_available_iaps_c::next_state() - remove APs that were found on channels 12 and 13" );
            	remove_disallowed_aps();            	
            	}
            
            /* If WLAN regional domain is not ETSI, we have to remove the high channels from
             * from the active channels mask to prevent direct scans on those channels.
             */
            if ( server_m->get_core_settings().regional_domain() != core_wlan_region_etsi )
            	{
                for ( u8_t idx( SCAN_BAND_2DOT4GHZ_MAX_CHANNEL_FCC ); idx < SCAN_BAND_2DOT4GHZ_MAX_CHANNEL_EURO; ++idx )
                    {
                    const u8_t channel( idx + 1);
                    if ( !server_m->get_core_settings().is_valid_channel(
                         SCAN_BAND_2DOT4GHZ,
                         channel ) )
                        {
                        active_channels_m.remove(
                            SCAN_BAND_2DOT4GHZ,
                            channel );
                        }
                    }
            	}

            DEBUG1( "core_operation_get_available_iaps_c::next_state() - broadcast scan done, %u beacon(s)/probe(s) received",
                bss_count_m );

            /**
             * Add the results from the broadcast scan to the scan list so that
             * they can be cached in adaptation.
             */
            core_scan_list_iterator_by_tag_c iter(
                server_m->get_scan_list(),
                core_scan_list_tag_scan );
            for ( core_ap_data_c* ap_data = iter.first(); ap_data; ap_data = iter.next() )
                {
                const core_mac_address_s bssid = ap_data->bssid();

                if ( !server_m->get_core_settings().is_mac_in_permanent_blacklist( bssid ) )
                    {
                    core_tools_c::add_beacon_to_scan_list(
                        client_scan_data_m,
                        *ap_data,
                        ap_data->rcpi() );
                    }
                }

            /**
             * Go through the broadcast scan results and remove matching IAPs.
             */
            process_scan_results(
                core_scan_list_tag_scan );

            /**
             * Direct scans are only needed if active scanning is allowed. If limiting algorithm
             * is used, channel activity based on broadcast scan is also required.
             */
            if ( ( is_active_scan_allowed_m &&
                   !is_limiting_algorithm_used_m ) ||
                 ( is_active_scan_allowed_m &&
                   is_limiting_algorithm_used_m &&
                   bss_count_m ) )
                {
                remove_non_hidden_iaps();

                /**
                 * If limiting algorithm is used, only the current active_channels_m
                 * will be used.
                 */               
                if ( is_limiting_algorithm_used_m )
                    {
                    DEBUG( "core_operation_get_available_iaps_c::next_state() - limiting channels based on WLAN activity" );
                    }
                else
                    {
                    active_channels_m.set(
                        server_m->get_core_settings().all_valid_scan_channels() );
                    }

                /**
                 * iap_data_list_m now contains only hidden ssid IAPs,
                 * that will be direct scanned next.
                 */
                core_iap_data_s* iap = iap_data_list_m.first();
                if( !iap )
                    {
                    DEBUG( "core_operation_get_available_iaps_c::next_state() - nothing to direct scan" );

                    /** No hidden IAP entries in the request, move on.. */
                    return goto_state( core_state_secondary_ssid_check );
                    }

                DEBUG1( "core_operation_get_available_iaps_c::next_state() - going to direct scan %u IAP(s)",
                    iap_data_list_m.count() );

                direct_scanned_ssid_m = iap->ssid;
                DEBUG1( "core_operation_get_available_iaps_c::next_state() - direct scanning IAP ID %u",
                    iap->id );
                DEBUG1S( "core_operation_get_available_iaps_c::next_state() - SSID: ",
                    direct_scanned_ssid_m.length, direct_scanned_ssid_m.ssid );

                server_m->get_scan_list().set_tag(
                    core_scan_list_tag_direct_scan );

                const core_scan_channels_s& channels(
                    active_channels_m.channels() );
                DEBUG2( "core_operation_get_available_iaps_c::next_state() - requesting scan on channels 0x%02X%02X",
                    channels.channels2dot4ghz[1],
                    channels.channels2dot4ghz[0] );

                bss_count_m = 0;

                server_m->register_event_handler( this );
                server_m->register_frame_handler( this );

                drivers_m->scan(
                    request_id_m,
                    core_scan_mode_active,
                    direct_scanned_ssid_m,
                    server_m->get_device_settings().scan_rate,
                    channels,
                    server_m->get_device_settings().active_scan_min_ch_time,
                    server_m->get_device_settings().active_scan_max_ch_time,
                    is_split_scan_m );

                break;
                }
            else
                {
                DEBUG( "core_operation_get_available_iaps_c::next_state() - no reason to direct scan anything" );
                remove_non_hidden_iaps();

                return goto_state( core_state_scanning_done );
                }
            }
        case core_state_direct_scan_start:
            {
            DEBUG( "core_operation_get_available_iaps_c::next_state() - direct scan request completed, waiting for scan completion" );

            break;            
            }
        case core_state_direct_scan_done:
            {
            operation_state_m = core_state_direct_scan_start;

            DEBUG1( "core_operation_get_available_iaps_c::next_state() - direct scan done, %u beacon(s)/probe(s) received",
                bss_count_m );
            DEBUG1( "core_operation_get_available_iaps_c::next_state() - max %u direct scans left",
                iap_data_list_m.count() );           

            /**
             * Go through the direct scan results and remove the matching IAPs.
             */
            process_scan_results(
                core_scan_list_tag_direct_scan );

            /**
             * If the first entry is still the same the one that was scanned,
             * it means the IAP was not found.
             */
            core_iap_data_s* iap = iap_data_list_m.first();
            if ( iap && iap->ssid == direct_scanned_ssid_m )
                {
                DEBUG1( "core_operation_get_available_iaps_c::next_state() - IAP ID %u not found in direct scan",
                    iap->id );

                core_error_e ret = iap_data_list_m.remove( iap );
                if( ret != core_error_ok )
                    {
                    DEBUG1( "core_operation_get_available_iaps_c::next_state() - error while removing IAP entry (%d)",
                        ret );
                    }

                non_found_iaps_list_m.append( iap );
                }

            iap = iap_data_list_m.first();
            if( iap )
                {
                direct_scanned_ssid_m = iap->ssid;
                DEBUG1( "core_operation_get_available_iaps_c::next_state() - direct scanning IAP ID %u",
                    iap->id );              
                DEBUG1S( "core_operation_get_available_iaps_c::next_state() - SSID: ",
                    direct_scanned_ssid_m.length, direct_scanned_ssid_m.ssid );

                server_m->get_scan_list().set_tag(
                    core_scan_list_tag_direct_scan );

                const core_scan_channels_s& channels(
                    active_channels_m.channels() );
                DEBUG2( "core_operation_get_available_iaps_c::next_state() - requesting scan on channels 0x%02X%02X",
                    channels.channels2dot4ghz[1],
                    channels.channels2dot4ghz[0] );

                bss_count_m = 0;

                server_m->register_event_handler( this );
                server_m->register_frame_handler( this );

                drivers_m->scan(
                    request_id_m,
                    core_scan_mode_active,
                    direct_scanned_ssid_m,
                    server_m->get_device_settings().scan_rate,
                    channels,
                    server_m->get_device_settings().active_scan_min_ch_time,
                    server_m->get_device_settings().active_scan_max_ch_time,
                    is_split_scan_m );
                }
            else
                {
                DEBUG( "core_operation_get_available_iaps_c::next_state() - nothing to direct scan" );

                return goto_state( core_state_secondary_ssid_check );
                }

            break;
            }
        case core_state_secondary_ssid_check:
            {
            ASSERT( !iap_data_list_m.count() );
            ASSERT( !iap_data_list_m.first() );

            /**
             * All the IAPs that were not found are contained in non_found_iaps_list_m
             * list. Those that have secondary SSIDs defined, will be moved back to
             * iap_data_list_m for scanning.
             */

            if ( iap_ssid_list_m )
                {
                core_iap_data_s* iap = non_found_iaps_list_m.first();
                while( iap )
                    {
                    DEBUG1( "core_operation_get_available_iaps_c::next_state() - checking IAP ID %u for secondary SSIDs",
                        iap->id );

                    if ( is_id_in_secondary_ssid_list( iap->id ) )
                        {
                        DEBUG( "core_operation_get_available_iaps_c::next_state() - IAP has secondary SSID(s) defined" );

                        /** Using a temporary pointer to guarantee list iterator working. */
                        core_iap_data_s* temp = iap;
                        iap = non_found_iaps_list_m.next();

                        non_found_iaps_list_m.remove( temp );
                        iap_data_list_m.append( temp );
                        }
                    else
                        {
                        iap = non_found_iaps_list_m.next();
                        }
                    }
                }

            if ( !iap_data_list_m.count() )
                {
                DEBUG( "core_operation_get_available_iaps_c::next_state() - no IAPs with secondary SSID(s) defined" );

                return goto_state( core_state_scanning_done );
                }

            DEBUG1( "core_operation_get_available_iaps_c::next_state() - %u IAP(s) with secondary SSID(s) defined",
                iap_data_list_m.count() );

            if ( iap_ssid_list_m )
                {
                (void)iap_ssid_list_m->first();
                }
            (void)iap_data_list_m.first();

            return goto_state( core_state_secondary_ssid_next );
            }
        case core_state_secondary_ssid_next:
            {
            operation_state_m = core_state_secondary_ssid_start;
            
            core_iap_data_s* iap = iap_data_list_m.current();
            if ( !iap )
                {
                DEBUG( "core_operation_get_available_iaps_c::next_state() - all IAPs with secondary SSID(s) handled" );

                return goto_state( core_state_scanning_done );
                }

            DEBUG1( "core_operation_get_available_iaps_c::next_state() - current IAP ID %u",
                iap->id );
            core_ssid_entry_s* entry = iap_ssid_list_m->current();
            while ( entry )
                {
                if ( iap->id == entry->id )
                    {
                    DEBUG1( "core_operation_get_available_iaps_c::next_state() - ID: %u",
                        entry->id );
                    DEBUG1S( "core_operation_get_available_iaps_c::next_state() - SSID: ",
                        entry->ssid.length, entry->ssid.ssid );
                    DEBUG1S( "core_operation_get_available_iaps_c::next_state() - Used SSID: ",
                        entry->used_ssid.length, entry->used_ssid.ssid );                    
 
                    if ( is_ssid_in_scanlist(
                            entry->ssid,
                            client_scan_data_m ) )
                        {                    
                        direct_scanned_ssid_m = entry->used_ssid;

                        DEBUG1S( "core_operation_get_available_iaps_c::next_state() - matching SSID found, doing a direct scan for SSID ",
                            direct_scanned_ssid_m.length, direct_scanned_ssid_m.ssid );

                        server_m->get_scan_list().set_tag(
                            core_scan_list_tag_direct_scan );

                        const core_scan_channels_s& channels(
                            active_channels_m.channels() );
                        DEBUG2( "core_operation_get_available_iaps_c::next_state() - requesting scan on channels 0x%02X%02X",
                            channels.channels2dot4ghz[1],
                            channels.channels2dot4ghz[0] );

                        bss_count_m = 0;

                        server_m->register_event_handler( this );
                        server_m->register_frame_handler( this );

                        drivers_m->scan(
                            request_id_m,
                            core_scan_mode_active,
                            direct_scanned_ssid_m,
                            server_m->get_device_settings().scan_rate,
                            channels,
                            server_m->get_device_settings().active_scan_min_ch_time,
                            server_m->get_device_settings().active_scan_max_ch_time,
                            is_split_scan_m );                    

                        return core_error_request_pending;
                        }
                    else
                        {
                        DEBUG( "core_operation_get_available_iaps_c::next_state() - matching SSID not found, moving to next entry" );

                        entry = iap_ssid_list_m->next();
                        }
                    }
                else
                    {
                    entry = iap_ssid_list_m->next();
                    }
                }

            DEBUG1( "core_operation_get_available_iaps_c::next_state() - no more entries for IAP ID %u, moving to next IAP",
                iap->id );

            remove_secondary_ssid_entries_by_id( iap->id );            
            iap_data_list_m.remove( iap );
            non_found_iaps_list_m.append( iap );
            (void)iap_ssid_list_m->first();
            (void)iap_data_list_m.first();

            return asynch_goto( core_state_secondary_ssid_next );
            }
        case core_state_secondary_ssid_start:
            {
            DEBUG( "core_operation_get_available_iaps_c::next_state() - direct scan request completed, waiting for scan completion" );

            break;
            }
        case core_state_secondary_ssid_done:
            {                
            core_iap_data_s* iap = iap_data_list_m.current();
            ASSERT( iap );

            core_ssid_entry_s* entry = iap_ssid_list_m->current();
            ASSERT( entry );

            core_scan_list_iterator_by_tag_and_ssid_c iter(
                server_m->get_scan_list(),
                core_scan_list_tag_direct_scan,
                direct_scanned_ssid_m );

            for ( core_ap_data_c* ap_data = iter.first(); ap_data; ap_data = iter.next() )
                {
                core_iap_data_c iap_data( *iap );
                if ( core_tools_parser_c::is_ap_compatible_with_iap(
                    server_m->get_wpx_adaptation_instance(),
                    *ap_data,
                    iap_data,
                    server_m->get_core_settings(),
                    false_t,
                    false_t ) == core_connect_ok )
                    {
                    DEBUG1( "core_operation_get_available_iaps_c::next_state() - secondary SSID match for IAP ID %u",
                        iap->id );

                    u32_t* iap_id = new u32_t;
                    if( iap_id )
                        {
                        *iap_id = iap->id;
                        iap_id_list_m.append( iap_id );
                        iap_id = NULL;
                        }

                    remove_secondary_ssid_entries_by_id( iap->id );
                    iap_data_list_m.remove( iap );
                    (void)iap_ssid_list_m->first();
                    (void)iap_data_list_m.first();
                    
                    delete iap;
                    iap = NULL;

                    return goto_state( core_state_secondary_ssid_next );
                    }
                }

            DEBUG1S( "core_operation_get_available_iaps_c::next_state() - no matching SSID ",
                direct_scanned_ssid_m.length, direct_scanned_ssid_m.ssid );

            (void)iap_ssid_list_m->next();

            return goto_state( core_state_secondary_ssid_next );
            }
        case core_state_scanning_done:
            {
            server_m->unregister_frame_handler( this );

            server_m->get_scan_list().print_contents(); // Additional print            

            DEBUG( "core_operation_get_available_iaps_c::next_state() - all scanning done" );
            DEBUG1( "core_operation_get_available_iaps_c::next_state() - scan list contains %u AP(s)",
                client_scan_data_m.Count() );
            DEBUG1( "core_operation_get_available_iaps_c::next_state() - scan list size is %u bytes",
                client_scan_data_m.Size() );
            DEBUG1( "core_operation_get_available_iaps_c::next_state() - %u IAP(s) found",
                iap_id_list_m.count() );
            DEBUG1( "core_operation_get_available_iaps_c::next_state() - %u IAP(s) not found",
                non_found_iaps_list_m.count() );

            /**
             * Detect channels that have APs with long beacon intervals.
             */
            const u32_t long_beacon_interval(
                server_m->get_device_settings().passive_scan_max_ch_time );

            core_long_beacon_interval_channels_s& long_beacon_interval_channels(
                server_m->get_core_settings().long_beacon_interval_channels() );

            core_scan_list_iterator_by_tag_c iter_long_beacon(
                server_m->get_scan_list(),
                core_scan_list_tag_scan );

            for ( core_ap_data_c* ap_data = iter_long_beacon.first(); ap_data; ap_data = iter_long_beacon.next() )
                {
                if ( ap_data->beacon_interval() > long_beacon_interval )
                    {
                    const u8_t channel( ap_data->channel() );                    
                    const core_mac_address_s bssid = ap_data->bssid();

                    DEBUG8( "core_operation_get_available_iaps_c::next_state() - BSSID %02X:%02X:%02X:%02X:%02X:%02X on channel %u has a long beacon interval (%u)",
                        bssid.addr[0], bssid.addr[1], bssid.addr[2],
                        bssid.addr[3], bssid.addr[4], bssid.addr[5],
                        channel,
                        ap_data->beacon_interval() );
                    if ( channel &&
                         channel <= SCAN_BAND_2DOT4GHZ_MAX_CHANNEL_EURO )
                        {
                        long_beacon_interval_channels.channel_scan_count[channel - 1] =
                            server_m->get_device_settings().long_beacon_find_count;                
                        }
                    }
                }

            return core_error_ok;
            }
        default:
            {
            }
        }

    return core_error_request_pending;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void core_operation_get_available_iaps_c::user_cancel(
    bool_t do_graceful_cancel )
    {
    DEBUG( "core_operation_get_available_iaps_c::user_cancel()" );

	/**
	 * If region is FCC and region information is not known, then ignore 
	 * and remove the APs from channels 12 and 13 
	 */
    if ( !server_m->get_core_settings().mcc_known() && 
          server_m->get_core_settings().regional_domain() == core_wlan_region_fcc )
    	{
    	DEBUG( "core_operation_get_available_iaps_c::next_state() - remove APs that were found on channels 12 and 13" );
    	remove_disallowed_aps();            	
    	}
    
    if ( !do_graceful_cancel )
        {
        /**
         * If we are in a middle of a scan, we have to schedule our own
         * event.
         */
        if ( ( operation_state_m == core_state_long_broadcast_scan_start ||
               operation_state_m == core_state_broadcast_scan_start ||
               operation_state_m == core_state_broadcast_scan_start_unknown_region ||
               operation_state_m == core_state_direct_scan_start ||
               operation_state_m == core_state_secondary_ssid_start ) &&         
            server_m->event_handler() == this &&
            server_m->frame_handler() == this )
            {
            asynch_default_user_cancel();

            return;
            }

        /**
         * Everything else is handled by the default implementation.
         */
        core_operation_base_c::user_cancel( do_graceful_cancel );
        }
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void core_operation_get_available_iaps_c::remove_non_hidden_iaps()
    {
    DEBUG("core_operation_get_available_iaps_c::remove_non_hidden_iaps()");

    core_iap_data_s* iap = iap_data_list_m.first();
    while( iap )
        {
        if( !iap->is_hidden )
            {
            DEBUG1("core_operation_get_available_iaps_c::remove_non_hidden_iaps() - removing IAP ID %u",
                iap->id );
            
            core_iap_data_s* temp = iap;
            iap = iap_data_list_m.next();
            core_error_e ret = iap_data_list_m.remove( temp );
            if( ret != core_error_ok )
                {
                DEBUG1("core_operation_get_available_iaps_c::remove_non_hidden_iaps() - error while removing IAP entry (%d)",
                    ret );
                }
            non_found_iaps_list_m.append( temp );
            }
        else
            {
            DEBUG1("core_operation_get_available_iaps_c::remove_non_hidden_iaps() - leaving IAP ID %u",
                iap->id );

            iap = iap_data_list_m.next();
            }
        }
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void core_operation_get_available_iaps_c::process_scan_results(
    u8_t tag )
    {
    DEBUG( "core_operation_get_available_iaps_c::process_scan_results()" );

    core_scan_list_iterator_by_tag_c iter(
        server_m->get_scan_list(),
        tag );

    for ( core_ap_data_c* ap_data = iter.first(); ap_data; ap_data = iter.next() )
        {
        remove_matching_iaps( *ap_data );
        }
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void core_operation_get_available_iaps_c::remove_matching_iaps(
    core_ap_data_c& ap_data )
    {
    DEBUG( "core_operation_get_available_iaps_c::remove_matching_iaps()" );
        
    if ( ap_data.rcpi() < server_m->get_device_settings().iap_availability_rcpi_threshold )
        {
        DEBUG2( "core_operation_get_available_iaps_c::remove_matching_iaps() - AP not considered, signal too weak (%u vs %u)",
            ap_data.rcpi(), server_m->get_device_settings().iap_availability_rcpi_threshold );

        return;
        }
    
    u8_t treshold_val = 
        static_cast<u8_t>(server_m->get_core_settings().rcp_improve_boundary());
        
    /**
     * Loop through the IAP list.
     */
    core_iap_data_s* iap = iap_data_list_m.first();
    while( iap )
        {
        core_iap_data_c iap_data( *iap );
        if ( iap->ssid == ap_data.ssid() &&
             core_tools_parser_c::is_ap_compatible_with_iap(
                server_m->get_wpx_adaptation_instance(),
                ap_data,
                iap_data,
                server_m->get_core_settings(),
                false_t,
                false_t ) == core_connect_ok )
            {
            DEBUG1("core_operation_get_available_iaps_c::remove_matching_iaps() - match found for IAP ID %u",
                iap->id );            

            u32_t* iap_id = new u32_t;
            if( iap_id )
                {
                if ( server_m->get_core_settings().is_iap_id_in_weak_list( iap->id) ) 
                    {
                    DEBUG( "core_operation_get_available_iaps_c::remove_matching_iaps() - IAP ID is in weak list" );
                    
                    if ( ap_data.rcpi() > treshold_val )
                        {
                        DEBUG( "core_operation_get_available_iaps_c::remove_matching_iaps() - RCPI improved enough, remove IAP ID from weak list" );
                        *iap_id = iap->id;
                        iap_id_list_m.append( iap_id );
            		    iap_id = NULL;
                        
                        server_m->get_core_settings().remove_iap_id_from_weak_list( iap->id );
                        }

                    }
                else
                    {
                    *iap_id = iap->id;
                    iap_id_list_m.append( iap_id );
                    iap_id = NULL;
                    }
                }

            /** Using a temporary pointer to guarantee list iterator working. */
            core_iap_data_s* temp = iap;
            
            iap = iap_data_list_m.next();
            core_error_e ret = iap_data_list_m.remove( temp );
            if( ret != core_error_ok )
                {
                if( iap )
                    {
                    DEBUG1("core_operation_get_available_iaps_c::remove_matching_iaps() - error while removing IAP entry (%d)",
                        iap->id );                   
                    }
                }

            delete temp;
            temp = NULL;
            delete iap_id;
            iap_id = NULL;
            }
        else
            {
            iap = iap_data_list_m.next();
            }
        }
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
u8_t core_operation_get_available_iaps_c::hidden_iap_count(
    core_type_list_c<core_iap_data_s>& iap_data_list )
    {
    DEBUG( "core_operation_get_available_iaps_c::hidden_iap_count()" );

    u8_t count( 0 );
    core_iap_data_s* iap = iap_data_list.first();

    while( iap )
        {
        if( iap->is_hidden )
            {
            ++count;
            }

        iap = iap_data_list.next();
        }

    return count;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void core_operation_get_available_iaps_c::remove_disallowed_aps()
    {
    core_type_list_c<core_mac_address_s> remove_ap_list;
                
    core_scan_list_iterator_by_tag_c iter_removed_aps(
        server_m->get_scan_list(),
        core_scan_list_tag_scan );

    core_ap_data_c* ap_data = iter_removed_aps.first();
    while ( ap_data )
        {
        if ( !server_m->get_core_settings().is_valid_channel(
                SCAN_BAND_2DOT4GHZ,
                ap_data->channel() ) )
            {
            core_mac_address_s* ignored_ap = new core_mac_address_s;
            if ( ignored_ap )
                {
                *ignored_ap = ap_data->bssid();
                remove_ap_list.append( ignored_ap );
                }
            }

        ap_data = iter_removed_aps.next(); 
        }

    core_mac_address_s* ignored_bssid = remove_ap_list.first(); 
    while (ignored_bssid )
        {
        server_m->get_scan_list().remove_entries_by_bssid( *ignored_bssid );
        ignored_bssid = remove_ap_list.next();
        }

    remove_ap_list.clear();    
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
bool_t core_operation_get_available_iaps_c::is_id_in_secondary_ssid_list(
    u32_t id )
    {
    for( core_ssid_entry_s* iter = iap_ssid_list_m->first(); iter; iter = iap_ssid_list_m->next() )
        {
        if ( id == iter->id )
            {
            return true_t;
            }
        }

    return false_t;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void core_operation_get_available_iaps_c::remove_secondary_ssid_entries_by_id(
    u32_t id )
    {
    core_ssid_entry_s* iter = iap_ssid_list_m->first();
    while( iter )
        {
        if ( id == iter->id )
            {
            /** Using a temporary pointer to guarantee list iterator working. */
            core_ssid_entry_s* temp = iter;
            iter = iap_ssid_list_m->next();

            core_error_e ret = iap_ssid_list_m->remove( temp );
            if( ret != core_error_ok )
                {
                DEBUG1("core_operation_get_available_iaps_c::remove_secondary_ssid_entries_by_id() - error while removing an entry (%u)",
                    ret );
                }
            
            delete temp;
            temp = NULL;
            }
        else
            {
            iter = iap_ssid_list_m->next();
            }
        }
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
bool_t core_operation_get_available_iaps_c::is_ssid_in_scanlist(
    const core_ssid_s& ssid,
    const ScanList& scan_data )
    {
    u8_t ie_len( 0 );
    const u8_t* ie_data = NULL;
    ScanInfo info( scan_data );

    for ( info.First(); !info.IsDone(); info.Next() )
        {
        if( !info.InformationElement( core_frame_dot11_ie_c::core_frame_dot11_ie_element_id_ssid, ie_len, &ie_data ) )
            {
            if( core_tools_c::compare(
                ssid.ssid, ssid.length,
                ie_data, ie_len ) == 0 )
                {
                return true_t;
                }
            }
        }

    return false_t;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//    
bool_t core_operation_get_available_iaps_c::receive_frame(
    const core_frame_dot11_c* frame,
    u8_t rcpi )
    {
    DEBUG( "core_operation_get_available_iaps_c::receive_frame()" );

    if ( frame->type() != core_frame_dot11_c::core_dot11_type_beacon &&
         frame->type() != core_frame_dot11_c::core_dot11_type_probe_resp )
        {
        DEBUG( "core_operation_get_available_iaps_c::receive_frame() - not a beacon or a probe" );        
        return false_t;
        }

    core_ap_data_c* ap_data = core_ap_data_c::instance(
        server_m->get_wpx_adaptation_instance(),
        frame,
        rcpi,
        false_t );
    if ( ap_data )
        {        
        const core_ssid_s ssid = ap_data->ssid();
        DEBUG1S( "core_operation_get_available_iaps_c::receive_frame() - SSID: ",
            ssid.length, &ssid.ssid[0] );

        const core_mac_address_s bssid = ap_data->bssid();
        DEBUG6( "core_operation_get_available_iaps_c::receive_frame() - BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
            bssid.addr[0], bssid.addr[1], bssid.addr[2], 
            bssid.addr[3], bssid.addr[4], bssid.addr[5] );        

        server_m->get_scan_list().update_entry( *ap_data );

        active_channels_m.add(
            ap_data->band(),
            ap_data->channel() );

        bss_count_m++;

        delete ap_data;
        ap_data = NULL;
        }

    return true_t;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
bool_t core_operation_get_available_iaps_c::notify(
    core_am_indication_e indication )
    {
    if ( indication == core_am_indication_wlan_scan_complete )
        {
        server_m->unregister_event_handler( this );

        if ( operation_state_m == core_state_long_broadcast_scan_start )
            {
            DEBUG( "core_operation_get_available_iaps_c::notify() - long broadcast scan complete" );

            asynch_goto( core_state_long_broadcast_scan_done, CORE_TIMER_IMMEDIATELY );
            }
        else if ( operation_state_m == core_state_broadcast_scan_start )
            {
            DEBUG( "core_operation_get_available_iaps_c::notify() - broadcast scan complete" );

            asynch_goto( core_state_broadcast_scan_done, CORE_TIMER_IMMEDIATELY );
            }
        else if ( operation_state_m == core_state_broadcast_scan_start_unknown_region )
        	{
        	DEBUG( "core_operation_get_available_iaps_c::notify() - broadcast scan complete for channels 12 and 13" );
        	
        	asynch_goto( core_state_broadcast_scan_complete_unknown_region, CORE_TIMER_IMMEDIATELY );
        	}
        else if ( operation_state_m == core_state_direct_scan_start )
            {
            DEBUG( "core_operation_get_available_iaps_c::notify() - direct scan complete" );

            asynch_goto( core_state_direct_scan_done, CORE_TIMER_IMMEDIATELY );
            }
        else if ( operation_state_m == core_state_secondary_ssid_start )
            {
            DEBUG( "core_operation_get_available_iaps_c::notify() - direct scan complete" );

            asynch_goto( core_state_secondary_ssid_done, CORE_TIMER_IMMEDIATELY );
            }
        else
            {
            DEBUG( "core_operation_get_available_iaps_c::notify() - scan complete in unknown state" );
            ASSERT( false_t );
            }

        return true_t;
        }

    return false_t;
    }
