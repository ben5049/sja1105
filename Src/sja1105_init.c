/*
 * sja1105_init.c
 *
 *  Created on: Jul 30, 2025
 *      Author: bens1
 */

#include "memory.h"
#include "stdlib.h"

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_tables.h"


sja1105_status_t SJA1105_PortConfigure(sja1105_config_t *config, const sja1105_port_t *port_config) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the parameters */
    if (port_config->port_num >= SJA1105_NUM_PORTS) status = SJA1105_PARAMETER_ERROR;
    if (port_config->interface >= SJA1105_INTERFACE_INVALID) status = SJA1105_PARAMETER_ERROR;
    if (port_config->mode >= SJA1105_MODE_INVALID) status = SJA1105_PARAMETER_ERROR;
    if (port_config->speed >= SJA1105_SPEED_INVALID) status = SJA1105_PARAMETER_ERROR;
    if (port_config->voltage >= SJA1105_IO_V_INVALID) status = SJA1105_PARAMETER_ERROR;
    if ((port_config->interface == SJA1105_INTERFACE_MII) && (port_config->speed == SJA1105_SPEED_1G)) status = SJA1105_PARAMETER_ERROR;  /* MII Interface doesn't support 1G speeds */
    if ((port_config->interface == SJA1105_INTERFACE_RMII) && (port_config->speed == SJA1105_SPEED_1G)) status = SJA1105_PARAMETER_ERROR; /* RMII Interface doesn't support 1G speeds */
    if ((port_config->voltage == SJA1105_IO_1V8) && (port_config->interface == SJA1105_INTERFACE_RMII)) status = SJA1105_PARAMETER_ERROR; /* 1V8 RMII not supported */
    if (config->ports[port_config->port_num].configured == true) status = SJA1105_ALREADY_CONFIGURED_ERROR;                               /* Note this may cause an unintended error if the struct uses non-zeroed memory. */
    if (status != SJA1105_OK) return status;

    /* Assign the generic parameters */
    config->ports[port_config->port_num].port_num  = port_config->port_num;
    config->ports[port_config->port_num].interface = port_config->interface;
    config->ports[port_config->port_num].mode      = port_config->mode;
    config->ports[port_config->port_num].speed     = port_config->speed;
    config->ports[port_config->port_num].voltage   = port_config->voltage;

    /* Assign interface specific parameters */
    switch (config->ports[port_config->port_num].interface) {

        case SJA1105_INTERFACE_MII:
            config->ports[port_config->port_num].output_rmii_refclk = false;
            config->ports[port_config->port_num].rx_error_unused    = port_config->rx_error_unused;
            break;

        case SJA1105_INTERFACE_RMII:
            config->ports[port_config->port_num].output_rmii_refclk = port_config->output_rmii_refclk;
            config->ports[port_config->port_num].rx_error_unused    = port_config->rx_error_unused;
            break;

        default:
            config->ports[port_config->port_num].output_rmii_refclk = false;
            config->ports[port_config->port_num].rx_error_unused    = false;
            break;
    }

    /* Mark the port as configured */
    config->ports[port_config->port_num].configured = true;

    return status;
}

sja1105_status_t SJA1105_Init(
    sja1105_handle_t          *dev,
    const sja1105_config_t    *config,
    const sja1105_callbacks_t *callbacks,
    void                      *callback_context,
    uint32_t                   fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE],
    const uint32_t            *static_conf,
    uint32_t                   static_conf_size) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device hasn't already been initialised. Note this may cause an unintended error if the struct uses non-zeroed memory. */
    if (dev->initialised) status = SJA1105_ALREADY_CONFIGURED_ERROR;
    if (status != SJA1105_OK) goto end;

    /* Take the mutex */
    status = callbacks->callback_take_mutex(config->timeout, callback_context);
    if (status != SJA1105_OK) return status;

    /* Only the SJA1105Q has been implemented, others may work but are unsupported. TODO: Add more */
    if (config->variant != VARIANT_SJA1105Q) status = SJA1105_NOT_IMPLEMENTED_ERROR;
    if (status != SJA1105_OK) return status;

    /* Check config parameters */
    if (config->switch_id >= 8) status = SJA1105_PARAMETER_ERROR; /* 3-bit field */
    for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) {
        if (!config->ports[i].configured) status = SJA1105_NOT_CONFIGURED_ERROR;
    }

    /* Check callbacks */
    if (callbacks->callback_write_rst_pin == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_write_cs_pin == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_spi_transmit == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_spi_receive == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_spi_transmit_receive == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_get_time_ms == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_delay_ms == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_delay_ns == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_take_mutex == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_give_mutex == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_allocate == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_free == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_free_all == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_crc_reset == NULL) status = SJA1105_PARAMETER_ERROR;
    if (callbacks->callback_crc_accumulate == NULL) status = SJA1105_PARAMETER_ERROR;

    /* If there are invalid parameters then return */
    if (status != SJA1105_OK) goto end;

    /* Assign the input arguments */
    dev->config           = config;
    dev->callbacks        = callbacks;
    dev->callback_context = callback_context;

    /* Reset tables (note this does not free memory) */
    SJA1105_ResetTables(dev, fixed_length_table_buffer);

    /* Reset event counters */
    SJA1105_ResetEventCounters(dev);

    /* Reset management routes */
    SJA1105_ResetManagementRoutes(dev);

    /* Set pins to a known state */
    SJA1105_WRITE_RST_PIN(SJA1105_PIN_SET);
    SJA1105_WRITE_CS_PIN(SJA1105_PIN_SET);

    /* Clear all previously allocated memory (usually just re-inits the byte pool) */
    status = SJA1105_FREE_ALL();
    if (status != SJA1105_OK) goto end;

    /* Load the static config into the internal tables. This will also write the ACU and CGU tables */
    status = SJA1105_LoadStaticConfig(dev, static_conf, static_conf_size);
    if (status != SJA1105_OK) goto end;

    /* TODO: Configure SGMII PHY/PCS (optional) */


    /* Configure the SJA1105 */

    /* Reset */
    SJA1105_FullReset(dev);

    /* Check the part number matches the specified variant */
    status = SJA1105_CheckPartID(dev);
    if (status != SJA1105_OK) goto end;

    /* Send the static config to the switch chip */
    status = SJA1105_SyncStaticConfig(dev);
    if (status != SJA1105_OK) goto end;

    /* The device has been initialised */
    dev->initialised = true;

    /* Check the status registers */
    status = SJA1105_CheckStatusRegisters(dev);
    if (status != SJA1105_OK) goto end;

/* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_DeInit(sja1105_handle_t *dev, bool hard, bool clear_counters) {

    sja1105_status_t status = SJA1105_OK;

    /* Can't deinit if not initialised */
    if (!dev->initialised) return status;

    /* Take the mutex */
    SJA1105_LOCK;

    /* Save the give mutex function because the callback struct may be unassigend by the end */
    sja1105_callback_give_mutex_t give_mutex = dev->callbacks->callback_give_mutex;

    /* Free table memory and reset struct */
    status = SJA1105_FreeAllTableMemory(dev);
    if (status != SJA1105_OK) goto end;
    SJA1105_ResetTables(dev, hard ? NULL : dev->tables.fixed_length_buffer);

    /* A hard deinit means clearing all config structs too */
    if (hard) {
        dev->config           = NULL;
        dev->callbacks        = NULL;
        dev->callback_context = NULL;
    }

    /* Reset event counters */
    if (clear_counters) {
        SJA1105_ResetEventCounters(dev);
    }

    /* Clear info about management routes */
    SJA1105_ResetManagementRoutes(dev);

    /* Set the device to uninitialised */
    dev->initialised = false;

    /* Give the mutex and return */
end:
    give_mutex(dev);
    return status;
}


/* Note: If the switch is reset then STP must be restarted. If the PHY's links don't go down then it can take a long time for neighbouring switches to notices that this switch bas reset */
sja1105_status_t SJA1105_ReInit(sja1105_handle_t *dev, const uint32_t *static_conf, uint32_t static_conf_size) {

    sja1105_status_t status = SJA1105_OK;

    /* Take the mutex */
    SJA1105_LOCK;

    status = SJA1105_DeInit(dev, false, false);
    if (status != SJA1105_OK) goto end;

    status = SJA1105_Init(dev, dev->config, dev->callbacks, dev->callback_context, dev->tables.fixed_length_buffer, static_conf, static_conf_size);
    if (status != SJA1105_OK) goto end;

/* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


/* THIS WILL NOT FREE MEMORY USED BY TABLES. That should be done before calling this function */
void SJA1105_ResetTables(sja1105_handle_t *dev, uint32_t fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE]) {

    /* Assign the buffer for fixed length tables */
    dev->tables.fixed_length_buffer = fixed_length_table_buffer;
    if (fixed_length_table_buffer != NULL) {
        dev->tables.first_free = dev->tables.fixed_length_buffer + 1; /* Leave one space for the device ID */
    } else {
        dev->tables.first_free = dev->tables.fixed_length_buffer = NULL;
    }

    /* Set all tables to be unused */
    for (uint_fast8_t i = 0; i < SJA1105_NUM_TABLES; i++) {
        dev->tables.by_index[i].in_use         = false;
        dev->tables.by_index[i].id             = NULL;
        dev->tables.by_index[i].size           = NULL;
        dev->tables.by_index[i].header_crc     = NULL;
        dev->tables.by_index[i].data           = NULL;
        dev->tables.by_index[i].data_crc       = NULL;
        dev->tables.by_index[i].data_crc_valid = false;
    }

    /* Reset the static config global CRC */
    dev->tables.global_crc       = 0;
    dev->tables.global_crc_valid = false;
}


/* Clear info about management routes */
void SJA1105_ResetManagementRoutes(sja1105_handle_t *dev) {
    for (uint_fast8_t i = 0; i < SJA1105_NUM_MGMT_SLOTS; i++) {
        dev->management_routes.slot_taken[i] = false;
        dev->management_routes.timestamps[i] = 0;
    }
}


/* Reset event counters */
void SJA1105_ResetEventCounters(sja1105_handle_t *dev) {
    memset(&dev->events, 0, sizeof(sja1105_event_counters_t));
}


sja1105_status_t SJA1105_CheckPartID(sja1105_handle_t *dev) {

    sja1105_status_t status   = SJA1105_OK;
    uint32_t         reg_data = 0;

    /* Read the DEVICE_ID register */
    status = SJA1105_ReadRegisterWithCheck(dev, SJA1105_REG_DEVICE_ID, &reg_data, 1);
    if (status != SJA1105_OK) return status;

    /* Check the device ID config matches the variant */
    switch (reg_data) {

        case SJA1105ET_DEVICE_ID:
            if ((dev->config->variant != VARIANT_SJA1105E) && (dev->config->variant != VARIANT_SJA1105T)) status = SJA1105_ID_ERROR;
            break;

        case SJA1105QS_DEVICE_ID:
            if ((dev->config->variant != VARIANT_SJA1105Q) && (dev->config->variant != VARIANT_SJA1105S)) status = SJA1105_ID_ERROR;
            break;

        case SJA1105PR_DEVICE_ID:
            if ((dev->config->variant != VARIANT_SJA1105P) && (dev->config->variant != VARIANT_SJA1105R)) status = SJA1105_ID_ERROR;
            break;

        default:
            status = SJA1105_ID_ERROR; /* Unknown device */
            break;
    }
    if (status != SJA1105_OK) return status;

    /* Read the PROD_ID register */
    status = SJA1105_ReadRegisterWithCheck(dev, SJA1105_ACU_REG_PROD_ID, &reg_data, 1);
    if (status != SJA1105_OK) return status;

    /* Extract and check the PART_NR */
    switch ((reg_data & SJA1105_PART_NR_MASK) >> SJA1105_PART_NR_OFFSET) {
        case PART_NR_SJA1105ET:
            if ((dev->config->variant != VARIANT_SJA1105E) && (dev->config->variant != VARIANT_SJA1105T)) status = SJA1105_ID_ERROR;
            break;

        case PART_NR_SJA1105P:
            if (dev->config->variant != VARIANT_SJA1105P) status = SJA1105_ID_ERROR;
            break;

        case PART_NR_SJA1105Q:
            if (dev->config->variant != VARIANT_SJA1105Q) status = SJA1105_ID_ERROR;
            break;

        case PART_NR_SJA1105R:
            if (dev->config->variant != VARIANT_SJA1105R) status = SJA1105_ID_ERROR;
            break;

        case PART_NR_SJA1105S:
            if (dev->config->variant != VARIANT_SJA1105S) status = SJA1105_ID_ERROR;
            break;

        default:
            status = SJA1105_ID_ERROR;
            break;
    }
    if (status != SJA1105_OK) return status;

    return status;
}


/* Check the static config ID the device variant */
sja1105_status_t SJA1105_CheckDeviceID(sja1105_handle_t *dev, uint32_t device_id) {

    sja1105_status_t status = SJA1105_OK;

    switch (dev->config->variant) {
        case VARIANT_SJA1105E:
        case VARIANT_SJA1105T:
            if (device_id != SJA1105ET_DEVICE_ID) status = SJA1105_ID_ERROR;
            break;
        case VARIANT_SJA1105P:
        case VARIANT_SJA1105R:
            if (device_id != SJA1105PR_DEVICE_ID) status = SJA1105_ID_ERROR;
            break;
        case VARIANT_SJA1105Q:
        case VARIANT_SJA1105S:
            if (device_id != SJA1105QS_DEVICE_ID) status = SJA1105_ID_ERROR;
            break;
        default:
            status = SJA1105_PARAMETER_ERROR; /* Unknown variant */
            break;
    }

    return status;
}
