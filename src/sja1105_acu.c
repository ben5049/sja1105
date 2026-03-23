/*
 * sja1105_acu.c
 *
 *  Created on: Jul 31, 2025
 *      Author: bens1
 */

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_regs.h"


static const uint32_t sja1105_acu_block_default[SJA1105_ACU_BLOCK_SIZE] = {
    (uint32_t) SJA1105_BLOCK_ID_ACU << 24,
    SJA1105_ACU_SIZE,
    0, /* CRC left at 0 means calculate at runtime */
    0, /* Reserved */
    0, /* Reserved */
    SJA1105_ACU_INITIAL_TS_CONFIG,
    SJA1105_ACU_INITIAL_CFG_PAD_JTAG,
    SJA1105_ACU_INITIAL_CFG_PAD_SPI,
    SJA1105_ACU_INITIAL_CFG_PAD_MISC,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_RX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_TX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_RX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_TX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_RX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_TX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_RX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_TX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_RX,
    SJA1105_ACU_INITIAL_CFG_PAD_MIIX_TX,
    0 /* CRC left at 0 means calculate at runtime */
};


sja1105_status_t SJA1105_ConfigureACU(sja1105_handle_t *dev, bool write) {

    sja1105_status_t status = SJA1105_OK;

    /* Add the table if it isn't there already */
    if (!dev->tables.acu_config_parameters.in_use) {
        status = SJA1105_AllocateFixedLengthTable(dev, sja1105_acu_block_default, SJA1105_ACU_BLOCK_SIZE);
        if (status != SJA1105_OK) return status;
    }

    /* Configure the ACU with each port's IO pad configuration */
    for (uint_fast8_t port_num = 0; port_num < SJA1105_NUM_PORTS; port_num++) {
        status = SJA1105_ConfigureACUPort(dev, port_num, write);
        if (status != SJA1105_OK) return status;
    }

    /* TODO: Configure MISC, SPI, and JTAG IO pads. */

    /* No need to configure the temperature sensor since that is done when the temperature sensor is read */

    return status;
}


sja1105_status_t SJA1105_ConfigureACUPort(sja1105_handle_t *dev, uint8_t port_num, bool write) {

    sja1105_status_t      status = SJA1105_OK;
    const sja1105_port_t *port   = &dev->config->ports[port_num];

    /* Skip port 4 in variants that don't have one */
    if (((dev->config->variant == VARIANT_SJA1105R) || (dev->config->variant == VARIANT_SJA1105S)) && (port_num == 4)) return status;

    /* Don't continue if no configuration is supplied. This isn't an error since a default register values will be used instead. */
    if (port->configured == false) {
        status = SJA1105_OK;
        return status;
    }

    /* Check port numbers match */
    if (port->port_num != port_num) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;

    /* Create and clear buffer */
    uint32_t reg_data[SJA1105_ACU_PAD_CFG_SIZE];
    reg_data[SJA1105_ACU_PAD_CFG_TX] = 0;
    reg_data[SJA1105_ACU_PAD_CFG_RX] = 0;

    /* Set slew rates */
    switch (port->interface) {
        case SJA1105_INTERFACE_MII:

            /* Low speed */
            reg_data[SJA1105_ACU_PAD_CFG_TX] |= SJA1105_OS_LOW;
            reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_CLK_OS_FAST;

            /* Pull down RX_ERR if unused */
            if (port->rx_error_unused) {
                reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_CTRL_IPUD_PD; /* Note the default 0x2 is ORed in later but 0x3 here overrules it */
            }
            break;

        case SJA1105_INTERFACE_RMII:

            /* 1V8 RMII not supported */
            if (port->voltage == SJA1105_IO_1V8) status = SJA1105_PARAMETER_ERROR;
            if (status != SJA1105_OK) return status;

            /* Low speed (but fast REF_CLK) */
            reg_data[SJA1105_ACU_PAD_CFG_TX] |= SJA1105_CLK_OS_FAST | SJA1105_CTRL_OS_LOW | SJA1105_D10_OS_LOW | SJA1105_D32_OS_LOW;
            reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_CLK_OS_FAST;

            /* Pull down RX_ERR if unused */
            if (port->rx_error_unused) {
                reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_CTRL_IPUD_PD; /* Note the default 0x2 is ORed in later but 0x3 here overrules it */
            }
            break;

        case SJA1105_INTERFACE_RGMII:

            switch (port->voltage) {

                case SJA1105_IO_2V5:
                case SJA1105_IO_3V3:

                    /* Medium speed */
                    reg_data[SJA1105_ACU_PAD_CFG_TX] |= SJA1105_OS_MEDIUM;
                    reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_CLK_OS_HIGH;
                    break;

                case SJA1105_IO_1V8:

                    /* Fast speed */
                    reg_data[SJA1105_ACU_PAD_CFG_TX] |= SJA1105_OS_HIGH;
                    reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_CLK_OS_HIGH;
                    break;

                default:
                    status = SJA1105_PARAMETER_ERROR;
                    break;
            }
            break;

        default:
            status = SJA1105_PARAMETER_ERROR;
            break;
    }
    if (status != SJA1105_OK) return status;

    /* Disable internal TX pull downs */
    reg_data[SJA1105_ACU_PAD_CFG_TX] |= SJA1105_IPUD_PI;

    /* Set TX CLK input hysteresis to non-Schmitt  */
    reg_data[SJA1105_ACU_PAD_CFG_TX] |= SJA1105_CLK_IH_NON_SCHMITT;

    /* Disable internal RX pull downs and set input hysteresis to non-Schmitt */
    reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_IPUD_PI;
    reg_data[SJA1105_ACU_PAD_CFG_RX] |= SJA1105_IH_NON_SCHMITT;

    /* Write the pad config */
    if (write) {
        status = SJA1105_WriteRegister(dev, SJA1105_ACU_REG_CFG_PAD_MIIX_TX(port_num), reg_data, SJA1105_ACU_PAD_CFG_SIZE);
        if (status != SJA1105_OK) return status;
    }

    /* Update the internal copy of the table */
    if (dev->tables.acu_config_parameters.in_use) {
        dev->tables.acu_config_parameters.data[SJA1105_ACU_TABLE_PAD_MIIX_TX_INDEX(port_num)] = reg_data[SJA1105_ACU_PAD_CFG_TX];
        dev->tables.acu_config_parameters.data[SJA1105_ACU_TABLE_PAD_MIIX_RX_INDEX(port_num)] = reg_data[SJA1105_ACU_PAD_CFG_RX];
        dev->tables.acu_config_parameters.data_crc_valid                                      = false;
    }

    /* Configure the tuneable delay lines */
    uint8_t tx_delay = ((port->rgmii_id_mode == SJA1105_RGMII_ID_TX_1NS) || (port->rgmii_id_mode == SJA1105_RGMII_ID_TX_RX_1NS)) ? SJA1105_ID_1NS : SJA1105_ID_NONE;
    uint8_t rx_delay = ((port->rgmii_id_mode == SJA1105_RGMII_ID_RX_1NS) || (port->rgmii_id_mode == SJA1105_RGMII_ID_TX_RX_1NS)) ? SJA1105_ID_1NS : SJA1105_ID_NONE;
    status           = SJA1105_ConfigureTDL(dev, port_num, tx_delay, rx_delay, write);
    if (status != SJA1105_OK) return status;

    return status;
}


/* Update the internal delay (ID) register (SJA1105_ACU_REG_CFG_PAD_MIIX_ID) if internal RGMII
 * CLK delays are needed. Many PHYs also implement this and it is only needed once per TX or RX
 * channel. Since the SJA1105's ID implementation uses phase (not time) delays and requires
 * managing frequency transitions, the PHY implementation is usually preferred.
 *
 * For RGMII MAC to MAC connections it is required
 */
sja1105_status_t SJA1105_ConfigureTDL(sja1105_handle_t *dev, uint8_t port_num, uint8_t tx_delay, uint8_t rx_delay, bool write) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data;

    /* Check the ID mode is valid */
    if (tx_delay > SJA1105_ID_NONE) status = SJA1105_PARAMETER_ERROR;
    if (rx_delay > SJA1105_ID_NONE) status = SJA1105_PARAMETER_ERROR;
    if ((tx_delay != SJA1105_ID_NONE) && ((tx_delay < SJA1105_ID_MIN) || (tx_delay > SJA1105_ID_MAX))) status = SJA1105_PARAMETER_ERROR;
    if ((rx_delay != SJA1105_ID_NONE) && ((rx_delay < SJA1105_ID_MIN) || (rx_delay > SJA1105_ID_MAX))) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;

    /* Reset ID config */
    reg_data = SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID;

    /* Set the TX delay */
    if (tx_delay != SJA1105_ID_NONE) {
        reg_data &= ~(SJA1105_TXC_PD | SJA1105_TXC_BYPASS); /* Disable power down and bypass */
        reg_data &= ~SJA1105_TXC_DELAY_MASK;
        reg_data |= (SJA1105_TXC_DELAY_MASK & (tx_delay << SJA1105_TXC_DELAY_SHIFT));
    }

    /* Set the RX delay */
    if (rx_delay != SJA1105_ID_NONE) {
        reg_data &= ~(SJA1105_RXC_PD | SJA1105_RXC_BYPASS); /* Disable power down and bypass */
        reg_data &= ~SJA1105_RXC_DELAY_MASK;
        reg_data |= (SJA1105_RXC_DELAY_MASK & (rx_delay << SJA1105_RXC_DELAY_SHIFT));
    }

    /* Update the internal copy of the table */
    if (dev->tables.acu_config_parameters.in_use) {
        dev->tables.acu_config_parameters.data[SJA1105_ACU_TABLE_PAD_MIIX_ID_INDEX(port_num)] = reg_data;
    }

    /* Write the config */
    if (write) {
        status                                           = SJA1105_WriteRegister(dev, SJA1105_ACU_REG_CFG_PAD_MIIX_ID(port_num), &reg_data, 1);
        dev->tables.acu_config_parameters.data_crc_valid = false;
        if (status != SJA1105_OK) return status;
    }

    return status;
}
