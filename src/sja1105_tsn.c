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


sja1105_status_t SJA1105_SyncTimestamps(sja1105_handle_t *dev_a, sja1105_handle_t *dev_b){

    sja1105_status_t status = SJA1105_OK;

    /* Take the mutexes */
    status = dev_a->callbacks->callback_take_mutex(dev_a->config->timeout, dev_a->callback_context);
    if (status != SJA1105_OK) return status;
    status = dev_b->callbacks->callback_take_mutex(dev_b->config->timeout, dev_b->callback_context);
    if (status != SJA1105_OK) goto release_a;

    bool dev_a_is_master;
    bool dev_b_is_master;
    sja1105_handle_t *master;
    sja1105_handle_t *slave;
    uint32_t reg_data[2];
    uint64_t master_timestamp = 0;
    uint64_t slave_timestamp = 0;
    uint64_t correction;

    /* Check the CAS master bits */
    status = SJA1105_AVBParamsTableGetCASMaster(&dev_a->tables.avb_parameters, &dev_a_is_master);
    if (status != SJA1105_OK) goto end;
    status = SJA1105_AVBParamsTableGetCASMaster(&dev_b->tables.avb_parameters, &dev_b_is_master);
    if (status != SJA1105_OK) goto end;

    /* Check config: exactly one must be a master */
    if (dev_a_is_master == dev_b_is_master){
        status = SJA1105_STATIC_CONF_ERROR;
        goto end;
    }

    /* dev_a is the timestamp master */
    if (dev_a_is_master){
        master = dev_a;
        slave  = dev_b;
    }

    /* dev_b is the timestamp master */
    else {
        master = dev_b;
        slave  = dev_a;
    }

    /* Set sync line high */
    reg_data[0] = SJA1105_STATIC_CTRL_AREA_PTP_VALID | SJA1105_STATIC_CTRL_AREA_PTP_CASSYNC;
    status = SJA1105_WriteRegister(master, SJA1105_CTRL_AREA_PTP_REG_1, reg_data, 1);
    if (status != SJA1105_OK) goto end;

    /* Set sync line low */
    reg_data[0] = SJA1105_STATIC_CTRL_AREA_PTP_VALID;
    status = SJA1105_WriteRegister(master, SJA1105_CTRL_AREA_PTP_REG_1, reg_data, 1);
    if (status != SJA1105_OK) goto end;

    /* Get the master timestamp */
    status = SJA1105_ReadRegister(master, SJA1105_CTRL_AREA_PTP_REG_13, reg_data, 2);
    if (status != SJA1105_OK) goto end;
    master_timestamp  = reg_data[0];
    master_timestamp |= (uint64_t) reg_data[1] << 32;

    /* Get the slave timestamp */
    status = SJA1105_ReadRegister(slave, SJA1105_CTRL_AREA_PTP_REG_13, reg_data, 2);
    if (status != SJA1105_OK) goto end;
    slave_timestamp  = reg_data[0];
    slave_timestamp |= (uint64_t) reg_data[1] << 32;

    /* Put slave's PTPCLKVAL writes into add mode */
    reg_data[0] = SJA1105_STATIC_CTRL_AREA_PTP_VALID | SJA1105_STATIC_CTRL_AREA_PTP_CLKADD;
    status = SJA1105_WriteRegister(slave, SJA1105_CTRL_AREA_PTP_REG_1, reg_data, 1);
    if (status != SJA1105_OK) goto end;

    /* Apply correction to slave */
    correction = master_timestamp - slave_timestamp;
    reg_data[0] = (uint32_t) ((correction & 0x00000000ffffffff) >>  0);
    reg_data[1] = (uint32_t) ((correction & 0xffffffff00000000) >> 32);
    status = SJA1105_WriteRegister(slave, SJA1105_CTRL_AREA_PTP_REG_7, reg_data, 2);
    if (status != SJA1105_OK) goto end;

    /* Give the mutexes and return */

end:

    dev_b->callbacks->callback_give_mutex(dev_b->callback_context);

release_a:

    dev_a->callbacks->callback_give_mutex(dev_a->callback_context);

    return status;
}
