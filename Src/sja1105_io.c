/*
 * sja1105_spi.c
 *
 *  Created on: Jul 30, 2025
 *      Author: bens1
 */

#include "sja1105.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_conf.h"


sja1105_status_t __SJA1105_ReadRegister(sja1105_handle_t *dev, uint32_t addr, uint32_t *data, uint32_t size, bool integrity_check) {

    sja1105_status_t      status        = SJA1105_OK;
    static const uint32_t dummy_payload = 0xcccc5555; /* When size = 1 and integrity_check = true, send this payload as we are reading data to confirm everything is working. */

    /* Check the parameters */
    if (size == 0) status = SJA1105_PARAMETER_ERROR;                                  /* Empty check */
    if (addr & ~SJA1105_SPI_ADDR_MASK) status = SJA1105_PARAMETER_ERROR;              /* Start address check */
    if ((addr + size - 1) & ~SJA1105_SPI_ADDR_MASK) status = SJA1105_PARAMETER_ERROR; /* End address check */
    if (status != SJA1105_OK) return status;

    /* Initialise counter for the number of double words remaining to receive */
    uint32_t dwords_remaining = size;
    uint32_t command_frame;
    uint16_t block_size;

    /* If the number of double words to read is greater than SJA1105_SPI_MAX_PAYLOAD_SIZE, then the read needs to be broken into smaller transactions */
    do {

        /* Create the command frame */
        command_frame  = SJA1105_SPI_READ_FRAME;
        command_frame |= ((uint32_t) ((addr + size - dwords_remaining) & SJA1105_SPI_ADDR_MASK)) << SJA1105_SPI_ADDR_POSITION;
        command_frame |= ((uint32_t) (CONSTRAIN(dwords_remaining, 0, SJA1105_SPI_MAX_RX_PAYLOAD_SIZE) & SJA1105_SPI_SIZE_MASK)) << SJA1105_SPI_SIZE_POSITION; /* Note that if the read size = SPI_MAX_PAYLOAD_SIZE it will wrap to 0 as intended */

        /* Start the transaction after a delay (ensures successive transactions meet timing requirements) */
        SJA1105_DELAY_NS(SJA1105_T_SPI_WR);
        SJA1105_WRITE_CS_PIN(SJA1105_PIN_RESET);
        SJA1105_DELAY_NS(SJA1105_T_SPI_LEAD);

        /* Send command frame */
        status = SJA1105_SPI_TRANSMIT(&command_frame, 1);
        if (status != SJA1105_OK) {
            dev->events.spi_errors++;
            goto end;
        }
        dev->events.words_written++;

        /* Insert delay to allow the device to fetch the data */
        SJA1105_DELAY_NS(SJA1105_T_SPI_CTRL_DATA);

        /* Receive data, if size = 1 then send dummy payload to test for faults */
        block_size = CONSTRAIN(dwords_remaining, 0, SJA1105_SPI_MAX_RX_PAYLOAD_SIZE);
        if ((size == 1) && integrity_check) {
            status = SJA1105_SPI_TRANSMIT_RECEIVE(&dummy_payload, data, 1);
            if (status != SJA1105_OK) {
                goto end;
            } else if (data[0] == dummy_payload) {
                status = SJA1105_SPI_ERROR;
                dev->events.spi_errors++;
                goto end;
            }
        } else {
            status = SJA1105_SPI_RECEIVE(&data[size - dwords_remaining], block_size);
            if (status != SJA1105_OK) goto end;
        }

        /* End the transaction */
        SJA1105_DELAY_NS(SJA1105_T_SPI_LAG);
        SJA1105_WRITE_CS_PIN(SJA1105_PIN_SET);

        /* Calculate the double words to receive remaining */
        dwords_remaining -= CONSTRAIN(dwords_remaining, 0, SJA1105_SPI_MAX_RX_PAYLOAD_SIZE);

    } while (dwords_remaining > 0);


end:

    return status;
}


sja1105_status_t SJA1105_ReadRegister(sja1105_handle_t *dev, uint32_t addr, uint32_t *data, uint32_t size) {
    return __SJA1105_ReadRegister(dev, addr, data, size, false);
}


sja1105_status_t SJA1105_ReadRegisterWithCheck(sja1105_handle_t *dev, uint32_t addr, uint32_t *data, uint32_t size) {
    return __SJA1105_ReadRegister(dev, addr, data, size, true);
}


/* Repeatedly read a flag until the flag is set or dev->config->timeout ms have passed
 * If polarity is high then it will poll until the flag is 1
 * If polarity is low then it will poll until the flag is 0
 */
sja1105_status_t SJA1105_PollFlag(sja1105_handle_t *dev, uint32_t addr, uint32_t mask, bool polarity) {

    sja1105_status_t status = SJA1105_OK;
    bool             flag   = false;

    /* Read the flag up to SJA1105_MAX_ATTEMPTS times */
    for (uint_fast8_t i = 0; i < SJA1105_MAX_ATTEMPTS; i++) {
        status = SJA1105_ReadFlag(dev, addr, mask, &flag);
        if (status != SJA1105_OK || flag == polarity) break;
        SJA1105_DELAY_MS(dev->config->timeout / SJA1105_MAX_ATTEMPTS);
    }

    /* If the loop reaches the end and the flag hasn't been set */
    if (status == SJA1105_OK && flag != polarity) {
        status = SJA1105_TIMEOUT;
    }

    return status;
}


/* When using this function an 8-bit type must be passed in for result. E.g. casting a uint32_t* to a bool* means that this function will overwrite the bottom 8-bits but not the other 24. */
sja1105_status_t SJA1105_ReadFlag(sja1105_handle_t *dev, uint32_t addr, uint32_t mask, bool *result) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data;

    status = __SJA1105_ReadRegister(dev, addr, &reg_data, 1, false);

    *result = (reg_data & mask) != 0;

    return status;
}


sja1105_status_t SJA1105_WriteRegister(sja1105_handle_t *dev, uint32_t addr, const uint32_t *data, uint32_t size) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the parameters */
    if (size == 0) status = SJA1105_PARAMETER_ERROR;                                  /* Empty check */
    if (addr & ~SJA1105_SPI_ADDR_MASK) status = SJA1105_PARAMETER_ERROR;              /* Start address check */
    if ((addr + size - 1) & ~SJA1105_SPI_ADDR_MASK) status = SJA1105_PARAMETER_ERROR; /* End address check */
    if (status != SJA1105_OK) return status;

    /* Initialise counter for the number of double words remaining to transmit */
    uint32_t dwords_remaining = size;
    uint32_t command_frame;
    uint16_t block_size;

    /* If the payload size is greater than SJA1105_SPI_MAX_PAYLOAD_SIZE, then the write needs to be broken into smaller transactions */
    do {

        /* Create the command frame */
        command_frame  = SJA1105_SPI_WRITE_FRAME;
        command_frame |= ((uint32_t) ((addr + size - dwords_remaining) & SJA1105_SPI_ADDR_MASK)) << SJA1105_SPI_ADDR_POSITION;

        /* Start the transaction after a delay (ensures successive transactions meet timing requirements) */
        SJA1105_DELAY_NS(SJA1105_T_SPI_WR);
        SJA1105_WRITE_CS_PIN(SJA1105_PIN_RESET);
        SJA1105_DELAY_NS(SJA1105_T_SPI_LEAD);

        /* Send command frame */
        status = SJA1105_SPI_TRANSMIT(&command_frame, 1);
        if (status != SJA1105_OK) goto end;

        /* Send payload */
        block_size = CONSTRAIN(dwords_remaining, 0, SJA1105_SPI_MAX_TX_PAYLOAD_SIZE);
        status     = SJA1105_SPI_TRANSMIT(&data[size - dwords_remaining], block_size);
        if (status != SJA1105_OK) goto end;

        /* End the transaction */
        SJA1105_DELAY_NS(SJA1105_T_SPI_LAG);
        SJA1105_WRITE_CS_PIN(SJA1105_PIN_SET);

        /* Calculate the double words to transmit remaining */
        dwords_remaining -= CONSTRAIN(dwords_remaining, 0, SJA1105_SPI_MAX_TX_PAYLOAD_SIZE);

    } while (dwords_remaining > 0);

end:

    return status;
}


/* Write a table to the chip */
sja1105_status_t SJA1105_WriteTable(sja1105_handle_t *dev, uint32_t addr, sja1105_table_t *table, bool safe) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         header[SJA1105_STATIC_CONF_BLOCK_HEADER + SJA1105_STATIC_CONF_BLOCK_HEADER_CRC];
    uint32_t         command_frame = 0;
    uint32_t         size          = SJA1105_STATIC_CONF_BLOCK_OVERHEAD + *table->size;
    uint32_t         crc_value     = 0;
    bool             crc_error     = false;
    uint32_t         reg_data      = 0;

    /* Check the parameters */
    if (!table->data_crc_valid) status = SJA1105_CRC_ERROR;                           /* CRC must be pre-computed */
    if (addr & ~SJA1105_SPI_ADDR_MASK) status = SJA1105_PARAMETER_ERROR;              /* Start address check */
    if ((addr + size - 1) & ~SJA1105_SPI_ADDR_MASK) status = SJA1105_PARAMETER_ERROR; /* End address check */
    if (status != SJA1105_OK) return status;

    /* Create the command frame */
    command_frame  = SJA1105_SPI_WRITE_FRAME;
    command_frame |= ((uint32_t) (addr & SJA1105_SPI_ADDR_MASK)) << SJA1105_SPI_ADDR_POSITION;

    /* Create the header */
    header[0] = ((uint32_t) *table->id) << 24;
    header[1] = *table->size & SJA1105_STATIC_CONF_BLOCK_SIZE_MASK;
    header[2] = *table->header_crc;

    /* Start the transaction after a delay (ensures successive transactions meet timing requirements) */
    SJA1105_DELAY_NS(SJA1105_T_SPI_WR);
    SJA1105_WRITE_CS_PIN(SJA1105_PIN_RESET);
    SJA1105_DELAY_NS(SJA1105_T_SPI_LEAD);

    /* Send command frame */
    status = SJA1105_SPI_TRANSMIT(&command_frame, 1);
    if (status != SJA1105_OK) goto end;

    /* Send the header */
    status = SJA1105_SPI_TRANSMIT(header, SJA1105_STATIC_CONF_BLOCK_HEADER + SJA1105_STATIC_CONF_BLOCK_HEADER_CRC);
    if (status != SJA1105_OK) goto end;

    /* Send the data */
    status = SJA1105_SPI_TRANSMIT(table->data, *table->size);
    if (status != SJA1105_OK) goto end;

    /* Send the data CRC */
    status = SJA1105_SPI_TRANSMIT(table->data_crc, 1);
    if (status != SJA1105_OK) goto end;

    /* End the transaction */
    SJA1105_DELAY_NS(SJA1105_T_SPI_LAG);
    SJA1105_WRITE_CS_PIN(SJA1105_PIN_SET);

    /* Check the block had no CRC errors if required to */
    if (safe) {
        status = SJA1105_ReadStaticConfFlags(dev, &reg_data);
        if (status != SJA1105_OK) goto end;
        crc_error = reg_data & SJA1105_CRCCHKL_MASK;

        /* If there is a CRC error then report it */
        if (crc_error) {
            status = SJA1105_CRC_ERROR;
            dev->events.crc_errors++;
            goto end;
        }
    }

    /* Only bother computing the CRC if the global crc isn't valid */
    if (!dev->tables.global_crc_valid) {

        /* Add the header to the CRC */
        status = SJA1105_CRC_ACCUMULATE(header, SJA1105_STATIC_CONF_BLOCK_HEADER + SJA1105_STATIC_CONF_BLOCK_HEADER_CRC, &crc_value);
        if (status != SJA1105_OK) goto end;

        /* Add the data to the CRC */
        status = SJA1105_CRC_ACCUMULATE(table->data, *table->size, &crc_value);
        if (status != SJA1105_OK) goto end;

        /* Add the data CRC to the CRC */
        status = SJA1105_CRC_ACCUMULATE(table->data_crc, 1, &crc_value);
        if (status != SJA1105_OK) goto end;
    }

end:

    return status;
}


/* Invalidate a range of L2 look up table indexes. This operation includes both end indexes.
 *
 * This function is esimated to take at least 12us to invalidate one entry. (Assuming VALID is
 * never set when checked and Fspi = 25MHz with negligible CPU overhead).
 *
 * TODO: This function could be sped up by using DMA transfers and grouping the invalidates into
 *       blocks. However this would require the CS pin to be synchronised, meaning the SPI
 *       peripheral would have to be reconfigured.
 */
sja1105_status_t SJA1105_L2LUTInvalidateRange(sja1105_handle_t *dev, uint16_t low_i, uint16_t high_i) {

    sja1105_status_t status = SJA1105_OK;

    /* Argument checking */
    if (low_i > high_i) status = SJA1105_PARAMETER_ERROR;
    if (high_i >= SJA1105_L2ADDR_LU_NUM_ENTRIES) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) goto end;

    /* Initialise the empty register data array: 1 command word + 5 entry words + 1 write entry command */
    static const uint8_t size                                           = 1 + SJA1105_L2ADDR_LU_ENTRY_SIZE + 1;
    uint32_t             reg_data[1 + SJA1105_L2ADDR_LU_ENTRY_SIZE + 1] = {0};

    /* Setup the command word for a write to L2 Address Lookup table reconfiguration register 1 */
    reg_data[0]  = SJA1105_SPI_WRITE_FRAME;
    reg_data[0] |= ((uint32_t) (SJA1105_DYN_CONF_L2_LUT_REG_1 & SJA1105_SPI_ADDR_MASK)) << SJA1105_SPI_ADDR_POSITION;

    /* Setup the L2 Address Lookup table reconfiguration register 0 with the invalidate command */
    reg_data[size - 1]  = SJA1105_DYN_CONF_L2_LUT_VALID;
    reg_data[size - 1] |= SJA1105_DYN_CONF_L2_LUT_RDRWSET;
    reg_data[size - 1] |= ((uint32_t) SJA1105_L2_LUT_HOSTCMD_INVALIDATE_ENTRY << SJA1105_L2_LUT_HOSTCMD_SHIFT) & SJA1105_L2_LUT_HOSTCMD_MASK;

    /* Iterate through all entries to be invalidated */
    for (uint_fast16_t i = low_i; i <= high_i; i++) {

        /* Set the entry index */
        reg_data[1 + SJA1105_L2_LUT_INDEX_OFFSET] = ((uint32_t) i << SJA1105_L2_LUT_INDEX_SHIFT) && SJA1105_L2_LUT_INDEX_MASK;

        /* Wait for VALID to be 0. */
        status = SJA1105_PollFlag(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, SJA1105_DYN_CONF_L2_LUT_VALID, false);
        if (status != SJA1105_OK) goto end;

        /* Start the transaction after a delay (ensures successive transactions meet timing requirements) */
        SJA1105_DELAY_NS(SJA1105_T_SPI_WR);
        SJA1105_WRITE_CS_PIN(SJA1105_PIN_RESET);
        SJA1105_DELAY_NS(SJA1105_T_SPI_LEAD);

        /* Write the invalidate command */
        status = SJA1105_SPI_TRANSMIT(reg_data, size);
        if (status != SJA1105_OK) goto end;

        /* End the transaction */
        SJA1105_DELAY_NS(SJA1105_T_SPI_LAG);
        SJA1105_WRITE_CS_PIN(SJA1105_PIN_SET);
    }

end:

    return status;
}


void SJA1105_FullReset(sja1105_handle_t *dev) {
    SJA1105_WRITE_RST_PIN(SJA1105_PIN_RESET);
    SJA1105_DELAY_NS(SJA1105_T_RST); /* 5us delay */
    SJA1105_WRITE_RST_PIN(SJA1105_PIN_SET);
    SJA1105_DELAY_MS(1);             /* 329us minimum until SPI commands can be written (SJA1105_T_RST_STARTUP_HW). Use a 1ms non-blocking delay so the RTOS can do other work */

    /* Increment the internal reset counter */
    dev->events.resets++;
}


sja1105_status_t SJA1105_CfgReset(sja1105_handle_t *dev) {

    sja1105_status_t status   = SJA1105_OK;
    uint32_t         reg_data = SJA1105_RGU_CFG_RST;

    status = SJA1105_WriteRegister(dev, SJA1105_RGU_REG_RESET_CTRL, &reg_data, 1);
    if (status != SJA1105_OK) return status;

    /* Increment the internal reset counter */
    dev->events.resets++;

    /* Delay to wait for startup */
    SJA1105_DELAY_NS(SJA1105_T_RST_STARTUP_SW);

    return status;
}
