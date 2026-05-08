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

    updated = (bool) (dev->regs.ptp_ctrl_reg_1_set_mask & SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS);

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

    updated = (bool) (dev->regs.ptp_ctrl_reg_1_clear_mask & SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS);

    dev->regs.ptp_ctrl_reg_1_set_mask   &= ~SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS;
    dev->regs.ptp_ctrl_reg_1_clear_mask |= SJA1105_STATIC_CTRL_AREA_PTP_CORRCLK4TS;

    if (updated) {
        status = SJA1105_WritePTPCtrlReg1(dev, SJA1105_STATIC_CTRL_AREA_PTP_VALID);
    }
    return status;
}


static sja1105_status_t SJA1105_GetTimestampOffset(sja1105_handle_t *master, sja1105_handle_t *slave, int64_t *offset) {

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


sja1105_status_t SJA1105_InitTSN(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Use the rate corrected PTP clock instead of the free running PTP timestamp clock */
    status = SJA1105_UsePTPCLK(dev);
    if (status != SJA1105_OK) return status;

    return status;
}


/* This shouldn't be called while there is traffic or it may corrupt timestamps */
sja1105_status_t SJA1105_SyncTimestamps(sja1105_handle_t *dev_a, sja1105_handle_t *dev_b) {

    sja1105_status_t status = SJA1105_OK;

    /* Take the mutexes */
    status = dev_a->callbacks->callback_take_mutex(dev_a->config->timeout, dev_a->callback_context);
    if (status != SJA1105_OK) return status;
    status = dev_b->callbacks->callback_take_mutex(dev_b->config->timeout, dev_b->callback_context);
    if (status != SJA1105_OK) goto release_a;

    bool              dev_a_is_master;
    bool              dev_b_is_master;
    sja1105_handle_t *master;
    sja1105_handle_t *slave;
    uint32_t          reg_data[2];
    int64_t           offset;
    uint64_t          offset_abs;
    int64_t           offset_new;

    /* Check the CAS master bits */
    status = SJA1105_AVBParamsTableGetCASMaster(&dev_a->tables.avb_parameters, &dev_a_is_master);
    if (status != SJA1105_OK) goto end;
    status = SJA1105_AVBParamsTableGetCASMaster(&dev_b->tables.avb_parameters, &dev_b_is_master);
    if (status != SJA1105_OK) goto end;

    /* Check config: exactly one must be a master */
    if (dev_a_is_master == dev_b_is_master) {
        status = SJA1105_STATIC_CONF_ERROR;
        goto end;
    }

    /* dev_a is the timestamp master */
    if (dev_a_is_master) {
        master = dev_a;
        slave  = dev_b;
    }

    /* dev_b is the timestamp master */
    else {
        master = dev_b;
        slave  = dev_a;
    }

    /* Slave must use PTPCLK (instead of PTPTSCLK) to support hardware corrections */
    status = SJA1105_UsePTPCLK(slave);
    if (status != SJA1105_OK) goto end;

    /* Get the offset */
    status = SJA1105_GetTimestampOffset(master, slave, &offset);
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
    status = SJA1105_GetTimestampOffset(master, slave, &offset_new);
    if (status != SJA1105_OK) goto end;

    /* Check the new offset under 10us */
    if (ABS(offset_new) > 1250) {
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
