/*
 * sja1105.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "assert.h"

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_regs.h"
#include "internal/sja1105_tables.h"


sja1105_status_t SJA1105_PortGetState(sja1105_handle_t *dev, uint8_t port_num, bool *state) {

    sja1105_status_t status = SJA1105_NOT_IMPLEMENTED_ERROR;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* state = true if the port is forwarding, not inhibited, has no errors etc */

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_PortGetSpeed(sja1105_handle_t *dev, uint8_t port_num, sja1105_speed_t *speed) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* For dynamic ports look at the MAC Configuration table */
    if (dev->config->ports[port_num].speed == SJA1105_SPEED_DYNAMIC) {
        status = SJA1105_MACConfTableGetSpeed(&dev->tables.mac_configuration, port_num, speed);
        if (status != SJA1105_OK) return status;
    }

    /* For static ports look at the port config struct */
    else {
        *speed = dev->config->ports[port_num].speed;
    }

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t __SJA1105_PortSetSpeed(sja1105_handle_t *dev, uint8_t port_num, sja1105_speed_t new_speed, bool recurse) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    const sja1105_port_t *port          = &dev->config->ports[port_num];
    sja1105_speed_t       current_speed = SJA1105_SPEED_INVALID;
    bool                  revert        = false;
    sja1105_status_t      revert_status = SJA1105_OK;

    /* Get the current speed */
    status = SJA1105_PortGetSpeed(dev, port_num, &current_speed);
    if (status != SJA1105_OK) goto end;

    /* Check the speed argument */
    if (new_speed == current_speed) { /* New speed should be different */
        status = SJA1105_OK;
        goto end;
    };
    if (port->speed != SJA1105_SPEED_DYNAMIC) status = SJA1105_PARAMETER_ERROR; /* Only ports configured as dynamic can have their speed changed */
    if (new_speed == SJA1105_SPEED_DYNAMIC) status = SJA1105_PARAMETER_ERROR;   /* Speed shouldn't be set to dynamic after the initial configuration */
    if (new_speed >= SJA1105_SPEED_INVALID) status = SJA1105_PARAMETER_ERROR;   /* Invalid speed */
    if (port->configured == false) status = SJA1105_NOT_CONFIGURED_ERROR;       /* Port should have already been configured once with interface and voltage */
    if (status != SJA1105_OK) goto end;

    /* TODO: Set SGMII speed */
    if (port->interface == SJA1105_INTERFACE_SGMII) {
        status = SJA1105_NOT_IMPLEMENTED_ERROR;
        goto end;
    }

    /* Set MII, RMII or RGMII port speed (AH1704 section 6.1) */
    else {

        /* Update the internal MAC Configuration table */
        status = SJA1105_MACConfTableSetSpeed(&dev->tables.mac_configuration, port_num, new_speed);
        if (status != SJA1105_OK) goto end;

        /* Write the internal MAC Configuration table to the device */
        status = SJA1105_MACConfTableWrite(dev, port_num);
        if (status != SJA1105_OK) {
            revert = true;
            goto end;
        }

        /* Configure the ACU with new options */
        status = SJA1105_ConfigureACUPort(dev, port_num, true);
        if (status != SJA1105_OK) {
            revert = true;
            goto end;
        }

        /* Configure the CGU with new options */
        status = SJA1105_ConfigureCGUPort(dev, port_num, true);
        if (status != SJA1105_OK) {
            revert = true;
            goto end;
        }
    }

end:

    /* If the configuration failed midway then try to revert it (do not need to revert dynamic speed since this is only possible when configuring for the first time) */
    if (revert && (current_speed != SJA1105_SPEED_DYNAMIC)) {
        revert_status = __SJA1105_PortSetSpeed(dev, port_num, current_speed, false);
        if (revert_status != SJA1105_OK) status = SJA1105_REVERT_ERROR; /* Error while fixing an error! */
    }

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


/* Allow one layer of recursion so the speed can be reverted if the new value is invalid */
sja1105_status_t SJA1105_PortSetSpeed(sja1105_handle_t *dev, uint8_t port_num, sja1105_speed_t new_speed) {
    return __SJA1105_PortSetSpeed(dev, port_num, new_speed, true);
}


sja1105_status_t SJA1105_PortSetLearning(sja1105_handle_t *dev, uint8_t port_num, bool enable) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    sja1105_status_t revert_status = SJA1105_OK;
    bool             learning      = false;

    /* Get the current port learning status */
    status = SJA1105_MACConfTableGetDynLearn(&dev->tables.mac_configuration, port_num, &learning);
    if (status != SJA1105_OK) goto end;

    /* New setting is different */
    if (learning != enable) {

        /* Update the internal MAC Configuration table */
        status = SJA1105_MACConfTableSetDynLearn(&dev->tables.mac_configuration, port_num, enable);
        if (status != SJA1105_OK) goto end;

        /* Write the internal MAC Configuration table to the device */
        status = SJA1105_MACConfTableWrite(dev, port_num);

        /* If an error occured revert the table */
        if (status != SJA1105_OK) {
            revert_status = SJA1105_MACConfTableSetDynLearn(&dev->tables.mac_configuration, port_num, learning);
            if (revert_status != SJA1105_OK) status = SJA1105_REVERT_ERROR;
            goto end;
        }
    }

/* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_PortGetForwarding(sja1105_handle_t *dev, uint8_t port_num, bool *forwarding) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    bool ingress = false;
    bool egress  = false;

    /* Get the current port ingress and egress status */
    status = SJA1105_MACConfTableGetIngress(&dev->tables.mac_configuration, port_num, &ingress);
    if (status != SJA1105_OK) goto end;
    status = SJA1105_MACConfTableGetEgress(&dev->tables.mac_configuration, port_num, &egress);
    if (status != SJA1105_OK) goto end;

    /* Get the result */
    if (ingress && egress) {
        *forwarding = true;
    } else {
        *forwarding = false;
    }

    /* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_PortSetForwarding(sja1105_handle_t *dev, uint8_t port_num, bool enable) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    sja1105_status_t revert_status = SJA1105_OK;
    bool             revert        = false;
    bool             ingress       = false;
    bool             egress        = false;

    /* Get the current port ingress and egress status */
    status = SJA1105_MACConfTableGetIngress(&dev->tables.mac_configuration, port_num, &ingress);
    if (status != SJA1105_OK) goto end;
    status = SJA1105_MACConfTableGetEgress(&dev->tables.mac_configuration, port_num, &egress);
    if (status != SJA1105_OK) goto end;

    /* New settings are different */
    if ((ingress != enable) || (egress != enable)) {

        /* Update the internal MAC Configuration table */
        status = SJA1105_MACConfTableSetIngress(&dev->tables.mac_configuration, port_num, enable);
        if (status != SJA1105_OK) goto end;
        status = SJA1105_MACConfTableSetEgress(&dev->tables.mac_configuration, port_num, enable);
        if (status != SJA1105_OK) {
            revert = true;
            goto end;
        }

        /* Write the internal MAC Configuration table to the device */
        status = SJA1105_MACConfTableWrite(dev, port_num);
        if (status != SJA1105_OK) {
            revert = true;
            goto end;
        }
    }

end:

    /* If an error occured then revert */
    if (revert) {
        revert_status = SJA1105_MACConfTableSetIngress(&dev->tables.mac_configuration, port_num, ingress);
        if (revert_status != SJA1105_OK) {
            status = SJA1105_REVERT_ERROR;
        }
        revert_status = SJA1105_MACConfTableSetEgress(&dev->tables.mac_configuration, port_num, egress);
        if (revert_status != SJA1105_OK) {
            status = SJA1105_REVERT_ERROR;
        }
    }

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_ReadTemperatureX10(sja1105_handle_t *dev, int16_t *temp_x10) {
    /*TODO: Update table to enable TS*/
    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Setup variables */
    uint8_t  temp_low_i     = 0;
    uint8_t  temp_high_i    = SJA1105_TS_LUT_SIZE;
    uint8_t  guess          = 0;
    uint8_t  previous_guess = 0;
    uint32_t reg_data       = 0;

    /* Check the temperature sensor is enabled TODO: Check the internal ACU table instead */
    status = SJA1105_ReadRegisterWithCheck(dev, SJA1105_ACU_REG_TS_CONFIG, &reg_data, 1);
    if (status != SJA1105_OK) goto end;

    /* Enable it if it isn't TODO: Modify the internal ACU table if it isn't */
    if (reg_data & SJA1105_TS_PD) {
        reg_data &= ~SJA1105_TS_PD;
        status    = SJA1105_WriteRegister(dev, SJA1105_ACU_REG_TS_CONFIG, &reg_data, 1);
        if (status != SJA1105_OK) goto end;
        SJA1105_DELAY_MS(1); /* A slight delay to let the sensor stabilise */
    }

    /* Perform a binary search for the temperature.  */
    for (uint_fast8_t i = 0; i < 7; i++) {

        /* Calculate the next guess by splitting the range in half.
         * If the guess is the same as the previous_guess then the guesses have converged on the answer. */
        guess = (temp_low_i + temp_high_i) / 2;
        if (guess == previous_guess) break;

        /* Write to the TS_CONFIG register */
        reg_data = guess & SJA1105_TS_THRESHOLD_MASK;
        status   = SJA1105_WriteRegister(dev, SJA1105_ACU_REG_TS_CONFIG, &reg_data, 1);
        if (status != SJA1105_OK) goto end;

        /* Read from the TS_STATUS register */
        status = SJA1105_ReadRegisterWithCheck(dev, SJA1105_ACU_REG_TS_STATUS, &reg_data, 1);
        if (status != SJA1105_OK) goto end;

        /* Adjust the range based on the result */
        if (reg_data & SJA1105_TS_EXCEEDED) {
            temp_low_i = guess;
        } else {
            temp_high_i = guess;
        }

        previous_guess = guess;
    }

    /* Check the answer is valid */
    if ((guess >= SJA1105_TS_LUT_SIZE) || (guess == 0)) status = SJA1105_ERROR;
    if (status != SJA1105_OK) goto end;

    /* Get the temp (multiplied by 10). E.g. temp_x10 = 364 means 36.4 degrees */
    /* Note that this is the lower end of the range, and the answer could be up to SJA1105_TS_LUT[guess + 1]*/
    *temp_x10 = SJA1105_TS_LUT[guess];

/* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_ReadTemperature(sja1105_handle_t *dev, float *temp) {
    int16_t          temperatureX10;
    sja1105_status_t status = SJA1105_ReadTemperatureX10(dev, &temperatureX10);
    *temp                   = (float) temperatureX10 / 10.0;
    return status;
}


sja1105_status_t SJA1105_CheckStatusRegisters(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         status_registers[SJA1105_REGULAR_CHECK_SIZE];
    uint32_t         reg_data;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Read the status registers */
    status = SJA1105_ReadRegister(dev, SJA1105_REGULAR_CHECK_ADDR, status_registers, SJA1105_REGULAR_CHECK_SIZE);
    if (status != SJA1105_OK) goto end;

    /* Check the virtual link status register */
    reg_data = status_registers[SJA1105_REG_VL_PART_STATUS - SJA1105_REGULAR_CHECK_ADDR];
    if (reg_data & (SJA1105_VLPARTS | SJA1105_VLROUTES)) {
        uint32_t vlind    = (reg_data & SJA1105_VLPARIND_MASK) >> SJA1105_VLPARIND_SHIFT;
        uint32_t vlparind = (reg_data & SJA1105_VLIND_SHIFT) >> SJA1105_VLIND_SHIFT;

        /* TODO: remove. virtual links haven't been implemented so this is an error */
        UNUSED(vlind);
        UNUSED(vlparind);
        dev->events.spi_errors++;
        return status;
    }


    /* TODO: Check other registers */


    /* Check for dropped frames due to forwarding or no free memory */
    reg_data = status_registers[SJA1105_REG_GENERAL_STATUS_9 - SJA1105_REGULAR_CHECK_ADDR];
    if (reg_data & (SJA1105_FWDS | SJA1105_PARTS)) {

        /* Extract the port number */
        uint8_t port = (reg_data & SJA1105_FWDS_PARTS_PORT_MASK) >> SJA1105_FWDS_PARTS_PORT_SHIFT;
        if (port >= SJA1105_NUM_PORTS) {
            status = SJA1105_INVALID_VALUE_ERROR;
            goto end;
        }

        /* Increment the dropped frame counter */
        dev->events.frames_dropped[port]++;

        // TODO: Look at counter registers (0x400) to figure out the exact error
    }

    /* Check for RAM parity errors */
    if (status_registers[SJA1105_REG_GENERAL_STATUS_10 - SJA1105_REGULAR_CHECK_ADDR] || status_registers[SJA1105_REG_GENERAL_STATUS_11 - SJA1105_REGULAR_CHECK_ADDR]) {
        status = SJA1105_RAM_PARITY_ERROR;
        goto end;
    }

    /* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_ReadStatistics(sja1105_handle_t *dev, sja1105_statistics_t *stats) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data[SJA1105_NUM_PORTS * SJA1105_HIGH_LEVEL_STATS_SIZE];

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Get the statistics */
    status = SJA1105_ReadRegister(dev, SJA1105_REG_HIGH_LEVEL_STATS_PORT0, reg_data, SJA1105_NUM_PORTS * SJA1105_HIGH_LEVEL_STATS_SIZE);
    if (status != SJA1105_OK) goto end;

    /* Store the relevent statistics in the output */
    for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {

        /* Get the byte counters */
        stats->tx_bytes[port]  = reg_data[SJA1105_HIGH_LEVEL_STATS_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS_N_TXBYTE_L];
        stats->tx_bytes[port] |= (uint64_t) reg_data[SJA1105_HIGH_LEVEL_STATS_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS_N_TXBYTE_H] << 32;
        stats->rx_bytes[port]  = reg_data[SJA1105_HIGH_LEVEL_STATS_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS_N_RXBYTE_L];
        stats->rx_bytes[port] |= (uint64_t) reg_data[SJA1105_HIGH_LEVEL_STATS_PORT_OFFSET(port) + SJA1105_HIGH_LEVEL_STATS_N_RXBYTE_H] << 32;

        /* Sum the dropped frame counter registers */
        stats->dropped_frames[port] = 0;
        for (uint_fast8_t err_reg = SJA1105_HIGH_LEVEL_STATS_N_POLERR; err_reg < SJA1105_HIGH_LEVEL_STATS_SIZE; err_reg++) {
            stats->dropped_frames[port] += reg_data[SJA1105_HIGH_LEVEL_STATS_PORT_OFFSET(port) + err_reg];
        }
    }

    // status = SJA1105_ReadRegister(dev, 0x200, reg_data2, (0x01 + 0x01) * 5);
    // if (status != SJA1105_OK) goto end;
    // status = SJA1105_ReadRegister(dev, 0x600, reg_data2, (0x0b + 0x05) * 5);
    // if (status != SJA1105_OK) goto end;
    // status = SJA1105_ReadRegister(dev, 0x1400, reg_data2, (0x16 + 0x01) * 5);
    // if (status != SJA1105_OK) goto end;

    /* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


/* TODO: Disable transmitted clocks */
sja1105_status_t SJA1105_PortSleep(sja1105_handle_t *dev, uint8_t port_num) {

    sja1105_status_t status = SJA1105_NOT_IMPLEMENTED_ERROR;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;


    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


/* TODO: Enable transmitted clocks */
sja1105_status_t SJA1105_PortWake(sja1105_handle_t *dev, uint8_t port_num) {

    sja1105_status_t status = SJA1105_NOT_IMPLEMENTED_ERROR;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;


    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_L2EntryReadByIndex(sja1105_handle_t *dev, uint16_t index, bool managment, uint32_t entry[SJA1105_L2ADDR_LU_ENTRY_SIZE]) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Argument checking */
    if (managment && (index >= SJA1105_NUM_MGMT_SLOTS)) status = SJA1105_PARAMETER_ERROR;
    if (!managment && (index >= SJA1105_L2ADDR_LU_NUM_ENTRIES)) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) goto end;

    /* Initialise variables */
    uint32_t reg_data[SJA1105_L2ADDR_LU_ENTRY_SIZE] = {0};

    /* Wait for VALID to be 0. */
    status = SJA1105_PollFlag(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, SJA1105_DYN_CONF_L2_LUT_VALID, false);
    if (status != SJA1105_OK) goto end;

    /* Write the index to be read */
    if (managment) {
        reg_data[SJA1105_MGMT_INDEX_OFFSET] = ((uint32_t) index << SJA1105_MGMT_INDEX_SHIFT) & SJA1105_MGMT_INDEX_MASK;
    } else {
        reg_data[SJA1105_L2_LUT_INDEX_OFFSET] = ((uint32_t) index << SJA1105_L2_LUT_INDEX_SHIFT) & SJA1105_L2_LUT_INDEX_MASK;
    }
    status = SJA1105_WriteRegister(dev, SJA1105_DYN_CONF_L2_LUT_REG_1, reg_data, SJA1105_L2ADDR_LU_ENTRY_SIZE);
    if (status != SJA1105_OK) goto end;

    /* Write the read command */
    reg_data[0]  = 0; /* Ensures RDRWSET is set to 0 (read) */
    reg_data[0] |= SJA1105_DYN_CONF_L2_LUT_VALID;
    if (managment) reg_data[0] |= SJA1105_DYN_CONF_L2_LUT_MGMTROUTE;
    reg_data[0] |= ((uint32_t) SJA1105_L2_LUT_HOSTCMD_READ << SJA1105_L2_LUT_HOSTCMD_SHIFT) & SJA1105_L2_LUT_HOSTCMD_MASK;
    status       = SJA1105_WriteRegister(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, reg_data, 1);
    if (status != SJA1105_OK) goto end;

    /* Wait for VALID to be 0. */
    status = SJA1105_PollFlag(dev, SJA1105_DYN_CONF_L2_LUT_REG_0, SJA1105_DYN_CONF_L2_LUT_VALID, false);
    if (status != SJA1105_OK) goto end;

    /* Read the entry */
    status = SJA1105_ReadRegister(dev, SJA1105_DYN_CONF_L2_LUT_REG_1, entry, SJA1105_L2ADDR_LU_ENTRY_SIZE);
    if (status != SJA1105_OK) goto end;

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_ManagementRouteCreate(sja1105_handle_t *dev, const uint8_t dst_addr[MAC_ADDR_SIZE], uint8_t dst_ports, bool takets, bool tsreg, void *context) {

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

    /* No free slots: attempt free any slots that have been used up and try again */
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
    if (takets) lut_entry[SJA1105_MGMT_TAKETS_MASK] |= SJA1105_MGMT_TAKETS_MASK;
    if (tsreg) lut_entry[SJA1105_MGMT_TSREG_OFFSET] |= SJA1105_MGMT_TSREG_MASK;

    /* Copy the destination MAC address into ENTRY[69:22] */
    lut_entry[0] |= (uint32_t) dst_addr[0] << 22; /* [29:22] */
    lut_entry[0] |= (uint32_t) dst_addr[1] << 30; /* [31:30] */
    lut_entry[1] |= (uint32_t) dst_addr[1] >> 2;  /* [37:32] */
    lut_entry[1] |= (uint32_t) dst_addr[2] << 6;  /* [45:38] */
    lut_entry[1] |= (uint32_t) dst_addr[3] << 14; /* [53:46] */
    lut_entry[1] |= (uint32_t) dst_addr[4] << 22; /* [61:54] */
    lut_entry[1] |= (uint32_t) dst_addr[5] << 30; /* [63:62] */
    lut_entry[2] |= (uint32_t) dst_addr[5] >> 2;  /* [69:64] */

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
    dev->management_routes.contexts[free_entry]   = context;

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
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
            if (entry[SJA1105_MGMT_MGMTVALID_OFFSET] & SJA1105_MGMT_MGMTVALID_MASK) {
                dev->events.mgmt_frames_sent++;
                dev->management_routes.slot_taken[i] = false;
            }

            /* If force is true then free the entry anyway */
            else if (force) {
                dev->management_routes.slot_taken[i] = false;
                dev->events.mgmt_entries_dropped++;
            }
        }
    }

end:

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


/* Invalidates all entries in the TCAM (L2 lookup table, sometimes also called the MAC address
 * table, the address translation unit (ATU) or forwarding database (FDB)).
 */
sja1105_status_t SJA1105_FlushTCAM(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Re-writing the static config flushes the TCAM */
    status = SJA1105_SyncStaticConfig(dev);

    /* TODO: only invalidate LUT entries if its faster than a reconfig */
    /* Invalidate all entries TODO: Start at first dynamic entry? */
    /* TODO: Upload static config again? */
    // status = SJA1105_L2LUTInvalidateRange(dev, 0, SJA1105_L2ADDR_LU_NUM_ENTRIES - 1);

    /* Give the mutex and return */
    SJA1105_UNLOCK;
    return status;
}


sja1105_status_t SJA1105_MACAddrTrapTest(sja1105_handle_t *dev, const uint8_t *addr, bool *trapped, bool *send_meta, bool *incl_srcpt) {

    sja1105_status_t status = SJA1105_OK;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Get the MAC filters */
    sja1105_mac_filters_t filters;
    status = SJA1105_GetMACFilters(dev, &filters);

    /* Test against the first filter */
    *trapped = true;
    for (uint_fast8_t i = 0; i < MAC_ADDR_SIZE; i++) {
        if ((addr[i] & filters.mac_flt0[i]) != filters.mac_fltres0[i]) {
            *trapped = false;
            break;
        }
    }
    if (*trapped) {
        *send_meta  = filters.send_meta0;
        *incl_srcpt = filters.incl_srcpt0;
        goto end;
    }

    /* Test against the second filter */
    *trapped = true;
    for (uint_fast8_t i = 0; i < MAC_ADDR_SIZE; i++) {
        if ((addr[i] & filters.mac_flt1[i]) != filters.mac_fltres1[i]) {
            *trapped = false;
            break;
        }
    }
    if (*trapped) {
        *send_meta  = filters.send_meta1;
        *incl_srcpt = filters.incl_srcpt1;
    }

/* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}


/* Read current table data from the SJA1105 into the device struct. Can be used to ensure shadow tables are the same */
sja1105_status_t SJA1105_ReadAllTables(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_NOT_IMPLEMENTED_ERROR;

    /* Check the device is initialised and take the mutex */
    SJA1105_LOCK;

    /* Read all the MAC config table entries */
    for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) {
        status = SJA1105_MACConfTableRead(dev, i);
        if (status != SJA1105_OK) goto end;
    }

    /* Read all the L2 forwarding table entries */
    for (uint_fast8_t i = 0; i < SJA1105_STATIC_CONF_L2_FORWARDING_NUM_ENTRIES; i++) {
        status = SJA1105_L2ForwardingTableRead(dev, i);
        if (status != SJA1105_OK) goto end;
    }

    // TODO: Add more tables

    /* Give the mutex and return */
end:
    SJA1105_UNLOCK;
    return status;
}
