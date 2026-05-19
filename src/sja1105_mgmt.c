/*
 * sja1105_mgmt.c
 *
 *  Created on: May 17, 2026
 *      Author: bens1
 */

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_utils.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_tables.h"


/* Create a management route in a single switch */
sja1105_status_t SJA1105_ManagementRouteCreate(sja1105_handle_t *dev, const uint8_t dst_addr[MAC_ADDR_SIZE], uint8_t dst_ports, bool takets, uint8_t tsreg, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Argument checking */
    if (dst_ports >= 1 << SJA1105_NUM_PORTS) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) goto end;

    /* Create variables */
    uint32_t lut_entry[SJA1105_L2ADDR_LU_ENTRY_SIZE] = {0, 0, 0, 0, 0};
    uint32_t reg_data;
    uint8_t  free_entry   = SJA1105_NUM_MGMT_SLOTS;
    uint32_t current_time = SJA1105_GET_TIME_MS();

    /* Look for a free slot */
    for (uint_fast8_t i = 0; i < SJA1105_NUM_MGMT_SLOTS; i++) {
        if (!dev->management_routes.slot_taken[i]) {
            free_entry = i;
            break;
        }
    }

    /* No free slots: attempt to free any slots that have been used up and try again */
    if (free_entry == SJA1105_NUM_MGMT_SLOTS) {

        status = SJA1105_ManagementRouteFree(dev, false);
        if (status != SJA1105_OK) goto end;

        for (uint_fast8_t i = 0; i < SJA1105_NUM_MGMT_SLOTS; i++) {
            if (!dev->management_routes.slot_taken[i]) {
                free_entry = i;
                break;
            }
        }
    }

    /* Still no free slots: attempt to evict an entry based on age */
    if (free_entry == SJA1105_NUM_MGMT_SLOTS) {
        for (uint_fast8_t i = 0; i < SJA1105_NUM_MGMT_SLOTS; i++) {
            if (current_time - dev->management_routes.timestamps[i] > dev->config->mgmt_timeout) {
                dev->management_routes.slot_taken[i] = false;
                dev->events.mgmt_entries_dropped++;

                /* Call the user callback */
                if (dev->management_routes.callbacks[i] != NULL) {
                    status = dev->management_routes.callbacks[i](dev, SJA1105_MGMT_FREE_TIMEOUT, dev->management_routes.contexts[i]);
                    if (status != SJA1105_OK) goto end;
                }

                free_entry = i;
                break;
            }
        }
    }

    /* Still no free slots, return an error */
    if (free_entry == SJA1105_NUM_MGMT_SLOTS) {
        status = SJA1105_NO_FREE_MGMT_ROUTES_ERROR;
        goto end;
    }

    /* Create the lookup table entry */
    lut_entry[SJA1105_MGMT_MGMTVALID_OFFSET]  = SJA1105_MGMT_MGMTVALID_MASK;
    lut_entry[SJA1105_MGMT_INDEX_OFFSET]     |= ((uint32_t) free_entry << SJA1105_MGMT_INDEX_SHIFT) & SJA1105_MGMT_INDEX_MASK;
    lut_entry[SJA1105_MGMT_DESTPORTS_OFFSET] |= ((uint32_t) dst_ports << SJA1105_MGMT_DESTPORTS_SHIFT) & SJA1105_MGMT_DESTPORTS_MASK;
    if (takets) lut_entry[SJA1105_MGMT_TAKETS_OFFSET] |= SJA1105_MGMT_TAKETS_MASK;
    if (tsreg) lut_entry[SJA1105_MGMT_TSREG_OFFSET] |= SJA1105_MGMT_TSREG_MASK;

    /* Copy the destination MAC address into ENTRY[69:22] */

    /* dst_addr[5] is MAC[7:0] -> maps to bits [29:22] */
    lut_entry[0] |= ((uint32_t) dst_addr[5]) << 22;

    /* dst_addr[4] is MAC[15:8] -> maps to bits [37:30] */
    lut_entry[0] |= ((uint32_t) dst_addr[4]) << 30; /* bits [31:30] */
    lut_entry[1] |= ((uint32_t) dst_addr[4]) >> 2;  /* bits [37:32] */

    /* dst_addr[3] is MAC[23:16] -> maps to bits [45:38] */
    lut_entry[1] |= ((uint32_t) dst_addr[3]) << 6;

    /* dst_addr[2] is MAC[31:24] -> maps to bits [53:46] */
    lut_entry[1] |= ((uint32_t) dst_addr[2]) << 14;

    /* dst_addr[1] is MAC[39:32] -> maps to bits [61:54] */
    lut_entry[1] |= ((uint32_t) dst_addr[1]) << 22;

    /* dst_addr[0] is MAC[47:40] -> maps to bits [69:62] */
    lut_entry[1] |= ((uint32_t) dst_addr[0]) << 30; /* bits [63:62] */
    lut_entry[2] |= ((uint32_t) dst_addr[0]) >> 2;  /* bits [69:64] */

    /* Wait for VALID to be 0. */
    status = SJA1105_PollFlag(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, SJA1105_DYN_CONF_L2_LUT_VALID, false);
    if (status != SJA1105_OK) goto end;

    /* Write the entry */
    status = SJA1105_WriteRegister(dev, SJA1105_DYN_CONF_L2_LUT_REG_1, lut_entry, SJA1105_L2ADDR_LU_ENTRY_SIZE);
    if (status != SJA1105_OK) goto end;

    /* Apply the entry */
    reg_data  = SJA1105_DYN_CONF_L2_LUT_VALID;
    reg_data |= SJA1105_DYN_CONF_L2_LUT_RDRWSET;
    reg_data |= SJA1105_DYN_CONF_L2_LUT_MGMTROUTE;
    reg_data |= ((uint32_t) SJA1105_L2_LUT_HOSTCMD_WRITE << SJA1105_L2_LUT_HOSTCMD_SHIFT) & SJA1105_L2_LUT_HOSTCMD_MASK;
    status    = SJA1105_WriteRegister(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, &reg_data, 1);
    if (status != SJA1105_OK) goto end;

    /* TODO: Possibly check ERRORS. It should only be set if VALID was 1 when the write started, which this function made sure it wasn't. */

    /* Wait for VALID to be 0. */
    status = SJA1105_PollFlag(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, SJA1105_DYN_CONF_L2_LUT_VALID, false);
    if (status != SJA1105_OK) goto end;

    /* Update the device struct */
    dev->management_routes.slot_taken[free_entry] = true;
    dev->management_routes.timestamps[free_entry] = current_time;
    dev->management_routes.callbacks[free_entry]  = free_callback;
    dev->management_routes.contexts[free_entry]   = callback_context;

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


/* Create management routes across a complex of switches */
sja1105_status_t SJA1105_ManagementRouteCreateCasc(sja1105_handle_t *dev, const uint8_t dst_addr[MAC_ADDR_SIZE], uint8_t *dst_ports, uint8_t dst_ports_length, bool takets, uint8_t tsreg, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context) {

    sja1105_status_t  status      = SJA1105_OK;
    uint8_t           dev_index   = 0;
    sja1105_handle_t *dev_current = dev;
    bool              non_casc_port;

    /* Check the parameters */
    if (dst_ports == NULL || dst_ports_length == 0) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;

    /* Create routes */
    do {

        /* Create the management route in the current switch.
         * Only take a timestamp if sending from non CASC port */
        non_casc_port = (bool) (dst_ports[dev_index] & ~(1 << dev_current->config->casc_port));
        status        = SJA1105_ManagementRouteCreate(
            dev_current,
            dst_addr,
            dst_ports[dev_index],
            non_casc_port ? takets : false,
            non_casc_port ? tsreg : 0,
            free_callback,
            callback_context);
        if (status != SJA1105_OK) return status;

        /* The management route doesn't extend to the next switch */
        if (!(dst_ports[dev_index] & (1 << dev_current->config->casc_port))) break;

        /* Get the next switch */
        dev_current = SJA1105_GetCasc(dev_current);
        dev_index++;

    } while ((dev_index < dst_ports_length) && (dev_current != NULL));

    return status;
}


/* Create a management route across a complex of switches for a single exit port.
 * The dev argument should be the handle of the switch connected to the host. */
sja1105_status_t SJA1105_ManagementRouteCreateCascSingle(sja1105_handle_t *dev, uint8_t switch_id, uint8_t switch_port, const uint8_t dst_addr[MAC_ADDR_SIZE], bool takets, uint8_t tsreg, uint8_t *depth, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context) {

    sja1105_status_t  status      = SJA1105_OK;
    sja1105_handle_t *dev_current = dev;
    uint8_t           dst_ports[SJA1105_MAX_CASC];

    for (uint_fast8_t i = 0; i < SJA1105_MAX_CASC; i++) {

        /* Found the target switch */
        if (dev_current->config->switch_id == switch_id) {
            dst_ports[i] = 1 << switch_port;
            status       = SJA1105_ManagementRouteCreateCasc(dev, dst_addr, dst_ports, i + 1, takets, tsreg, free_callback, callback_context);
            if (status != SJA1105_OK) return status;

            if (depth != NULL) *depth = i + 1;
            break;
        }

        /* More downstream switches */
        else if ((dev_current->config->casc_port < SJA1105_NUM_PORTS) && (i != (SJA1105_MAX_CASC - 1))) {
            dst_ports[i] = 1 << dev_current->config->casc_port;
        }

        /* No more downstream switches */
        else {
            status = SJA1105_ID_NOT_FOUND_ERROR;
            break;
        }

        /* Get the next switch handle */
        dev_current = SJA1105_GetCasc(dev_current);
        if (dev_current == NULL) {
            status = SJA1105_ID_NOT_FOUND_ERROR;
            return status;
        }
    }

    return status;
}


sja1105_status_t SJA1105_ManagementRouteFree(sja1105_handle_t *dev, bool force) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Initialise variables */
    uint32_t entry[SJA1105_L2ADDR_LU_ENTRY_SIZE] = {0};

    /* Iterate through all management routes */
    for (uint_fast8_t i = 0; i < SJA1105_NUM_MGMT_SLOTS; i++) {

        /* If the slot is full then check whether it has been used */
        if (dev->management_routes.slot_taken[i]) {

            /* Read the entry from the L2 LUT */
            status = SJA1105_L2EntryReadByIndex(dev, i, true, entry);
            if (status != SJA1105_OK) goto end;

            /* If the entry has been used then free it */
            if (!(entry[SJA1105_MGMT_MGMTVALID_OFFSET] & SJA1105_MGMT_MGMTVALID_MASK)) {
                dev->events.mgmt_frames_sent++;
                dev->management_routes.slot_taken[i] = false;

                /* Call the user callback */
                if (dev->management_routes.callbacks[i] != NULL) {
                    status = dev->management_routes.callbacks[i](dev, SJA1105_MGMT_FREE_USED, dev->management_routes.contexts[i]);
                    if (status != SJA1105_OK) goto end;
                }
            }

            /* If force is true then free the entry anyway */
            else if (force) {
                dev->management_routes.slot_taken[i] = false;
                dev->events.mgmt_entries_dropped++;

                /* Call the user callback */
                if (dev->management_routes.callbacks[i] != NULL) {
                    status = dev->management_routes.callbacks[i](dev, SJA1105_MGMT_FREE_FORCED, dev->management_routes.contexts[i]);
                    if (status != SJA1105_OK) goto end;
                }
            }
        }
    }

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_ManagementRouteFreeCasc(sja1105_handle_t *dev, bool force, uint8_t depth) {

    sja1105_status_t  status      = SJA1105_OK;
    sja1105_handle_t *dev_current = dev;

    /* Check parameters */
    if (depth == 0) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;

    /* Iterate through switches */
    for (uint_fast8_t i = 0; i < MIN(depth, SJA1105_MAX_CASC); i++) {

        /* Free the management routes in the current switch */
        status = SJA1105_ManagementRouteFree(dev_current, force);
        if (status != SJA1105_OK) return status;

        /* Get the next switch handle */
        dev_current = SJA1105_GetCasc(dev_current);
        if (dev_current == NULL) break;
    }

    return status;
}
