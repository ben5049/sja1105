/*
 * sja1105_regs.h
 *
 *  Created on: Jul 30, 2025
 *      Author: bens1
 */

#ifndef SJA1105_INC_SJA1105_REGS_H_
#define SJA1105_INC_SJA1105_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "sja1105.h"


/* ---------------------------------------------------------------------------- */
/* Ethernet Switch Core */
/* ---------------------------------------------------------------------------- */

enum SJA1105_GeneralReg_Enum {
    SJA1105_REG_DEVICE_ID              = 0x00000000,
    SJA1105_REG_STATIC_CONF_FLAGS      = 0x00000001,
    SJA1105_REG_VL_PART_STATUS         = 0x00000002,
    SJA1105_REG_GENERAL_STATUS_1       = 0x00000003,
    SJA1105_REG_GENERAL_STATUS_2       = 0x00000004,
    SJA1105_REG_GENERAL_STATUS_3       = 0x00000005,
    SJA1105_REG_GENERAL_STATUS_4       = 0x00000006,
    SJA1105_REG_GENERAL_STATUS_5       = 0x00000007,
    SJA1105_REG_GENERAL_STATUS_6       = 0x00000008,
    SJA1105_REG_GENERAL_STATUS_7       = 0x00000009,
    SJA1105_REG_GENERAL_STATUS_8       = 0x0000000a,
    SJA1105_REG_GENERAL_STATUS_9       = 0x0000000b,
    SJA1105_REG_GENERAL_STATUS_10      = 0x0000000c, /* RAM Parity error register (lower) */
    SJA1105_REG_GENERAL_STATUS_11      = 0x0000000d, /* RAM Parity error register (upper) */
    SJA1105_REG_HIGH_LEVEL_STATS_PORT0 = 0x00000400,
    SJA1105_REG_HIGH_LEVEL_STATS_PORT1 = 0x00000410,
    SJA1105_REG_HIGH_LEVEL_STATS_PORT2 = 0x00000420,
    SJA1105_REG_HIGH_LEVEL_STATS_PORT3 = 0x00000430,
    SJA1105_REG_HIGH_LEVEL_STATS_PORT4 = 0x00000440,
};

enum SJA1105_DeviceID_Enum {
    SJA1105ET_DEVICE_ID = 0x9f00030e,
    SJA1105PR_DEVICE_ID = 0xaf00030e,
    SJA1105QS_DEVICE_ID = 0xae00030e,
};

#define SJA1105_CONFIGS_SHIFT                      (31)
#define SJA1105_CONFIGS_MASK                       (0x1 << SJA1105_CONFIGS_SHIFT)
#define SJA1105_CRCCHKL_SHIFT                      (30)                           /* Local CRC check */
#define SJA1105_CRCCHKL_MASK                       (0x1 << SJA1105_CRCCHKL_SHIFT) /* Local CRC check */
#define SJA1105_IDS_SHIFT                          (29)
#define SJA1105_IDS_MASK                           (0x1 << SJA1105_IDS_SHIFT)
#define SJA1105_CRCCHKG_SHIFT                      (28)                           /* Global CRC check */
#define SJA1105_CRCCHKG_MASK                       (0x1 << SJA1105_CRCCHKG_SHIFT) /* Global CRC check */

#define SJA1105_REGULAR_CHECK_ADDR                 (SJA1105_REG_VL_PART_STATUS)
#define SJA1105_REGULAR_CHECK_SIZE                 (SJA1105_REG_GENERAL_STATUS_11 - SJA1105_REGULAR_CHECK_ADDR + 1)

#define SJA1105_L2BUSYS_SHIFT                      (0)
#define SJA1105_L2BUSYS_MASK                       (0x1 << SJA1105_L2BUSYS_SHIFT)

#define SJA1105_VLPARTS                            (1 << 0)
#define SJA1105_VLROUTES                           (1 << 1)
#define SJA1105_VLPARIND_SHIFT                     (8)
#define SJA1105_VLPARIND_MASK                      (0xff << SJA1105_VLPARIND_SHIFT)
#define SJA1105_VLIND_SHIFT                        (16)
#define SJA1105_VLIND_MASK                         (0xffff << SJA1105_VLIND_SHIFT)

#define SJA1105_PARTS                              (1 << 0)
#define SJA1105_FWDS                               (1 << 1)
#define SJA1105_FWDS_PARTS_PORT_SHIFT              (8)
#define SJA1105_FWDS_PARTS_PORT_MASK               (0xff << SJA1105_FWDS_PARTS_PORT_SHIFT)

#define SJA1105_HIGH_LEVEL_STATS_SIZE              (0x10)
#define SJA1105_HIGH_LEVEL_STATS_PORT_OFFSET(port) ((port) * SJA1105_HIGH_LEVEL_STATS_SIZE)

#define SJA1105_HIGH_LEVEL_STATS_N_TXBYTE_L        (0x0)
#define SJA1105_HIGH_LEVEL_STATS_N_TXBYTE_H        (0x1)
#define SJA1105_HIGH_LEVEL_STATS_N_RXBYTE_L        (0x4)
#define SJA1105_HIGH_LEVEL_STATS_N_RXBYTE_H        (0x5)
#define SJA1105_HIGH_LEVEL_STATS_N_POLERR          (0x8)

/* ---------------------------------------------------------------------------- */
/* Auxiliary Configuration Unit */
/* ---------------------------------------------------------------------------- */

#define SJA1105_ACU_SIZE                              (21)
#define SJA1105_ACU_BLOCK_SIZE                        (SJA1105_STATIC_CONF_BLOCK_OVERHEAD + SJA1105_ACU_SIZE)

#define SJA1105_ACU_INITIAL_TS_CONFIG                 (0x00000065)
#define SJA1105_ACU_INITIAL_CFG_PAD_JTAG              (0x02000000)
#define SJA1105_ACU_INITIAL_CFG_PAD_SPI               (0x12040407)
#define SJA1105_ACU_INITIAL_CFG_PAD_MISC              (0x00120612)
#define SJA1105_ACU_INITIAL_CFG_PAD_MIIX_ID           (0x00002323)
#define SJA1105_ACU_INITIAL_CFG_PAD_MIIX_RX           (0x02020212)
#define SJA1105_ACU_INITIAL_CFG_PAD_MIIX_TX           (0x12121212)

#define SJA1105_ACU_TABLE_PAD_MIIX_RX_INDEX(port_num) (11 + (2 * (SJA1105_NUM_PORTS - 1 - (port_num))))
#define SJA1105_ACU_TABLE_PAD_MIIX_TX_INDEX(port_num) (12 + (2 * (SJA1105_NUM_PORTS - 1 - (port_num))))

enum SJA1105_ACUReg_Enum {
    SJA1105_ACU_REG_CFG_PAD_MIIX_BASE = 0x100800,
    SJA1105_ACU_REG_CFG_PAD_MISC      = 0x100840,
    SJA1105_ACU_REG_CFG_PAD_SPI       = 0x100880,
    SJA1105_ACU_REG_CFG_PAD_JTAG      = 0x100881,
    SJA1105_ACU_REG_PORT_STATUS_MII0  = 0x100900,
    SJA1105_ACU_REG_PORT_STATUS_MII1  = 0x100901,
    SJA1105_ACU_REG_PORT_STATUS_MII2  = 0x100902,
    SJA1105_ACU_REG_PORT_STATUS_MII3  = 0x100903,
    SJA1105_ACU_REG_PORT_STATUS_MII4  = 0x100904,
};

#define SJA1105_ACU_REG_CFG_PAD_MIIX_TX(port_num)  (SJA1105_ACU_REG_CFG_PAD_MIIX_BASE + 2 * (port_num))
#define SJA1105_ACU_REG_CFG_PAD_MIIX_RX(port_num)  (SJA1105_ACU_REG_CFG_PAD_MIIX_BASE + 2 * (port_num) + 1)
#define SJA1105_ACU_REG_CFG_PAD_MIIX_ID(port_num)  (SJA1105_ACU_REG_CFG_PAD_MIIX_BASE + 16 + (port_num))

#define SJA1105_ACU_REG_PORT_STATUS_MIIX(port_num) (SJA1105_ACU_REG_PORT_STATUS_MII0 + (port_num))

#define SJA1105_ACU_REG_TS_CONFIG                  (0x100a00)
#define SJA1105_ACU_REG_TS_STATUS                  (0x100a01)
#define SJA1105_ACU_REG_PROD_CFG                   (0x100bc0)
#define SJA1105_ACU_REG_PROD_ID                    (0x100bc3)
#define SJA1105_ACU_REG_ACCESS_DISABLE             (0x100bfd)

#define SJA1105_ACU_PAD_CFG_TX                     (0)
#define SJA1105_ACU_PAD_CFG_RX                     (1)
#define SJA1105_ACU_PAD_CFG_SIZE                   (2)

#define SJA1105_CLK_OS_LOW                         (0 << 3)
#define SJA1105_CLK_OS_MEDIUM                      (1 << 3)
#define SJA1105_CLK_OS_FAST                        (2 << 3)
#define SJA1105_CLK_OS_HIGH                        (3 << 3)
#define SJA1105_CTRL_OS_LOW                        (0 << 11)
#define SJA1105_CTRL_OS_MEDIUM                     (1 << 11)
#define SJA1105_CTRL_OS_FAST                       (2 << 11)
#define SJA1105_CTRL_OS_HIGH                       (3 << 11)
#define SJA1105_D10_OS_LOW                         (0 << 19)
#define SJA1105_D10_OS_MEDIUM                      (1 << 19)
#define SJA1105_D10_OS_FAST                        (2 << 19)
#define SJA1105_D10_OS_HIGH                        (3 << 19)
#define SJA1105_D32_OS_LOW                         (0 << 27)
#define SJA1105_D32_OS_MEDIUM                      (1 << 27)
#define SJA1105_D32_OS_FAST                        (2 << 27)
#define SJA1105_D32_OS_HIGH                        (3 << 27)

#define SJA1105_OS_LOW                             (SJA1105_CLK_OS_LOW | SJA1105_CTRL_OS_LOW | SJA1105_D10_OS_LOW | SJA1105_D32_OS_LOW)
#define SJA1105_OS_MEDIUM                          (SJA1105_CLK_OS_MEDIUM | SJA1105_CTRL_OS_MEDIUM | SJA1105_D10_OS_MEDIUM | SJA1105_D32_OS_MEDIUM)
#define SJA1105_OS_FAST                            (SJA1105_CLK_OS_FAST | SJA1105_CTRL_OS_FAST | SJA1105_D10_OS_FAST | SJA1105_D32_OS_FAST)
#define SJA1105_OS_HIGH                            (SJA1105_CLK_OS_HIGH | SJA1105_CTRL_OS_HIGH | SJA1105_D10_OS_HIGH | SJA1105_D32_OS_HIGH)

#define SJA1105_CLK_IPUD_PU                        (0 << 0)
#define SJA1105_CLK_IPUD_R                         (1 << 0)
#define SJA1105_CLK_IPUD_PI                        (2 << 0)
#define SJA1105_CLK_IPUD_PD                        (3 << 0)
#define SJA1105_CTRL_IPUD_PU                       (0 << 8)
#define SJA1105_CTRL_IPUD_R                        (1 << 8)
#define SJA1105_CTRL_IPUD_PI                       (2 << 8)
#define SJA1105_CTRL_IPUD_PD                       (3 << 8)
#define SJA1105_D10_IPUD_PU                        (0 << 16)
#define SJA1105_D10_IPUD_R                         (1 << 16)
#define SJA1105_D10_IPUD_PI                        (2 << 16)
#define SJA1105_D10_IPUD_PD                        (3 << 16)
#define SJA1105_D32_IPUD_PU                        (0 << 24)
#define SJA1105_D32_IPUD_R                         (1 << 24)
#define SJA1105_D32_IPUD_PI                        (2 << 24)
#define SJA1105_D32_IPUD_PD                        (3 << 24)

#define SJA1105_IPUD_PU                            (SJA1105_CLK_IPUD_PU | SJA1105_CTRL_IPUD_PU | SJA1105_D10_IPUD_PU | SJA1105_D32_IPUD_PU)
#define SJA1105_IPUD_R                             (SJA1105_CLK_IPUD_R | SJA1105_CTRL_IPUD_R | SJA1105_D10_IPUD_R | SJA1105_D32_IPUD_R)
#define SJA1105_IPUD_PI                            (SJA1105_CLK_IPUD_PI | SJA1105_CTRL_IPUD_PI | SJA1105_D10_IPUD_PI | SJA1105_D32_IPUD_PI)
#define SJA1105_IPUD_PD                            (SJA1105_CLK_IPUD_PD | SJA1105_CTRL_IPUD_PD | SJA1105_D10_IPUD_PD | SJA1105_D32_IPUD_PD)

#define SJA1105_CLK_IH_NON_SCHMITT                 (0 << 2)
#define SJA1105_CLK_IH_SCHMITT                     (1 << 2)
#define SJA1105_CTRL_IH_NON_SCHMITT                (0 << 10)
#define SJA1105_CTRL_IH_SCHMITT                    (1 << 10)
#define SJA1105_D10_IH_NON_SCHMITT                 (0 << 18)
#define SJA1105_D10_IH_SCHMITT                     (1 << 18)
#define SJA1105_D32_IH_NON_SCHMITT                 (0 << 26)
#define SJA1105_D32_IH_SCHMITT                     (1 << 26)

#define SJA1105_IH_SCHMITT                         (SJA1105_CLK_IH_SCHMITT | SJA1105_CTRL_IH_SCHMITT | SJA1105_D10_IH_SCHMITT | SJA1105_D32_IH_SCHMITT)
#define SJA1105_IH_NON_SCHMITT                     (SJA1105_CLK_IH_NON_SCHMITT | SJA1105_CTRL_IH_NON_SCHMITT | SJA1105_D10_IH_NON_SCHMITT | SJA1105_D32_IH_NON_SCHMITT)

#define SJA1105_PART_NR_OFFSET                     (4)
#define SJA1105_PART_NR_MASK                       (0xffff << SJA1105_PART_NR_OFFSET)

enum SJA1105_PartNR_Enum {
    PART_NR_SJA1105ET = 0x9a82,
    PART_NR_SJA1105P  = 0x9a84,
    PART_NR_SJA1105Q  = 0x9a85,
    PART_NR_SJA1105R  = 0x9a86,
    PART_NR_SJA1105S  = 0x9a87
};

#define SJA1105_TS_PD             (1 << 6)
#define SJA1105_TS_THRESHOLD_MASK (0x3f)
#define SJA1105_TS_EXCEEDED       (1)

#define SJA1105_TS_LUT_SIZE       (40)
static const int16_t SJA1105_TS_LUT[SJA1105_TS_LUT_SIZE] = {
    INT16_MIN, -457, -417, -375, -330, -284, -235, -183,
    -114, -61, -21, 21, 65, 110, 157, 206,
    256, 309, 364, 420, 461, 502, 545, 588,
    633, 679, 726, 774, 824, 875, 928, 982,
    1025, 1069, 1114, 1160, 1207, 1255, 1305, 1355};

/* ---------------------------------------------------------------------------- */
/* Clock Generation Unit */
/* ---------------------------------------------------------------------------- */

#define SJA1105_CGU_SIZE                                      (44)
#define SJA1105_CGU_BLOCK_SIZE                                (SJA1105_STATIC_CONF_BLOCK_OVERHEAD + SJA1105_CGU_SIZE)

#define SJA1105_CGU_TABLE_PLL_0_C_INDEX                       (42)
#define SJA1105_CGU_TABLE_PLL_1_C_INDEX                       (41)
#define SJA1105_CGU_TABLE_IDIV_X_C_INDEX(port_num)            (40 - (port_num))
#define SJA1105_CGU_TABLE_MIIX_MII_TX_CLK_C_INDEX(port_num)   (32 - SJA1105_CGU_REG_CLK_NUM * (port_num))
#define SJA1105_CGU_TABLE_MIIX_MII_RX_CLK_C_INDEX(port_num)   (31 - SJA1105_CGU_REG_CLK_NUM * (port_num))
#define SJA1105_CGU_TABLE_MIIX_RMII_REF_CLK_C_INDEX(port_num) (30 - SJA1105_CGU_REG_CLK_NUM * (port_num))
#define SJA1105_CGU_TABLE_MIIX_RGMII_TX_CLK_CINDEX(port_num)  (29 - SJA1105_CGU_REG_CLK_NUM * (port_num))
#define SJA1105_CGU_TABLE_MIIX_EXT_TX_CLK_C_INDEX(port_num)   (28 - SJA1105_CGU_REG_CLK_NUM * (port_num))
#define SJA1105_CGU_TABLE_MIIX_EXT_RX_CLK_C_INDEX(port_num)   (27 - SJA1105_CGU_REG_CLK_NUM * (port_num))

enum SJA1105_CGUReg_Enum {
    SJA1105_CGU_REG_RFRQ           = 0x100006,
    SJA1105_CGU_REG_XO66M_0_C      = 0x100006, /* C = Control */
    SJA1105_CGU_REG_PLL_0_S        = 0x100007, /* S = Status */
    SJA1105_CGU_REG_PLL_0_C        = 0x100008,
    SJA1105_CGU_REG_PLL_1_S        = 0x100009,
    SJA1105_CGU_REG_PLL_1_C        = 0x10000a,
    SJA1105_CGU_REG_IDIV_0_C       = 0x10000b,
    SJA1105_CGU_REG_IDIV_1_C       = 0x10000c,
    SJA1105_CGU_REG_IDIV_2_C       = 0x10000d,
    SJA1105_CGU_REG_IDIV_3_C       = 0x10000e,
    SJA1105_CGU_REG_IDIV_4_C       = 0x10000f,
    SJA1105_CGU_REG_MII_TX_CLK_0   = 0x100013,
    SJA1105_CGU_REG_MII_RX_CLK_0   = 0x100014,
    SJA1105_CGU_REG_RMII_REF_CLK_0 = 0x100015,
    SJA1105_CGU_REG_RGMII_TX_CLK_0 = 0x100016,
    SJA1105_CGU_REG_EXT_TX_CLK_0   = 0x100017,
    SJA1105_CGU_REG_EXT_RX_CLK_0   = 0x100018,
};

#define SJA1105_CGU_PLL_LOCK                 (1)

#define SJA1105_CGU_REG_IDIV_C(port_num)     (SJA1105_CGU_REG_IDIV_0_C + (port_num))

#define SJA1105_CGU_REG_CLK_NUM              (6)
#define SJA1105_CGU_REG_CLK_BASE(port_num)   (SJA1105_CGU_REG_MII_TX_CLK_0 + (SJA1105_CGU_REG_CLK_NUM * (port_num)))
#define SJA1105_CGU_MII_TX_CLK               (0)
#define SJA1105_CGU_MII_RX_CLK               (1)
#define SJA1105_CGU_RMII_REF_CLK             (2)
#define SJA1105_CGU_RGMII_TX_CLK             (3)
#define SJA1105_CGU_EXT_TX_CLK               (4)
#define SJA1105_CGU_EXT_RX_CLK               (5)

#define SJA1105_CGU_PD                       (1 << 0)
#define SJA1105_CGU_BYPASS                   (1 << 1)
#define SJA1105_CGU_P23EN                    (1 << 2)
#define SJA1105_CGU_FBSEL                    (1 << 6)
#define SJA1105_CGU_DIRECT                   (1 << 7)
#define SJA1105_CGU_PSEL_SHIFT               (8)
#define SJA1105_CGU_PSEL_MASK                (0x3 << SJA1105_CGU_PSEL_SHIFT)
#define SJA1105_CGU_AUTOBLOCK                (1 << 11)
#define SJA1105_CGU_NSEL_SHIFT               (12)
#define SJA1105_CGU_NSEL_MASK                (0x3 << SJA1105_CGU_NSEL_SHIFT)
#define SJA1105_CGU_MSEL_SHIFT               (16)
#define SJA1105_CGU_MSEL_MASK                (0xff << SJA1105_CGU_MSEL_SHIFT)
#define SJA1105_CGU_CLKSRC_SHIFT             (24)
#define SJA1105_CGU_CLKSRC_MASK              (0x1f << SJA1105_CGU_CLKSRC_SHIFT)

#define SJA1105_CGU_IDIV_SHIFT               (2)
#define SJA1105_CGU_IDIV_MASK                (0xff << SJA1105_CGU_IDIV_SHIFT)

#define SJA1105_CGU_CLK_SRC_XO66M_0          (0xa)
#define SJA1105_CGU_CLK_SRC_PLL0(port_num)   (0xb + ((port_num) % 3)) /* Use different phase for each port to improve EMC */
#define SJA1105_CGU_CLK_SRC_PLL1(port_num)   (0xe + ((port_num) % 3)) /* Use different phase for each port to improve EMC */
#define SJA1105_CGU_CLK_SRC_IDIV(port_num)   (0x11 + (port_num))
#define SJA1105_CGU_CLK_SRC_TX_CLK(port_num) (2 * (port_num))
#define SJA1105_CGU_CLK_SRC_RX_CLK(port_num) ((2 * (port_num)) + 1)

/* ---------------------------------------------------------------------------- */
/* Reset Generation Unit */
/* ---------------------------------------------------------------------------- */

#define SJA1105_RGU_REG_RESET_CTRL   0x100440
#define SJA1105_RGU_REG_UNIT_DISABLE 0x1007fd

#define SJA1105_RGU_SWITCH_RST       (1 << 8)
#define SJA1105_RGU_CFG_RST          (1 << 7)
#define SJA1105_RGU_CAR_RST          (1 << 5)
#define SJA1105_RGU_OTP_RST          (1 << 4)
#define SJA1105_RGU_WARM_RST         (1 << 3)
#define SJA1105_RGU_COLD_RST         (1 << 2)
#define SJA1105_RGU_POR_RST          (1 << 1)

/* ---------------------------------------------------------------------------- */
/* Static Configuration */
/* ---------------------------------------------------------------------------- */

#define SJA1105_STATIC_CONF_ADDR               (0x20000)

#define SJA1105_STATIC_CONF_BLOCK_ID           (1)
#define SJA1105_STATIC_CONF_BLOCK_SIZE         (1)
#define SJA1105_STATIC_CONF_BLOCK_HEADER       (SJA1105_STATIC_CONF_BLOCK_ID + SJA1105_STATIC_CONF_BLOCK_SIZE)
#define SJA1105_STATIC_CONF_BLOCK_HEADER_CRC   (1)
#define SJA1105_STATIC_CONF_BLOCK_DATA_CRC     (1)
#define SJA1105_STATIC_CONF_BLOCK_OVERHEAD     (SJA1105_STATIC_CONF_BLOCK_HEADER + SJA1105_STATIC_CONF_BLOCK_HEADER_CRC + SJA1105_STATIC_CONF_BLOCK_DATA_CRC) /* Number of non data words in a block */
#define SJA1105_STATIC_CONF_BLOCK_FIRST_OFFSET (1)
#define SJA1105_STATIC_CONF_BLOCK_LAST_SIZE    (3)                                                                                                            /* Last block contains two empty words and the global CRC */
#define SJA1105_STATIC_CONF_MIN_NUM_BLOCKS     (6)                                                                                                            /* Number of required config tables */
#define SJA1105_STATIC_CONF_MIN_SIZE           (SJA1105_STATIC_CONF_BLOCK_FIRST_OFFSET + SJA1105_STATIC_CONF_BLOCK_LAST_SIZE + (SJA1105_STATIC_CONF_MIN_NUM_BLOCKS * (SJA1105_STATIC_CONF_BLOCK_OVERHEAD)) + (SJA1105_STATIC_CONF_MAC_CONF_ENTRY_SIZE * SJA1105_NUM_PORTS) + SJA1105_STATIC_CONF_GENERAL_PARAMS_SIZE + SJA1105_STATIC_CONF_XMII_MODE_SIZE + SJA1105_STATIC_CONF_L2_FORWARDING_PARAMS_SIZE + SJA1105_STATIC_CONF_L2_FORWARDING_SIZE + SJA1105_STATIC_CONF_L2_POLICING_ENTRY_SIZE)

#define SJA1105_STATIC_CONF_BLOCK_ID_OFFSET    (0)
#define SJA1105_STATIC_CONF_BLOCK_ID_SHIFT     (24)
#define SJA1105_STATIC_CONF_BLOCK_ID_MASK      (0xff << SJA1105_STATIC_CONF_BLOCK_ID_SHIFT)
#define SJA1105_STATIC_CONF_BLOCK_SIZE_OFFSET  (1)
#define SJA1105_STATIC_CONF_HEADER_CRC_OFFSET  (2)
#define SJA1105_STATIC_CONF_DATA_OFFSET        (3)
#define SJA1105_STATIC_CONF_BLOCK_SIZE_SHIFT   (0)
#define SJA1105_STATIC_CONF_BLOCK_SIZE_MASK    (0xffffff)

typedef enum {
    SJA1105_BLOCK_ID_SCHEDULE                     = 0x00,
    SJA1105_BLOCK_ID_SCHEDULE_ENTRY_POINTS        = 0x01,
    SJA1105_BLOCK_ID_VL_LOOKUP                    = 0x02,
    SJA1105_BLOCK_ID_VL_POLICING                  = 0x03,
    SJA1105_BLOCK_ID_VL_FORWARDING                = 0x04,
    SJA1105_BLOCK_ID_L2_ADDR_LOOKUP               = 0x05,
    SJA1105_BLOCK_ID_L2_POLICING                  = 0x06,
    SJA1105_BLOCK_ID_VLAN_LOOKUP                  = 0x07,
    SJA1105_BLOCK_ID_L2_FORWARDING                = 0x08,
    SJA1105_BLOCK_ID_MAC_CONF                     = 0x09,
    SJA1105_BLOCK_ID_SCHEDULE_PARAMS              = 0x0a,
    SJA1105_BLOCK_ID_SCHEDULE_ENTRY_POINTS_PARAMS = 0x0b,
    SJA1105_BLOCK_ID_VL_FORWARDING_PARAMS         = 0x0c,
    SJA1105_BLOCK_ID_L2_LOOKUP_PARAMS             = 0x0d,
    SJA1105_BLOCK_ID_L2_FORWARDING_PARAMS         = 0x0e,
    SJA1105_BLOCK_ID_CLK_SYNC_PARAMS              = 0x0f,
    SJA1105_BLOCK_ID_AVB_PARAMS                   = 0x10,
    SJA1105_BLOCK_ID_GENERAL_PARAMS               = 0x11,
    SJA1105_BLOCK_ID_RETAGGING                    = 0x12,
    SJA1105_BLOCK_ID_CBS                          = 0x13,
    SJA1105_BLOCK_ID_XMII_MODE                    = 0x4e,
    SJA1105_BLOCK_ID_CGU                          = 0x80,
    SJA1105_BLOCK_ID_RGU                          = 0x81,
    SJA1105_BLOCK_ID_ACU                          = 0x82,
    SJA1105_BLOCK_ID_SGMII_CONF                   = 0xc8,
} sja1105_block_id_t;

#define SJA1105_STATIC_CONF_MAC_CONF_ENTRY_SIZE                 (8)
#define SJA1105_STATIC_CONF_MAC_CONF_BASE(port_num)             ((port_num) * SJA1105_STATIC_CONF_MAC_CONF_ENTRY_SIZE)
#define SJA1105_STATIC_CONF_MAC_CONF_WORD(port_num, word)       (SJA1105_STATIC_CONF_MAC_CONF_BASE(port_num) + (word))

#define SJA1105_STATIC_CONF_MAC_CONF_INGRESS_OFFSET             (0)  /* bit 31 therefore in the 1st word */
#define SJA1105_STATIC_CONF_MAC_CONF_INGRESS_SHIFT              (31) /* shifted up by 31 */
#define SJA1105_STATIC_CONF_MAC_CONF_INGRESS_MASK               (0x1 << SJA1105_STATIC_CONF_MAC_CONF_INGRESS_SHIFT)

#define SJA1105_STATIC_CONF_MAC_CONF_EGRESS_OFFSET              (1) /* bit 32 therefore in the 2nd word */
#define SJA1105_STATIC_CONF_MAC_CONF_EGRESS_SHIFT               (0) /* shifted up by 0 */
#define SJA1105_STATIC_CONF_MAC_CONF_EGRESS_MASK                (0x1 << SJA1105_STATIC_CONF_MAC_CONF_EGRESS_SHIFT)

#define SJA1105_STATIC_CONF_MAC_CONF_DYN_LEARN_OFFSET           (1) /* bit 33 therefore in the 2nd word */
#define SJA1105_STATIC_CONF_MAC_CONF_DYN_LEARN_SHIFT            (1) /* shifted up by 1 */
#define SJA1105_STATIC_CONF_MAC_CONF_DYN_LEARN_MASK             (0x1 << SJA1105_STATIC_CONF_MAC_CONF_DYN_LEARN_SHIFT)

#define SJA1105_STATIC_CONF_MAC_CONF_SPEED_OFFSET               (3) /* [98:97] therefore in the 4th word */
#define SJA1105_STATIC_CONF_MAC_CONF_SPEED_SHIFT                (1) /* shifted up by 1 */
#define SJA1105_STATIC_CONF_MAC_CONF_SPEED_MASK                 (0x3 << SJA1105_STATIC_CONF_MAC_CONF_SPEED_SHIFT)

#define SJA1105_STATIC_CONF_GENERAL_PARAMS_SIZE                 (11)

#define SJA1105_STATIC_CONF_L2_FORWARDING_ENTRY_SIZE            (2)
#define SJA1105_STATIC_CONF_L2_FORWARDING_NUM_ENTRIES           (13)
#define SJA1105_STATIC_CONF_L2_FORWARDING_SIZE                  (SJA1105_STATIC_CONF_L2_FORWARDING_ENTRY_SIZE * SJA1105_STATIC_CONF_L2_FORWARDING_NUM_ENTRIES)

#define SJA1105_STATIC_CONF_L2_FORWARDING_PARAMS_SIZE           (3)

#define SJA1105_STATIC_CONF_L2_POLICING_ENTRY_SIZE              (2)

#define SJA1105_MAC_FLT_START_OFFSET_W                          (4) /* Starts at bit 152 therefore in the 5th word */
#define SJA1105_MAC_FLT_START_OFFSET_B                          (3) /* Starts at bit 152 therefore offset 3 bytes from the nearest multiple of 32 bits (128 + 3 * 8 = 152) */

#define SJA1105_INCL_SRCPT1                                     (1 << 23)
#define SJA1105_INCL_SRCPT0                                     (1 << 22)
#define SJA1105_SEND_META1                                      (1 << 21)
#define SJA1105_SEND_META0                                      (1 << 20)

#define SJA1105_STATIC_CONF_GENERAL_PARAMS_HOST_PORT_OFFSET     (4)  /* [144:142] therefore in the 5th word */
#define SJA1105_STATIC_CONF_GENERAL_PARAMS_HOST_PORT_SHIFT      (14) /* shifted up by 14 */
#define SJA1105_STATIC_CONF_GENERAL_PARAMS_HOST_PORT_MASK       (0x7 << SJA1105_STATIC_CONF_GENERAL_PARAMS_HOST_PORT_SHIFT)

#define SJA1105_STATIC_CONF_GENERAL_PARAMS_CASC_PORT_OFFSET     (4)  /* [147:145] therefore in the 5th word */
#define SJA1105_STATIC_CONF_GENERAL_PARAMS_CASC_PORT_SHIFT      (17) /* shifted up by 17 */
#define SJA1105_STATIC_CONF_GENERAL_PARAMS_CASC_PORT_MASK       (0x7 << SJA1105_STATIC_CONF_GENERAL_PARAMS_CASC_PORT_SHIFT)

#define SJA1105_STATIC_CONF_GENERAL_PARAMS_SWITCHID_OFFSET      (10) /* [349:347] therefore in the 11th word */
#define SJA1105_STATIC_CONF_GENERAL_PARAMS_SWITCHID_SHIFT       (24) /* shifted up by 24 */
#define SJA1105_STATIC_CONF_GENERAL_PARAMS_SWITCHID_MASK        (0x7 << SJA1105_STATIC_CONF_GENERAL_PARAMS_SWITCHID_SHIFT)

#define SJA1105_STATIC_CONF_XMII_MODE_SIZE                      (1)

#define SJA1105_STATIC_CONF_XMII_MODE_PHY_MAC_SHIFT(port_num)   (19 + ((port_num) * 3))
#define SJA1105_STATIC_CONF_XMII_MODE_PHY_MAC_MASK(port_num)    (1 << SJA1105_STATIC_CONF_XMII_MODE_PHY_MAC_SHIFT(port_num))

#define SJA1105_STATIC_CONF_XMII_MODE_INTERFACE_SHIFT(port_num) (17 + ((port_num) * 3))
#define SJA1105_STATIC_CONF_XMII_MODE_INTERFACE_MASK(port_num)  (0x3 << SJA1105_STATIC_CONF_XMII_MODE_INTERFACE_SHIFT(port_num))

/* ---------------------------------------------------------------------------- */
/* Dynamic Reonfiguration */
/* ---------------------------------------------------------------------------- */


enum SJA1105_DynConfReg_Enum {
    SJA1105_DYN_CONF_L2_LUT_REG_0        = 0x29,
    SJA1105_DYN_CONF_L2_LUT_REG_1        = 0x24,
    SJA1105_DYN_CONF_L2_LUT_REG_2        = 0x25,
    SJA1105_DYN_CONF_L2_LUT_REG_3        = 0x26,
    SJA1105_DYN_CONF_L2_LUT_REG_4        = 0x27,
    SJA1105_DYN_CONF_L2_LUT_REG_5        = 0x28,
    SJA1105_DYN_CONF_L2_FORWARDING_REG_0 = 0x2c,
    SJA1105_DYN_CONF_L2_FORWARDING_REG_1 = 0x2a,
    SJA1105_DYN_CONF_L2_FORWARDING_REG_2 = 0x2b,
    SJA1105_DYN_CONF_MAC_CONF_REG_0      = 0x53,
    SJA1105_DYN_CONF_MAC_CONF_REG_1      = 0x4b,
    SJA1105_DYN_CONF_MAC_CONF_REG_2      = 0x4c,
    SJA1105_DYN_CONF_MAC_CONF_REG_3      = 0x4d,
    SJA1105_DYN_CONF_MAC_CONF_REG_4      = 0x4e,
    SJA1105_DYN_CONF_MAC_CONF_REG_5      = 0x4f,
    SJA1105_DYN_CONF_MAC_CONF_REG_6      = 0x50,
    SJA1105_DYN_CONF_MAC_CONF_REG_7      = 0x51,
    SJA1105_DYN_CONF_MAC_CONF_REG_8      = 0x52,
};

#define SJA1105_DYN_CONF_L2_LUT_VALID     (1 << 31)
#define SJA1105_DYN_CONF_L2_LUT_RDRWSET   (1 << 30)
#define SJA1105_DYN_CONF_L2_LUT_ERRORS    (1 << 29)
#define SJA1105_DYN_CONF_L2_LUT_LOCKEDS   (1 << 28)
#define SJA1105_DYN_CONF_L2_LUT_VALIDENT  (1 << 27)
#define SJA1105_DYN_CONF_L2_LUT_MGMTROUTE (1 << 26)

#define SJA1105_L2_LUT_INDEX_OFFSET       (0)
#define SJA1105_L2_LUT_INDEX_SHIFT        (6)
#define SJA1105_L2_LUT_INDEX_MASK         (0x3ff << SJA1105_MGMT_INDEX_SHIFT)

#define SJA1105_MGMT_INDEX_OFFSET         (0)
#define SJA1105_MGMT_INDEX_SHIFT          (6)
#define SJA1105_MGMT_INDEX_MASK           (0x3 << SJA1105_MGMT_INDEX_SHIFT)

#define SJA1105_MGMT_MGMTVALID_OFFSET     (0)
#define SJA1105_MGMT_MGMTVALID_MASK       (1 << 16)

#define SJA1105_MGMT_DESTPORTS_OFFSET     (0)
#define SJA1105_MGMT_DESTPORTS_SHIFT      (17)
#define SJA1105_MGMT_DESTPORTS_MASK       (0x1f << SJA1105_MGMT_DESTPORTS_SHIFT)

#define SJA1105_MGMT_TAKETS_OFFSET        (2)
#define SJA1105_MGMT_TAKETS_MASK          (1 << 6)

#define SJA1105_MGMT_TSREG_OFFSET         (2)
#define SJA1105_MGMT_TSREG_MASK           (1 << 7)

enum SJA1105_L2LUTHostCmd_Enum {
    SJA1105_L2_LUT_HOSTCMD_SEARCH           = 0x1,
    SJA1105_L2_LUT_HOSTCMD_READ             = 0x2,
    SJA1105_L2_LUT_HOSTCMD_WRITE            = 0x3,
    SJA1105_L2_LUT_HOSTCMD_INVALIDATE_ENTRY = 0x4
};

#define SJA1105_L2_LUT_HOSTCMD_SHIFT               (23)
#define SJA1105_L2_LUT_HOSTCMD_MASK                (0x7 << SJA1105_L2_LUT_HOSTCMD_SHIFT)

#define SJA1105_MGMT_L2ADDR_LU_ENTRY_SIZE          (3)

#define SJA1105_DYN_CONF_VALID                     (1 << 31)
#define SJA1105_DYN_CONF_ERRORS                    (1 << 30)
#define SJA1105_DYN_CONF_RDWRSET                   (1 << 29)

#define SJA1105_DYN_CONF_L2_FORWARDING_INDEX_SHIFT (0)
#define SJA1105_DYN_CONF_L2_FORWARDING_INDEX_MASK  (0x1f << SJA1105_DYN_CONF_L2_FORWARDING_INDEX_SHIFT)

#define SJA1105_DYN_CONF_MAC_CONF_PORTID_SHIFT     (0)
#define SJA1105_DYN_CONF_MAC_CONF_PORTID_MASK      (0x7 << SJA1105_DYN_CONF_MAC_CONF_PORTID_SHIFT)


#ifdef __cplusplus
}
#endif

#endif /* SJA1105_INC_SJA1105_REGS_H_ */
