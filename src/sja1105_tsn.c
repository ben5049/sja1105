/*
 * sja1105_tables.c
 *
 *  Created on: Aug 2, 2025
 *      Author: bens1
 */

#include "assert.h"

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_utils.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_tables.h"


static inline sja1105_status_t SJA1105_WritePTPCtrlReg1(sja1105_handle_t *dev, uint32_t data) {

    /* Apply masking */
    data |= dev->regs.ptp_ctrl_reg_1_set_mask;
    data &= ~dev->regs.ptp_ctrl_reg_1_clear_mask;

    /* Write the reg */
    return SJA1105_WriteRegister(dev, SJA1105_CTRL_AREA_PTP_REG_1, &data, 1);
}


/* DO NOT CHANGE WHILE THERE IS TRAFFIC */
static inline sja1105_status_t SJA1105_UsePTPCLK(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;
    bool             updated;

    updated = (dev->regs.ptp_ctrl_reg_1_set_mask & SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS) != 0;

    dev->regs.ptp_ctrl_reg_1_set_mask   |= SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS;
    dev->regs.ptp_ctrl_reg_1_clear_mask &= ~SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS;

    if (updated) {
        status = SJA1105_WritePTPCtrlReg1(dev, SJA1105_STATIC_CTRL_AREA_PTP_VALID);
    }
    return status;
}


/* DO NOT CHANGE WHILE THERE IS TRAFFIC */
static inline sja1105_status_t SJA1105_UsePTPTSCLK(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;
    bool             updated;

    updated = (dev->regs.ptp_ctrl_reg_1_clear_mask & SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS) != 0;

    dev->regs.ptp_ctrl_reg_1_set_mask   &= ~SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS;
    dev->regs.ptp_ctrl_reg_1_clear_mask |= SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS;

    if (updated) {
        status = SJA1105_WritePTPCtrlReg1(dev, SJA1105_STATIC_CTRL_AREA_PTP_VALID);
    }
    return status;
}


static sja1105_status_t SJA1105_GetCASMaster(sja1105_handle_t *dev_a, sja1105_handle_t *dev_b, sja1105_handle_t **master, sja1105_handle_t **slave) {

    sja1105_status_t status = SJA1105_OK;

    bool dev_a_is_master;
    bool dev_b_is_master;

    /* Check the CAS master bits */
    status = SJA1105_AVBParamsTableGetCASMaster(&dev_a->tables.avb_parameters, &dev_a_is_master);
    if (status != SJA1105_OK) return status;
    status = SJA1105_AVBParamsTableGetCASMaster(&dev_b->tables.avb_parameters, &dev_b_is_master);
    if (status != SJA1105_OK) return status;

    /* Check config: exactly one must be a master */
    if (dev_a_is_master == dev_b_is_master) {
        status = SJA1105_STATIC_CONF_ERROR;
        if (status != SJA1105_OK) return status;
    }

    /* dev_a is the timestamp master */
    if (dev_a_is_master) {
        *master = dev_a;
        *slave  = dev_b;
    }

    /* dev_b is the timestamp master */
    else {
        *master = dev_b;
        *slave  = dev_a;
    }

    return status;
}


static sja1105_status_t _SJA1105_GetTimestampOffset(sja1105_handle_t *master, sja1105_handle_t *slave, int64_t *offset) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[2];
    uint64_t         master_timestamp;
    uint64_t         slave_timestamp;

    /* Set sync line high */
    status = SJA1105_WritePTPCtrlReg1(master, SJA1105_STATIC_CTRL_AREA_PTP_VALID | SJA1105_STATIC_CTRL_AREA_PTP_CASSYNC);
    if (status != SJA1105_OK) return status;

    /* Set sync line low */
    status = SJA1105_WritePTPCtrlReg1(master, SJA1105_STATIC_CTRL_AREA_PTP_VALID);
    if (status != SJA1105_OK) return status;

    /* Get the master timestamp */
    status = SJA1105_ReadRegister(master, SJA1105_CTRL_AREA_PTP_REG_13, reg_data, 2);
    if (status != SJA1105_OK) return status;
    master_timestamp  = reg_data[0];
    master_timestamp |= (uint64_t) reg_data[1] << 32;

    /* Get the slave timestamp */
    status = SJA1105_ReadRegister(slave, SJA1105_CTRL_AREA_PTP_REG_13, reg_data, 2);
    if (status != SJA1105_OK) return status;
    slave_timestamp  = reg_data[0];
    slave_timestamp |= (uint64_t) reg_data[1] << 32;

    *offset = master_timestamp - slave_timestamp;

    return status;
}


sja1105_status_t SJA1105_GetTimestampOffset(sja1105_handle_t *dev_a, sja1105_handle_t *dev_b, int64_t *offset) {

    sja1105_status_t status = SJA1105_OK;

#if SJA1105_PARAM_CHECKS_ENABLED
    if (dev_a == NULL) status = SJA1105_PARAMETER_ERROR;
    if (dev_b == NULL) status = SJA1105_PARAMETER_ERROR;
    if (offset == NULL) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    /* Take the mutexes */
    status = dev_a->callbacks->callback_take_mutex(dev_a->config->timeout, dev_a->callback_context);
    if (status != SJA1105_OK) return status;
    status = dev_b->callbacks->callback_take_mutex(dev_b->config->timeout, dev_b->callback_context);
    if (status != SJA1105_OK) goto release_a;

    sja1105_handle_t *master;
    sja1105_handle_t *slave;
    int64_t           offset_internal;

    /* Find out which is the master and which is the slave */
    status = SJA1105_GetCASMaster(dev_a, dev_b, &master, &slave);
    if (status != SJA1105_OK) goto end;

    /* Get the timestamp offset */
    status = _SJA1105_GetTimestampOffset(master, slave, &offset_internal);
    if (status != SJA1105_OK) goto end;

    /* Account for the fact that handles could have swapped order */
    if (master == dev_a) {
        *offset = offset_internal;
    } else {
        *offset = -offset_internal;
    }

    /* Give the mutexes and return */

end:

    dev_b->callbacks->callback_give_mutex(dev_b->callback_context);

release_a:

    dev_a->callbacks->callback_give_mutex(dev_a->callback_context);

    return status;
}


sja1105_status_t SJA1105_InitTSN(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Use the rate corrected PTP clock instead of the free running PTP timestamp clock */
    status = SJA1105_UsePTPCLK(dev);
    if (status != SJA1105_OK) return status;

    return status;
}


/* This shouldn't be called while there is traffic or it may corrupt timestamps.
 * Note: This function requires two SJA1105 to have their PTP_TS pins physically
 *       connected. */
sja1105_status_t SJA1105_SyncTimestamps(sja1105_handle_t *dev_a, sja1105_handle_t *dev_b) {

    sja1105_status_t status = SJA1105_OK;

#if SJA1105_PARAM_CHECKS_ENABLED
    if (dev_a == NULL) status = SJA1105_PARAMETER_ERROR;
    if (dev_b == NULL) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    /* Take the mutexes */
    status = dev_a->callbacks->callback_take_mutex(dev_a->config->timeout, dev_a->callback_context);
    if (status != SJA1105_OK) return status;
    status = dev_b->callbacks->callback_take_mutex(dev_b->config->timeout, dev_b->callback_context);
    if (status != SJA1105_OK) goto release_a;

    sja1105_handle_t *master;
    sja1105_handle_t *slave;
    uint32_t          reg_data[2];
    int64_t           offset;
    uint64_t          offset_abs;
    int64_t           offset_new;

    /* Find out which is the master and which is the slave */
    status = SJA1105_GetCASMaster(dev_a, dev_b, &master, &slave);
    if (status != SJA1105_OK) goto end;

    /* Slave must use PTPCLK (instead of PTPTSCLK) to support hardware corrections */
    status = SJA1105_UsePTPCLK(slave);
    if (status != SJA1105_OK) goto end;

    /* Get the offset */
    status = _SJA1105_GetTimestampOffset(master, slave, &offset);
    if (status != SJA1105_OK) goto end;
    offset_abs = ABS(offset);

    /* Put slave's PTPCLKVAL writes into add/sub mode */
    status = SJA1105_WritePTPCtrlReg1(slave, SJA1105_STATIC_CTRL_AREA_PTP_VALID | ((offset >= 0) ? SJA1105_STATIC_CTRL_AREA_PTP_CLKADD : SJA1105_STATIC_CTRL_AREA_PTP_CLKSUB));
    if (status != SJA1105_OK) goto end;

    /* Apply correction to slave */
    reg_data[0] = (uint32_t) ((offset_abs & 0x00000000ffffffffULL) >> 0);
    reg_data[1] = (uint32_t) ((offset_abs & 0xffffffff00000000ULL) >> 32);
    status      = SJA1105_WriteRegister(slave, SJA1105_CTRL_AREA_PTP_REG_7, reg_data, 2);
    if (status != SJA1105_OK) goto end;

    /* Get the new offset and check the correction has been applied */
    status = _SJA1105_GetTimestampOffset(master, slave, &offset_new);
    if (status != SJA1105_OK) goto end;

    /* Check the new offset under 1us */
    if (ABS(offset_new) > 125) {
        status = SJA1105_CASC_SYNC_FAILED_ERROR;
        goto end;
    }

    /* Give the mutexes and return */

end:

    dev_b->callbacks->callback_give_mutex(dev_b->callback_context);

release_a:

    dev_a->callbacks->callback_give_mutex(dev_a->callback_context);

    return status;
}


/* Read the rate correct timestamp register. Note the timestamp is in intervals of 8ns */
sja1105_status_t SJA1105_GetCurrentTime(sja1105_handle_t *dev, uint64_t *timestamp) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[2];

#if SJA1105_PARAM_CHECKS_ENABLED
    if (timestamp == NULL) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Read the timestamp register */
    status = SJA1105_ReadRegister(dev, SJA1105_CTRL_AREA_PTP_REG_7, reg_data, 2);
    if (status != SJA1105_OK) goto end;

    /* Store the timestamp */
    *timestamp  = reg_data[0];
    *timestamp |= (uint64_t) reg_data[1] << 32;

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_GetEgressTimestamp(sja1105_handle_t *dev, uint8_t port, uint8_t tsreg, uint64_t *timestamp) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[2];
    uint64_t         current_time;
    uint64_t         reconstructed_timestamp = 0;

#if SJA1105_PARAM_CHECKS_ENABLED
    if (port >= SJA1105_NUM_PORTS) status = SJA1105_PARAMETER_ERROR;
    if (tsreg > 1) status = SJA1105_PARAMETER_ERROR;
    if (timestamp == NULL) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Read the egress timestamp register */
    status = SJA1105_ReadRegister(
        dev,
        SJA1105_PTP_EGR_TS_REG_OFFSET(port, tsreg),
        reg_data,
        SJA1105_PTP_EGR_TS_REG_SIZE);
    if (status != SJA1105_OK) goto end;

    /* Check for an update */
    if (!(reg_data[0] & SJA1105_PTP_EGR_TS_UPDATE)) {
        *timestamp = SJA1105_NO_TIMESTAMP;
        goto end;
    }

    /* Read the current time */
    status = SJA1105_GetCurrentTime(dev, &current_time);
    if (status != SJA1105_OK) goto end;

    /* Reconstruct the egress timestamp */
    reconstructed_timestamp = (current_time & ~SJA1105_PTP_EGR_TS_MASK_64) | (reg_data[1] & SJA1105_PTP_EGR_TS_MASK_32);

    /* Handle the wrap around */
    if (reconstructed_timestamp > current_time) {
        reconstructed_timestamp -= (1ULL << SJA1105_PTP_EGR_TS_BITS);
    }

    *timestamp = reconstructed_timestamp;

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_GetIngressTimestamp(sja1105_handle_t *dev, uint8_t *payload, uint8_t *switch_id, uint8_t *src_port, uint64_t *timestamp) {

    sja1105_status_t status = SJA1105_OK;

#if SJA1105_PARAM_CHECKS_ENABLED
    if (payload == NULL) status = SJA1105_PARAMETER_ERROR;
    if (timestamp == NULL) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    SJA1105_LOCK;

    uint8_t  switch_id_internal;
    uint8_t  src_port_internal;
    uint32_t partial_timestamp;

    uint64_t current_time;
    uint64_t reconstructed_timestamp;

    /* Extract fields from the META frame */
    status = SJA1105_ParseMETAFrame(payload, &switch_id_internal, &src_port_internal, &partial_timestamp);
    if (status != SJA1105_OK) goto end;

    /* Get the handle of the switch that received the META frame */
    do {
        if (dev->config->switch_id == switch_id_internal) break;
        dev = SJA1105_GetCasc(dev);
    } while (dev != NULL);

    /* No switch found with matching ID */
    if (dev == NULL) {
        status = SJA1105_ID_NOT_FOUND_ERROR;
        goto end;
    }

    /* Get the current (64-bit time) */
    status = SJA1105_GetCurrentTime(dev, &current_time);
    if (status != SJA1105_OK) goto end;

    /* Reconstruct the ingress timestamp */
    reconstructed_timestamp = (current_time & ~SJA1105_PTP_ING_TS_MASK_64) | (partial_timestamp & SJA1105_PTP_ING_TS_MASK_32);

    /* Handle the wrap around */
    if (reconstructed_timestamp > current_time) {
        reconstructed_timestamp -= (1ULL << SJA1105_PTP_ING_TS_BITS);
    }

    /* Assign outputs */
    *timestamp = reconstructed_timestamp;
    if (switch_id != NULL) *switch_id = switch_id_internal;
    if (src_port != NULL) *src_port = src_port_internal;

end:

    SJA1105_UNLOCK;
    return status;
}
