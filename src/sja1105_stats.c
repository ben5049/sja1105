/*
 * sja1105_stats.c
 *
 *  Created on: May 2, 2026
 *      Author: bens1
 */

#include "sja1105.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_io.h"


static sja1105_status_t SJA1105_ReadStatsMAC(sja1105_handle_t *dev, sja1105_stats_mac_level_t *stats) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[SJA1105_MAC_LEVEL_STATS_SIZE * SJA1105_NUM_PORTS];
    uint32_t         counters;
    uint32_t         flags;

    /* MAC Level diagnostics */
    status = SJA1105_ReadRegister(dev, SJA1105_REG_MAC_LEVEL_STATS_PORT0, reg_data, SJA1105_MAC_LEVEL_STATS_SIZE * SJA1105_NUM_PORTS);
    if (status != SJA1105_OK) return status;

    /* Store the relevent statistics in the output */
    for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {

        /* Extract port counters and flags */
        counters = reg_data[SJA1105_MAC_LEVEL_STATS_COUNTERS_PORT_OFFSET(port)];
        flags    = reg_data[SJA1105_MAC_LEVEL_STATS_FLAGS_PORT_OFFSET(port)];

        /* Counters */
        stats->mii_errors[port]       = U32_BYTE_0(counters);
        stats->alignment_errors[port] = U32_BYTE_1(counters);
        stats->sof_errors[port]       = U32_BYTE_2(counters);
        stats->runt_count[port]       = U32_BYTE_3(counters);

        /* Flags */
        stats->agedrp[port]        = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_AGEDRP;
        stats->spcerr[port]        = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_SPCERR;
        stats->drpnona664err[port] = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_DRPNONA664ERR;
        stats->poliecerr[port]     = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_POLIECERR;
        stats->bagdrop[port]       = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_BAGDROP;
        stats->lendrop[port]       = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_LENDROP;
        stats->portdrop[port]      = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_PORTDROP;
        stats->ageprior[port]      = (flags & SJA1105_MAC_LEVEL_STATS_FLAGS_AGEPRIOR_MASK) >> SJA1105_MAC_LEVEL_STATS_FLAGS_AGEPRIOR_SHIFT;
        stats->spcprior[port]      = (flags & SJA1105_MAC_LEVEL_STATS_FLAGS_SPCPRIOR_MASK) >> SJA1105_MAC_LEVEL_STATS_FLAGS_SPCPRIOR_SHIFT;
        stats->pcfbagdrop[port]    = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_PCFBAGDROP;
        stats->domerr[port]        = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_DOMERR;
        stats->intcyov[port]       = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_INTCYOV;
        stats->invtyp[port]        = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_INVTYP;
        stats->memerr[port]        = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_MEMERR;
        stats->memov[port]         = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_MEMOV;
        stats->nomaster[port]      = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_NOMASTER;
        stats->priorerr[port]      = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_PRIORERR;
        stats->tcttimeout[port]    = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_TCTIMEOUT;
        stats->sizeerr[port]       = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_SIZEERR;
        stats->typeerr[port]       = flags & SJA1105_MAC_LEVEL_STATS_FLAGS_TYPEERR;
    }

    return status;
}


static sja1105_status_t SJA1105_ReadStatsHighLevel(sja1105_handle_t *dev, sja1105_stats_high_level_t *stats, bool part1, bool part2) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[MAX(SJA1105_HIGH_LEVEL_STATS1_SIZE * SJA1105_NUM_PORTS,
                                  SJA1105_HIGH_LEVEL_STATS2_SIZE * SJA1105_NUM_PORTS)];

    /* Get the statistics part 1 */
    if (part1) {
        status = SJA1105_ReadRegister(dev, SJA1105_REG_HIGH_LEVEL_STATS1_PORT0, reg_data, SJA1105_HIGH_LEVEL_STATS1_SIZE * SJA1105_NUM_PORTS);
        if (status != SJA1105_OK) return status;

        /* Store the relevent statistics in the output */
        for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {

            /* 64-bit counters  */
            stats->tx_bytes[port]   = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_TXBYTE_L];
            stats->tx_bytes[port]  |= (uint64_t) reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_TXBYTE_H] << 32;
            stats->tx_frames[port]  = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_TXFRM_L];
            stats->tx_frames[port] |= (uint64_t) reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_TXFRM_H] << 32;
            stats->rx_bytes[port]   = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_RXBYTE_L];
            stats->rx_bytes[port]  |= (uint64_t) reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_RXBYTE_H] << 32;
            stats->rx_frames[port]  = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_RXFRM_L];
            stats->rx_frames[port] |= (uint64_t) reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_RXFRM_H] << 32;

            /* 32-bit counters */
            stats->policing_errors[port]          = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_POLERR];
            stats->policing_errors_critical[port] = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_CTPOLERR];
            stats->virtual_link_not_found[port]   = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_VNOTFOUND];
            stats->crc_errors[port]               = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_CRCERR];
            stats->size_errors[port]              = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_SIZEERR];
            stats->unreleased_errors[port]        = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_UNRELEASED];
            stats->vlan_errors[port]              = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_VLANERR];
            stats->non_664_errors[port]           = reg_data[SJA1105_HIGH_LEVEL_STATS1_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS1_N_N664ERR];
        }
    }

    /* Get the statistics part 2 */
    if (part2) {
        status = SJA1105_ReadRegister(dev, SJA1105_REG_HIGH_LEVEL_STATS2_PORT0, reg_data, SJA1105_HIGH_LEVEL_STATS2_SIZE * SJA1105_NUM_PORTS);
        if (status != SJA1105_OK) return status;

        /* Store the relevent statistics in the output */
        for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {

            /* TODO: Queue watermarks */

            /* 32-bit counters */
            stats->not_reachable[port]   = reg_data[SJA1105_HIGH_LEVEL_STATS2_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS2_N_NOT_REACH];
            stats->egress_disabled[port] = reg_data[SJA1105_HIGH_LEVEL_STATS2_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS2_N_EGR_DISABLED];
            stats->partition_drop[port]  = reg_data[SJA1105_HIGH_LEVEL_STATS2_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS2_N_PART_DROP];
            stats->queue_full[port]      = reg_data[SJA1105_HIGH_LEVEL_STATS2_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS2_N_QFULL];
        }
    }

    return status;
}


static sja1105_status_t SJA1105_ReadStatsEthernet(sja1105_handle_t *dev, sja1105_stats_ethernet_t *stats) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[SJA1105_ETHERNET_STATS_SIZE * SJA1105_NUM_PORTS];

    /* Get the statistics part */
    status = SJA1105_ReadRegister(dev, SJA1105_REG_ETHERNET_STATS_PORT0, reg_data, SJA1105_ETHERNET_STATS_SIZE * SJA1105_NUM_PORTS);
    if (status != SJA1105_OK) return status;

    /* Store the relevent statistics in the output */
    for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {
        stats->dropped_utag[port]         = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_UTAG];
        stats->dropped_sitag[port]        = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_SITAG];
        stats->dropped_sotag[port]        = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_SOTAG];
        stats->dropped_dtag[port]         = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_DTAG];
        stats->dropped_illegal_dtag[port] = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_ILL_DTAG];
        stats->dropped_empty_route[port]  = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_EMPTY_ROUTE];
        stats->dropped_no_learn[port]     = reg_data[SJA1105_ETHERNET_STATS_PORT_OFFSET(port) + SJA1105_ETHERNET_STATS_N_DROPS_NOLEARN];
    }

    return status;
}


sja1105_status_t SJA1105_ReadStatsSummary(sja1105_handle_t *dev, sja1105_stats_summary_t *stats) {

    sja1105_status_t           status = SJA1105_OK;
    sja1105_stats_high_level_t stats_high_level;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Get the high level stats */
    status = SJA1105_ReadStatsHighLevel(dev, &stats_high_level, true, false);
    if (status != SJA1105_OK) goto end;

    /* Summarise the stats */
    for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {

        /* Byte counters */
        stats->tx_bytes[port] = stats_high_level.tx_bytes[port];
        stats->rx_bytes[port] = stats_high_level.rx_bytes[port];

        /* Sum dropped frames */
        stats->dropped_frames[port]  = stats_high_level.policing_errors[port];
        stats->dropped_frames[port] += stats_high_level.policing_errors_critical[port];
        stats->dropped_frames[port] += stats_high_level.virtual_link_not_found[port];
        stats->dropped_frames[port] += stats_high_level.crc_errors[port];
        stats->dropped_frames[port] += stats_high_level.size_errors[port];
        stats->dropped_frames[port] += stats_high_level.unreleased_errors[port];
        stats->dropped_frames[port] += stats_high_level.vlan_errors[port];
        stats->dropped_frames[port] += stats_high_level.non_664_errors[port];
    }

    /* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_ReadStatsDetailed(sja1105_handle_t *dev, sja1105_stats_detailed_t *stats) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Get the MAC stats */
    status = SJA1105_ReadStatsMAC(dev, &(stats->mac));
    if (status != SJA1105_OK) goto end;

    /* Get the high level stats (part 1) */
    status = SJA1105_ReadStatsHighLevel(dev, &(stats->high_level), true, true);
    if (status != SJA1105_OK) goto end;

    /* Get the ethernet stats */
    status = SJA1105_ReadStatsEthernet(dev, &(stats->ethernet));
    if (status != SJA1105_OK) goto end;

    /* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}
