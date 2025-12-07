/*
 * sja1105_static_conf.c
 *
 *  Created on: Aug 6, 2025
 *      Author: bens1
 */

#include "memory.h"

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_tables.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_io.h"


sja1105_status_t SJA1105_ReadStaticConfFlags(sja1105_handle_t *dev, uint32_t *flags) {

    sja1105_status_t status      = SJA1105_OK;
    uint32_t         reg_data    = 0;
    uint8_t          first_value = 0;
    uint8_t          value       = 0;
    bool             valid       = false;

    /* Read the initial config flags register */
    for (uint_fast8_t i = 0; (i < 32) && !valid; i++) {
        status = SJA1105_ReadRegisterWithCheck(dev, SJA1105_REG_STATIC_CONF_FLAGS, &reg_data, 1);
        if (status != SJA1105_OK) return status;

        /* Extract the free running counter */
        value = reg_data & 0xf;

        /* Record the first non-zero value */
        if ((first_value == 0) && value) {
            first_value = value;
            continue;
        }

        /* Stop when the free running counter has changed from its first recorded (non-zero) value  */
        if ((first_value != 0) && (value != first_value)) valid = true;
    }

    /* Set the output */
    *flags = reg_data & 0xfffffff0;

    /* If the free running counter isn't running then an error has occured */
    if (!valid) status = SJA1105_STATIC_CONF_FLAGS_READ_ERROR;
    if (status != SJA1105_OK) return status;

    return status;
}

sja1105_status_t SJA1105_FreeAllTableMemory(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;
    sja1105_table_t  table;

    /* Go through each table */
    for (uint_fast8_t i = 0; i < SJA1105_NUM_TABLES; i++) {
        table = dev->tables.by_index[i];

        /* Ignore unused tables */
        if (!table.in_use) {
            continue;
        }

        /* Free differently based on the type */
        switch (SJA1105_GET_TABLE_LENGTH_TYPE(*table.id)) {

            /* Fixed length tables don't need anything freed */
            case SJA1105_TABLE_FIXED_LENGTH:
                table.in_use         = false;
                table.data_crc_valid = false;
                break;

            /* Variable length tables need everything freed */
            case SJA1105_TABLE_VARIABLE_LENGTH:
                status = SJA1105_FREE((uint32_t *) table.id);
                if (status != SJA1105_OK) return status;
                status = SJA1105_FREE(table.size);
                if (status != SJA1105_OK) return status;
                status = SJA1105_FREE(table.header_crc);
                if (status != SJA1105_OK) return status;
                status = SJA1105_FREE(table.data);
                if (status != SJA1105_OK) return status;
                status = SJA1105_FREE(table.data_crc);
                if (status != SJA1105_OK) return status;
                table.in_use         = false;
                table.data_crc_valid = false;
                break;

            /* Invalid table ID */
            default:
                status = SJA1105_PARAMETER_ERROR;
                return status;
                break;
        }
    }

    /* Reset the fixed length table array */
    dev->tables.first_free = dev->tables.fixed_length_buffer + 1; /* Leave one space for the device ID */

    dev->tables.global_crc_valid = false;

    return status;
}


sja1105_status_t SJA1105_AllocateFixedLengthTable(sja1105_handle_t *dev, const uint32_t *block, uint8_t block_size) {

    sja1105_status_t   status = SJA1105_OK;
    sja1105_block_id_t id;
    uint32_t           size;
    uint32_t           header_crc;
    uint32_t           data_crc;
    sja1105_table_t   *table;

    /* Check table isn't already in use */
    id    = (block[SJA1105_STATIC_CONF_BLOCK_ID_OFFSET] & SJA1105_STATIC_CONF_BLOCK_ID_MASK) >> SJA1105_STATIC_CONF_BLOCK_ID_SHIFT;
    table = &dev->tables.by_index[SJA1105_GET_TABLE_INDEX(id)];
    if (table->in_use) status = SJA1105_ALREADY_CONFIGURED_ERROR;
    if (status != SJA1105_OK) return status;

    /* Check size argument */
    size = (block[SJA1105_STATIC_CONF_BLOCK_SIZE_OFFSET] & SJA1105_STATIC_CONF_BLOCK_SIZE_MASK) >> SJA1105_STATIC_CONF_BLOCK_SIZE_SHIFT;
    if (block_size != (size + SJA1105_STATIC_CONF_BLOCK_OVERHEAD)) status = SJA1105_STATIC_CONF_ERROR;
    if (status != SJA1105_OK) return status;

    /* Check header CRC */
    status = SJA1105_CRC_RESET();
    if (status != SJA1105_OK) return status;
    status = SJA1105_CRC_ACCUMULATE(block, SJA1105_STATIC_CONF_BLOCK_HEADER, &header_crc);
    if (status != SJA1105_OK) return status;
    if ((header_crc != block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET]) && (block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET] != 0)) {
        status = SJA1105_CRC_ERROR;
        dev->events.crc_errors++;
        return status;
    }

    /* Check data CRC */
    status = SJA1105_CRC_RESET();
    if (status != SJA1105_OK) return status;
    status = SJA1105_CRC_ACCUMULATE(block + SJA1105_STATIC_CONF_DATA_OFFSET, size, &data_crc);
    if (status != SJA1105_OK) return status;
    if ((data_crc != block[block_size - 1]) && (block[block_size - 1] != 0)) {
        status = SJA1105_CRC_ERROR;
        dev->events.crc_errors++;
        return status;
    }

    /* Setup the pointers */
    table->id         = ((uint8_t *) dev->tables.first_free) + 3;
    table->size       = dev->tables.first_free + SJA1105_STATIC_CONF_BLOCK_SIZE_OFFSET;
    table->header_crc = dev->tables.first_free + SJA1105_STATIC_CONF_HEADER_CRC_OFFSET;
    table->data       = dev->tables.first_free + SJA1105_STATIC_CONF_DATA_OFFSET;
    table->data_crc   = dev->tables.first_free + block_size - 1;

    /* Copy in the block and advance the free pointer */
    memcpy(dev->tables.first_free, block, block_size * sizeof(uint32_t));
    dev->tables.first_free += block_size;

    /* Write the CRCs if none were provided */
    if (block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET] == 0) *table->header_crc = header_crc;
    if (block[block_size - 1] == 0) *(table->data_crc) = data_crc;

    /* Check the values were copied correctly */
    if (*table->id != id) status = SJA1105_MEMORY_ERROR;
    if (*table->size != size) status = SJA1105_MEMORY_ERROR;
    if (status != SJA1105_OK) return status;

    /* Set the flags in the entry */
    table->in_use         = true;
    table->data_crc_valid = true;

    return status;
}


sja1105_status_t SJA1105_AllocateVariableLengthTable(sja1105_handle_t *dev, const uint32_t *block, uint8_t block_size) {

    sja1105_status_t   status     = SJA1105_OK;
    sja1105_block_id_t id         = 0xff;
    uint32_t           size       = 0;
    uint32_t           header_crc = 0;
    uint32_t           data_crc   = 0;
    sja1105_table_t   *table      = NULL;

    /* Check table isn't already in use */
    id    = (block[SJA1105_STATIC_CONF_BLOCK_ID_OFFSET] & SJA1105_STATIC_CONF_BLOCK_ID_MASK) >> SJA1105_STATIC_CONF_BLOCK_ID_SHIFT;
    table = &dev->tables.by_index[SJA1105_GET_TABLE_INDEX(id)];
    if (table->in_use) status = SJA1105_ALREADY_CONFIGURED_ERROR;
    if (status != SJA1105_OK) return status;

    /* Check size argument */
    size = (block[SJA1105_STATIC_CONF_BLOCK_SIZE_OFFSET] & SJA1105_STATIC_CONF_BLOCK_SIZE_MASK) >> SJA1105_STATIC_CONF_BLOCK_SIZE_SHIFT;
    if (block_size != (size + SJA1105_STATIC_CONF_BLOCK_OVERHEAD)) status = SJA1105_STATIC_CONF_ERROR;
    if (status != SJA1105_OK) return status;

    /* Check header CRC */
    status = SJA1105_CRC_RESET();
    if (status != SJA1105_OK) return status;
    status = SJA1105_CRC_ACCUMULATE(block, SJA1105_STATIC_CONF_BLOCK_HEADER, &header_crc);
    if (status != SJA1105_OK) return status;
    if ((header_crc != block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET]) && (block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET] != 0)) {
        status = SJA1105_CRC_ERROR;
        dev->events.crc_errors++;
        return status;
    }

    /* Check data CRC */
    status = SJA1105_CRC_RESET();
    if (status != SJA1105_OK) return status;
    status = SJA1105_CRC_ACCUMULATE(block + SJA1105_STATIC_CONF_DATA_OFFSET, size, &data_crc);
    if (status != SJA1105_OK) return status;
    if ((data_crc != block[block_size - 1]) && (block[block_size - 1] != 0)) {
        status = SJA1105_CRC_ERROR;
        dev->events.crc_errors++;
        return status;
    }

    /* Allocate the memory */
    status = SJA1105_ALLOCATE((uint32_t **) &table->id, SJA1105_STATIC_CONF_BLOCK_ID);
    if (status != SJA1105_OK) return status;
    status = SJA1105_ALLOCATE(&table->size, SJA1105_STATIC_CONF_BLOCK_SIZE);
    if (status != SJA1105_OK) return status;
    status = SJA1105_ALLOCATE(&table->header_crc, SJA1105_STATIC_CONF_BLOCK_HEADER_CRC);
    if (status != SJA1105_OK) return status;
    status = SJA1105_ALLOCATE(&table->data, size);
    if (status != SJA1105_OK) return status;
    status = SJA1105_ALLOCATE(&table->data_crc, SJA1105_STATIC_CONF_BLOCK_DATA_CRC);
    if (status != SJA1105_OK) return status;

    /* Copy in the values */
    *table->id         = id;
    *table->size       = size;
    *table->header_crc = (block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET] != 0) ? block[SJA1105_STATIC_CONF_HEADER_CRC_OFFSET] : header_crc;
    memcpy(table->data, block + SJA1105_STATIC_CONF_DATA_OFFSET, size);
    *table->data_crc = (block[block_size - 1] != 0) ? block[block_size - 1] : data_crc;

    /* Set the flags in the entry */
    table->in_use         = true;
    table->data_crc_valid = true;

    return status;
}


sja1105_status_t SJA1105_LoadStaticConfig(sja1105_handle_t *dev, const uint32_t *static_conf, uint32_t static_conf_size) {

    sja1105_status_t status = SJA1105_OK;

    /* Argument checking */
    if (static_conf_size < SJA1105_STATIC_CONF_MIN_SIZE) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;

    /* Free all memory used by tables (also resets flags) */
    status = SJA1105_FreeAllTableMemory(dev);
    if (status != SJA1105_OK) return status;

    /* Check the device ID in the new static config is valid */
    status = SJA1105_CheckDeviceID(dev, static_conf[0]);
    if (status != SJA1105_OK) return status;
    *dev->tables.device_id = static_conf[0];

    /* Setup block variables */
    uint32_t           block_index       = SJA1105_STATIC_CONF_BLOCK_FIRST_OFFSET; /* Index of static_conf_size used for the start of the current block. Starts at 1 because the SWITCH_CORE_ID comes first. */
    uint32_t           block_index_next  = 0;
    sja1105_block_id_t block_id          = 0;
    uint16_t           block_size        = 0; /* Block size listed in the static config */
    uint16_t           block_size_actual = 0; /* Actual block size including headers and CRCs */
    bool               last_block        = false;

    do {

        /* Get the block ID and size */
        block_id   = (static_conf[block_index + SJA1105_STATIC_CONF_BLOCK_ID_OFFSET] & SJA1105_STATIC_CONF_BLOCK_ID_MASK) >> SJA1105_STATIC_CONF_BLOCK_ID_SHIFT;
        block_size = (static_conf[block_index + SJA1105_STATIC_CONF_BLOCK_SIZE_OFFSET] & SJA1105_STATIC_CONF_BLOCK_SIZE_MASK) >> SJA1105_STATIC_CONF_BLOCK_SIZE_SHIFT;

        /* Get the actual block size (with headers and CRCs) */
        if (block_size != 0) {
            block_size_actual = block_size + SJA1105_STATIC_CONF_BLOCK_OVERHEAD;
            block_index_next  = block_index + block_size_actual;
            if ((block_index_next) >= static_conf_size) status = SJA1105_STATIC_CONF_ERROR;
            if (status != SJA1105_OK) return status;
        }

        /* Last block has size = 0 and id = 0 */
        else if (block_id == 0) {
            last_block        = true;
            block_size_actual = SJA1105_STATIC_CONF_BLOCK_LAST_SIZE;
            if ((block_index + block_size_actual) != static_conf_size) status = SJA1105_STATIC_CONF_ERROR;
            if (status != SJA1105_OK) return status;
            break;
        }

        /* Non-final block has size = 0 */
        else {
            status = SJA1105_STATIC_CONF_ERROR;
            if (status != SJA1105_OK) return status;
        }

        /* Check blocks before copying them */
        status = SJA1105_CheckTable(dev, block_id, static_conf + block_index + SJA1105_STATIC_CONF_BLOCK_HEADER + SJA1105_STATIC_CONF_BLOCK_HEADER_CRC, block_size);
        if (status != SJA1105_OK) return status;

        /* Allocate and store the table */
        switch (SJA1105_GET_TABLE_LENGTH_TYPE(block_id)) {

            case SJA1105_TABLE_FIXED_LENGTH:
                status = SJA1105_AllocateFixedLengthTable(dev, static_conf + block_index, block_size_actual);
                break;

            case SJA1105_TABLE_VARIABLE_LENGTH:
                status = SJA1105_AllocateVariableLengthTable(dev, static_conf + block_index, block_size_actual);
                break;

            default:
                break;
        }
        if (status != SJA1105_OK) return status;

        /* Set the block index for the next block */
        block_index = block_index_next;

    } while (!last_block);

    /* Check all required tables are present */
    status = SJA1105_CheckRequiredTables(dev);
    if (status != SJA1105_OK) return status;

    /* Add the CGU config */
    status = SJA1105_ConfigureCGU(dev, false);
    if (status != SJA1105_OK) return status;

    /* Add the ACU config */
    status = SJA1105_ConfigureACU(dev, false);
    if (status != SJA1105_OK) return status;

    /* Reset ingress, egress and learning in MAC config table based SJA1105_PORTS_START_ENABLED */
    status = SJA1105_ResetMACConfTable(dev, false);
    if (status != SJA1105_OK) return status;

    return status;
}


/* Write the static config to the chip */
sja1105_status_t SJA1105_WriteStaticConfig(sja1105_handle_t *dev, bool safe) {

    /* TODO: Fix unsafe mode, currently the device rejects the config when written in unsafe mode */
    assert(safe);

    sja1105_status_t status                                         = SJA1105_OK;
    sja1105_table_t *table                                          = NULL;
    uint32_t         crc_value                                      = 0;
    uint32_t         reg_data                                       = 0;
    uint32_t         offset                                         = 0; /* Number of words written so far */
    uint32_t         end_block[SJA1105_STATIC_CONF_BLOCK_LAST_SIZE] = {0, 0, dev->tables.global_crc};

    /* Calculate all missing data CRCs */
    for (uint_fast8_t table_i = 0; table_i < SJA1105_NUM_TABLES; table_i++) {

        table = &dev->tables.by_index[table_i];

        /* Calculate the CRC. Don't rely on a pre-computed CRCs in safe mode */
        if (table->in_use && (!table->data_crc_valid || safe)) {
            dev->tables.global_crc_valid = false;
            status                       = SJA1105_CRC_RESET();
            if (status != SJA1105_OK) return status;
            status = SJA1105_CRC_ACCUMULATE(table->data, *table->size, &crc_value);
            if (status != SJA1105_OK) return status;
            *table->data_crc      = crc_value;
            table->data_crc_valid = true;
        }
    }

    /* Write the device ID */
    status = SJA1105_WriteRegister(dev, SJA1105_STATIC_CONF_ADDR, dev->tables.device_id, 1);
    if (status != SJA1105_OK) return status;
    offset = 1;

    /* Check the device ID was accepted */
    status = SJA1105_ReadStaticConfFlags(dev, &reg_data);
    if (status != SJA1105_OK) return status;
    if ((reg_data & SJA1105_IDS_MASK) != 0) {
        status = SJA1105_ID_ERROR;
        return status;
    }

    /* Reset the CRC and accumulate the device ID */
    if (!dev->tables.global_crc_valid) {
        status = SJA1105_CRC_RESET();
        if (status != SJA1105_OK) return status;
        status = SJA1105_CRC_ACCUMULATE(dev->tables.device_id, 1, &crc_value);
        if (status != SJA1105_OK) return status;
    }

    /* Safe means tables are written one by one and the CRC error flag is checked after each write */
    if (safe) {
        for (uint_fast8_t i = 0; i < SJA1105_NUM_TABLES; i++) {

            table = &dev->tables.by_index[i];
            if (!table->in_use) continue;

#ifdef DEBUG
            sja1105_block_id_t id = *table->id;
            UNUSED(id);
#endif

            status = SJA1105_WriteTable(dev, SJA1105_STATIC_CONF_ADDR + offset, table, true); /* Note this also accumulates the bytes written into the crc */
            if (status != SJA1105_OK) return status;
            offset += SJA1105_STATIC_CONF_BLOCK_OVERHEAD + *table->size;
        }
    }

    /* Unsafe means tables are written as fast as possible with no checks */
    else {

        /* Write the fixed length tables */
        status = SJA1105_WriteRegister(dev, SJA1105_STATIC_CONF_ADDR + offset, dev->tables.fixed_length_buffer + offset, (dev->tables.first_free - dev->tables.fixed_length_buffer) - offset);
        if (status != SJA1105_OK) return status;
        offset = dev->tables.first_free - dev->tables.fixed_length_buffer;

        /* Calculate the CRC for the fixed length tables */
        if (!dev->tables.global_crc_valid) {
            status = SJA1105_CRC_ACCUMULATE(dev->tables.fixed_length_buffer + 1, offset - 1, &crc_value);
            if (status != SJA1105_OK) return status;
        }

        /* Write the variable length tables */
        for (uint_fast8_t i = 0; i < SJA1105_NUM_TABLES; i++) {

            table = &dev->tables.by_index[i];

            if (table->in_use && (SJA1105_GET_TABLE_LENGTH_TYPE(*table->id) == SJA1105_TABLE_VARIABLE_LENGTH)) {
                status = SJA1105_WriteTable(dev, SJA1105_STATIC_CONF_ADDR + offset, table, false); /* Note this also accumulates the bytes written into the crc */
                if (status != SJA1105_OK) return status;
                offset += SJA1105_STATIC_CONF_BLOCK_OVERHEAD + *table->size;
            }
        }
    }

    /* Finish calculating the global CRC */
    if (!dev->tables.global_crc_valid) {
        status = SJA1105_CRC_ACCUMULATE(end_block, SJA1105_STATIC_CONF_BLOCK_LAST_SIZE - 1, &crc_value);
        if (status != SJA1105_OK) return status;
        end_block[SJA1105_STATIC_CONF_BLOCK_LAST_SIZE - 1] = crc_value;
        dev->tables.global_crc_valid                       = true;
    }

    /* Write the last block */
    status = SJA1105_WriteRegister(dev, SJA1105_STATIC_CONF_ADDR + offset, end_block, SJA1105_STATIC_CONF_BLOCK_LAST_SIZE);
    if (status != SJA1105_OK) return status;

    /* Read the initial config flags register */
    status = SJA1105_ReadStaticConfFlags(dev, &reg_data);
    if (status != SJA1105_OK) return status;

    /* Check for global CRC errors */
    if ((reg_data & SJA1105_CRCCHKG_MASK) != 0) {
        status = SJA1105_CRC_ERROR;
        dev->events.crc_errors++;
        return status;
    }

    /* Check that the config was accepted */
    if ((reg_data & SJA1105_CONFIGS_MASK) == 0) {
        status = SJA1105_STATIC_CONF_ERROR;
        return status;
    }

    return status;
}


sja1105_status_t SJA1105_CheckRequiredTables(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Check if any required tables are missing */
    if (!dev->tables.l2_policing.in_use || dev->tables.l2_policing.size == 0) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.l2_forwarding.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.l2_forwarding_parameters.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.mac_configuration.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.general_parameters.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.xmii_mode_parameters.in_use) status = SJA1105_MISSING_TABLE_ERROR;

    /* Check table dependencies */
    if (!dev->tables.schedule_entry_point_parameters.in_use && dev->tables.schedule.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.schedule_parameters.in_use && dev->tables.schedule.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.schedule_entry_point_parameters.in_use && dev->tables.schedule.in_use) status = SJA1105_MISSING_TABLE_ERROR;
    if (!dev->tables.vl_forwarding_parameters.in_use && dev->tables.vl_forwarding.in_use) status = SJA1105_MISSING_TABLE_ERROR;

    /* TODO: Check if VL Policing and forwarding tables are present if VL lookup is and has any critical entries */

    return status;
}


/* Sync the states of the driver and the chip by reuploading the static config (note this flushes learned MAC addresses) */
sja1105_status_t SJA1105_SyncStaticConfig(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Free management routes */
    if (dev->initialised) {

        /* Cleanup */
        status = SJA1105_ManagementRouteFree(dev, true);
        if (status != SJA1105_OK) return status;

        /* Set the device to not initialised */
        dev->initialised = false;
    }

    /* Purge all configuration data on the chip */
    status = SJA1105_CfgReset(dev);
    if (status != SJA1105_OK) {
        SJA1105_FullReset(dev);
    }

    /* Write the configuration and try again in safe mode if it fails */
    status = SJA1105_WriteStaticConfig(dev, true); /* TODO: Change to false when unsafe mode is fixed */
    if (status == SJA1105_CRC_ERROR) {
        status = SJA1105_WriteStaticConfig(dev, true);
    }
    if (status != SJA1105_OK) return status;

    /* Configure the CGU. Note this was done previously in SJA1105_LoadStaticConfig()
     * and then loaded in SJA1105_WriteStaticConfig(), however it doesn't work when done
     * through static tables for some reason. TODO: Find out why */
    status = SJA1105_ConfigureCGU(dev, true);
    if (status != SJA1105_OK) return status;

    /* Set the device to initialised again and increment the static config upload count */
    dev->initialised = true;
    dev->events.static_conf_uploads++;

    return status;
}
