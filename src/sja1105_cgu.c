/*
 * sja1105_cgu.c
 *
 *  Created on: Jul 31, 2025
 *      Author: bens1
 */

#include "sja1105.h"
#include "internal/sja1105_conf.h"
#include "internal/sja1105_io.h"
#include "internal/sja1105_regs.h"


/* TODO: Fill in the non-reserved register defaults (this is low priority since all values are written in SJA1105_ConfigureCGU()) */
static const uint32_t sja1105_cgu_block_default[SJA1105_CGU_BLOCK_SIZE] = {
    (uint32_t) SJA1105_BLOCK_ID_CGU << 24,
    SJA1105_CGU_SIZE,
    0, /* CRC left at 0 means calculate at runtime */
    0, /* Reserved */
    0, /* Reserved */
    0, /* Reserved */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,          /* Reserved */
    0,          /* Reserved */
    0,          /* Reserved */
    0x0a000000, /* IDIV_4_C*/
    0x0a000000, /* IDIV_3_C*/
    0x0a000000, /* IDIV_2_C*/
    0x0a000000, /* IDIV_1_C*/
    0x0a000000, /* IDIV_0_C*/
    0x0a000083, /* PLL_1_C */
    0x0a040040, /* PLL_0_C */
    0,          /* Reserved */
    0,          /* CRC left at 0 means calculate at runtime */
};


sja1105_status_t SJA1105_ConfigureCGU(sja1105_handle_t *dev, bool write) {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         reg_data;

    /* Add the table if it isn't there already */
    if (!dev->tables.cgu_config_parameters.in_use) {
        status = SJA1105_AllocateFixedLengthTable(dev, sja1105_cgu_block_default, SJA1105_CGU_BLOCK_SIZE);
        if (status != SJA1105_OK) return status;
    }

    /* Setup PLL0 (f = 125MHz), the only thing able to be configured is P23EN (skew clocks) */
    reg_data = ((uint32_t) SJA1105_CGU_CLK_SRC_XO66M_0) << SJA1105_CGU_CLKSRC_SHIFT; /* Source must be XO66M_0 (read only) */
    if (dev->config->skew_clocks) reg_data |= SJA1105_CGU_P23EN;                     /* Enable 120 and 240 degree outputs for better EMC performance */

    /* Write the PLL0 config */
    if (write) {
        status = SJA1105_WriteRegister(dev, SJA1105_CGU_REG_PLL_0_C, &reg_data, 1);
        if (status != SJA1105_OK) return status;

        /* Delay to prevent false lock from previous config */
        SJA1105_DELAY_MS(1);

        /* Wait for PLL0 lock */
        status = SJA1105_PollFlag(dev, SJA1105_CGU_REG_PLL_0_S, SJA1105_CGU_PLL_LOCK, true);
        if (status != SJA1105_OK) return status;
    }


    /* Save the PLL0 configuration */
    if (dev->tables.cgu_config_parameters.in_use) dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_PLL_0_C_INDEX] = reg_data;

    /* Setup PLL1 (f = 50MHz, integer mode) */
    reg_data  = 0;
    reg_data |= ((uint32_t) SJA1105_CGU_CLK_SRC_XO66M_0) << SJA1105_CGU_CLKSRC_SHIFT; /* Source must be XO66M_0 (read only) */
    reg_data |= SJA1105_CGU_PD;                                                       /* Enable power down while configuring */
    reg_data &= ~SJA1105_CGU_BYPASS;                                                  /* Disable bypass */
    if (dev->config->skew_clocks) reg_data |= SJA1105_CGU_P23EN;                      /* Enable 120 and 240 degree outputs for better EMC performance */
    reg_data |= SJA1105_CGU_FBSEL;                                                    /* Enable PLL feedback */
    reg_data |= ((uint32_t) 0x1 << SJA1105_CGU_PSEL_SHIFT) & SJA1105_CGU_PSEL_MASK;   /* PSEL = 1 */
    reg_data |= ((uint32_t) 0x1 << SJA1105_CGU_MSEL_SHIFT) & SJA1105_CGU_MSEL_MASK;   /* MSEL = 1 */
    reg_data |= ((uint32_t) 0x0 << SJA1105_CGU_NSEL_SHIFT) & SJA1105_CGU_NSEL_MASK;   /* NSEL = 0 */
    if (write) {
        status = SJA1105_WriteRegister(dev, SJA1105_CGU_REG_PLL_1_C, &reg_data, 1);
        if (status != SJA1105_OK) return status;
    }

    /* Enable PLL1 */
    reg_data &= ~SJA1105_CGU_PD; /* Disable power down */
    if (write) {
        status = SJA1105_WriteRegister(dev, SJA1105_CGU_REG_PLL_1_C, &reg_data, 1);
        if (status != SJA1105_OK) return status;

        /* Wait for PLL1 lock */
        SJA1105_DELAY_MS(1); /* Delay to prevent false lock from previous config */
        status = SJA1105_PollFlag(dev, SJA1105_CGU_REG_PLL_1_S, SJA1105_CGU_PLL_LOCK, true);
        if (status != SJA1105_OK) return status;
    }

    /* Save the PLL1 configuration */
    if (dev->tables.cgu_config_parameters.in_use) dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_PLL_1_C_INDEX] = reg_data;

    /* Configure each port */
    for (uint_fast8_t port_num = 0; port_num < SJA1105_NUM_PORTS; port_num++) {
        status = SJA1105_ConfigureCGUPort(dev, port_num, write);
        if (status != SJA1105_OK) return status;
    }

    return status;
}

sja1105_status_t SJA1105_ConfigureCGUPort(sja1105_handle_t *dev, uint8_t port_num, bool write) {

    sja1105_status_t      status                            = SJA1105_OK;
    const sja1105_port_t *port                              = &dev->config->ports[port_num];
    uint32_t              clk_data[SJA1105_CGU_REG_CLK_NUM] = {0};
    uint32_t              idiv_data                         = (uint32_t) SJA1105_CGU_CLK_SRC_XO66M_0 << SJA1105_CGU_CLKSRC_SHIFT;
    sja1105_speed_t       speed                             = SJA1105_SPEED_INVALID;

    /* Skip port 4 in variants that don't have one (due to SGMII) */
    if (((dev->config->variant == VARIANT_SJA1105R) || (dev->config->variant == VARIANT_SJA1105S)) && (port_num == 4)) return status;

    /* Get the speed of the port */
    status = SJA1105_PortGetSpeed(dev, port_num, &speed);
    if (status != SJA1105_OK) return status;

    /* Skip clock setup for dynamic ports, they must be configured separately with SJA1105_PortSetSpeed()
     * Note: SJA1105_PortSetSpeed() does call this function, but it calls SJA1105_PortGetSpeed() first
     *       which updates the MAC config table and means speed != SJA1105_SPEED_DYNAMIC. */
    if (speed == SJA1105_SPEED_DYNAMIC) return status;

    switch (port->interface) {
        case SJA1105_INTERFACE_MII:

            /* MII MAC */
            if (port->mode == SJA1105_MODE_MAC) {

                /* Disable IDIVX */
                idiv_data |= SJA1105_CGU_PD;

                /* Set CLKSRC of MII_TX_CLK_X to TX_CLK_X */
                clk_data[SJA1105_CGU_MII_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_TX_CLK(port_num) << SJA1105_CGU_CLKSRC_SHIFT; /* Implicitly sets PD to 0 */
                clk_data[SJA1105_CGU_MII_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;                                                       /* Prevent clock glitches */

                /* Set CLKSRC of MII_RX_CLK_X to RX_CLK_X */
                clk_data[SJA1105_CGU_MII_RX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_RX_CLK(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                clk_data[SJA1105_CGU_MII_RX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                /* Disable all other clock sinks */
                clk_data[SJA1105_CGU_RMII_REF_CLK] = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_RGMII_TX_CLK] = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_EXT_TX_CLK]   = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_EXT_RX_CLK]   = SJA1105_CGU_PD;
            }

            /* MII PHY */
            else {
                switch (speed) {

                    /* 10M MII PHY */
                    case SJA1105_SPEED_10M: {

                        /* Enable IDIVX and divide by 10 */
                        idiv_data |= ((uint32_t) 9 << SJA1105_CGU_IDIV_SHIFT) & SJA1105_CGU_IDIV_MASK;

                        /* Set CLKSRC of MII_TX_CLK_X to IDIVX */
                        clk_data[SJA1105_CGU_MII_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_IDIV(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                        clk_data[SJA1105_CGU_MII_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                        /* Set CLKSRC of MII_RX_CLK_X to RX_CLK_X */
                        clk_data[SJA1105_CGU_MII_RX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_RX_CLK(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                        clk_data[SJA1105_CGU_MII_RX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                        /* Set CLKSRC of EXT_TX_CLK_X to IDIVX */
                        clk_data[SJA1105_CGU_EXT_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_IDIV(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                        clk_data[SJA1105_CGU_EXT_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                        /* Set CLKSRC of EXT_RX_CLK_X to IDIVX */
                        clk_data[SJA1105_CGU_EXT_RX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_IDIV(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                        clk_data[SJA1105_CGU_EXT_RX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                        /* Disable all other clock sinks */
                        clk_data[SJA1105_CGU_RMII_REF_CLK] = SJA1105_CGU_PD;
                        clk_data[SJA1105_CGU_RGMII_TX_CLK] = SJA1105_CGU_PD;

                        break;
                    }

                    /* 100M MII PHY */
                    case SJA1105_SPEED_100M: {
                        status = SJA1105_NOT_IMPLEMENTED_ERROR;
                        break;
                    }

                    default:
                        status = SJA1105_PARAMETER_ERROR;
                        break;
                }
            }
            break;

        case SJA1105_INTERFACE_RMII:

            /* RMII MAC */
            if (port->mode == SJA1105_MODE_MAC) {

                /* Disable IDIVX */
                idiv_data |= SJA1105_CGU_PD;

                /* Set CLKSRC of RMII_REF_CLK_X to TX_CLK_X */
                clk_data[SJA1105_CGU_RMII_REF_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_TX_CLK(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                clk_data[SJA1105_CGU_RMII_REF_CLK] |= SJA1105_CGU_AUTOBLOCK;

                /* Set CLKSRC of EXT_TX_CLK_X to PLL1 */
                clk_data[SJA1105_CGU_EXT_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_PLL1(dev->config->skew_clocks ? port_num : 0) << SJA1105_CGU_CLKSRC_SHIFT;
                clk_data[SJA1105_CGU_EXT_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                /* Disable all other clock sinks */
                clk_data[SJA1105_CGU_MII_TX_CLK]   = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_MII_RX_CLK]   = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_RGMII_TX_CLK] = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_EXT_RX_CLK]   = SJA1105_CGU_PD;
            }

            /* RMII PHY */
            else {

                /* Disable IDIVX */
                idiv_data |= SJA1105_CGU_PD;

                /* Set CLKSRC of RMII_REF_CLK_X to TX_CLK_X */
                clk_data[SJA1105_CGU_RMII_REF_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_TX_CLK(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                clk_data[SJA1105_CGU_RMII_REF_CLK] |= SJA1105_CGU_AUTOBLOCK;

                /* Set CLKSRC of EXT_TX_CLK_X to PLL1 */
                if (port->output_rmii_refclk) {
                    clk_data[SJA1105_CGU_EXT_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_PLL1(dev->config->skew_clocks ? port_num : 0) << SJA1105_CGU_CLKSRC_SHIFT;
                    clk_data[SJA1105_CGU_EXT_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;
                } else {
                    clk_data[SJA1105_CGU_EXT_TX_CLK] = SJA1105_CGU_PD;
                }

                /* Disable all other clock sinks */
                clk_data[SJA1105_CGU_MII_TX_CLK]   = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_MII_RX_CLK]   = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_RGMII_TX_CLK] = SJA1105_CGU_PD;
                clk_data[SJA1105_CGU_EXT_RX_CLK]   = SJA1105_CGU_PD;
            }
            break;

        case SJA1105_INTERFACE_RGMII:

            /* RGMII */
            switch (speed) {

                /* 10M RGMII MAC */
                case SJA1105_SPEED_10M: {

                    /* Enable IDIVX and divide by 10 */
                    idiv_data |= ((uint32_t) 9 << SJA1105_CGU_IDIV_SHIFT) & SJA1105_CGU_IDIV_MASK;

                    /* Set CLKSRC of RGMII_TXC_X to IDIVX */
                    clk_data[SJA1105_CGU_RGMII_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_IDIV(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                    clk_data[SJA1105_CGU_RGMII_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                    /* Disable all other clock sinks */
                    clk_data[SJA1105_CGU_MII_TX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_MII_RX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_RMII_REF_CLK] = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_EXT_TX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_EXT_RX_CLK]   = SJA1105_CGU_PD;

                    break;
                }

                /* 100M RGMII MAC */
                case SJA1105_SPEED_100M: {

                    /* Enable IDIVX and divide by 1 */
                    idiv_data |= ((uint32_t) 0 << SJA1105_CGU_IDIV_SHIFT) & SJA1105_CGU_IDIV_MASK;

                    /* Set CLKSRC of RGMII_TXC_X to IDIVX */
                    clk_data[SJA1105_CGU_RGMII_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_IDIV(port_num) << SJA1105_CGU_CLKSRC_SHIFT;
                    clk_data[SJA1105_CGU_RGMII_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                    /* Disable all other clock sinks */
                    clk_data[SJA1105_CGU_MII_TX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_MII_RX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_RMII_REF_CLK] = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_EXT_TX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_EXT_RX_CLK]   = SJA1105_CGU_PD;

                    break;
                }

                /* 1G RGMII MAC */
                case SJA1105_SPEED_1G: {

                    /* Disable IDIVX */
                    idiv_data |= SJA1105_CGU_PD;

                    /* Set CLKSRC of RGMII_TXC_X to PLL0 */
                    clk_data[SJA1105_CGU_RGMII_TX_CLK]  = (uint32_t) SJA1105_CGU_CLK_SRC_PLL0(dev->config->skew_clocks ? port_num : 0) << SJA1105_CGU_CLKSRC_SHIFT;
                    clk_data[SJA1105_CGU_RGMII_TX_CLK] |= SJA1105_CGU_AUTOBLOCK;

                    /* Disable all other clock sinks */
                    clk_data[SJA1105_CGU_MII_TX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_MII_RX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_RMII_REF_CLK] = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_EXT_TX_CLK]   = SJA1105_CGU_PD;
                    clk_data[SJA1105_CGU_EXT_RX_CLK]   = SJA1105_CGU_PD;

                    break;
                }

                default:
                    status = SJA1105_PARAMETER_ERROR;
                    break;
            }
            break;

        /* Nothing to do for SGMII since it's clocks are hardwired */
        case SJA1105_INTERFACE_SGMII:
            status = SJA1105_OK;
            break;

        default:
            status = SJA1105_PARAMETER_ERROR;
            break;
    }
    if (status != SJA1105_OK) return status;


    /* Apply the configuration to non-sgmii ports */
    if (port->interface != SJA1105_INTERFACE_SGMII) {

        /* Write the configuration */
        if (write) {

            /* Write to the IDIV register */
            status = SJA1105_WriteRegister(dev, SJA1105_CGU_REG_IDIV_C(port_num), &idiv_data, 1);
            if (status != SJA1105_OK) return status;

            /* Write to the CLK registers */
            status = SJA1105_WriteRegister(dev, SJA1105_CGU_REG_CLK_BASE(port_num), clk_data, SJA1105_CGU_REG_CLK_NUM);
            if (status != SJA1105_OK) return status;
        }

        /* Update the internal table */
        if (dev->tables.cgu_config_parameters.in_use) {
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_IDIV_X_C_INDEX(port_num)]            = idiv_data;
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_MIIX_MII_TX_CLK_C_INDEX(port_num)]   = clk_data[SJA1105_CGU_MII_TX_CLK];
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_MIIX_MII_RX_CLK_C_INDEX(port_num)]   = clk_data[SJA1105_CGU_MII_RX_CLK];
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_MIIX_RMII_REF_CLK_C_INDEX(port_num)] = clk_data[SJA1105_CGU_RMII_REF_CLK];
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_MIIX_RGMII_TX_CLK_CINDEX(port_num)]  = clk_data[SJA1105_CGU_RGMII_TX_CLK];
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_MIIX_EXT_TX_CLK_C_INDEX(port_num)]   = clk_data[SJA1105_CGU_EXT_TX_CLK];
            dev->tables.cgu_config_parameters.data[SJA1105_CGU_TABLE_MIIX_EXT_RX_CLK_C_INDEX(port_num)]   = clk_data[SJA1105_CGU_EXT_RX_CLK];
        }
    }

    return status;
}
