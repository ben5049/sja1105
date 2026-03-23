/*
 * sja1105_tables.h
 *
 *  Created on: Aug 2, 2025
 *      Author: bens1
 */

#ifndef SJA1105_INC_SJA1105_TABLES_H_
#define SJA1105_INC_SJA1105_TABLES_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "sja1105.h"
#include "internal/sja1105_regs.h"


#define SJA1105_GET_TABLE_LENGTH_TYPE(id)                \
    ({                                                   \
        sja1105_table_type_t __table_type;               \
        if ((id) > SJA1105_BLOCK_ID_SGMII_CONF) {        \
            __table_type = SJA1105_TABLE_ID_INVALID;     \
        } else {                                         \
            __table_type = SJA1105_TABLE_TYPE_LUT[(id)]; \
        }                                                \
        __table_type;                                    \
    })

#define SJA1105_GET_TABLE_INDEX(id)                        \
    ({                                                     \
        uint8_t __table_index;                             \
        if ((id) > SJA1105_BLOCK_ID_SGMII_CONF) {          \
            __table_index = UINT8_MAX;                     \
        } else {                                           \
            __table_index = SJA1105_TABLE_INDEX_LUT[(id)]; \
        }                                                  \
        if ((__table_index == 0) && (id != 0)) {           \
            __table_index = UINT8_MAX;                     \
        } else if (__table_index >= SJA1105_NUM_TABLES) {  \
            __table_index = UINT8_MAX;                     \
        }                                                  \
        __table_index;                                     \
    })


extern const sja1105_table_type_t SJA1105_TABLE_TYPE_LUT[SJA1105_BLOCK_ID_SGMII_CONF + 1];
extern const uint8_t              SJA1105_TABLE_INDEX_LUT[SJA1105_BLOCK_ID_SGMII_CONF + 1];

sja1105_status_t SJA1105_CheckTable(sja1105_handle_t *dev, sja1105_block_id_t id, const uint32_t *table_data, uint32_t size);

sja1105_status_t SJA1105_MACConfTableCheck(sja1105_handle_t *dev, const sja1105_table_t *table);
sja1105_status_t SJA1105_ResetMACConfTable(sja1105_handle_t *dev, bool write);
sja1105_status_t SJA1105_MACConfTableWrite(sja1105_handle_t *dev, uint8_t port_num);
sja1105_status_t SJA1105_MACConfTableRead(sja1105_handle_t *dev, uint8_t port_num);
sja1105_status_t SJA1105_MACConfTableGetSpeed(const sja1105_table_t *table, uint8_t port_num, sja1105_speed_t *speed);
sja1105_status_t SJA1105_MACConfTableSetSpeed(sja1105_table_t *table, uint8_t port_num, sja1105_speed_t speed);
sja1105_status_t SJA1105_MACConfTableGetIngress(const sja1105_table_t *table, uint8_t port_num, bool *ingress);
sja1105_status_t SJA1105_MACConfTableSetIngress(sja1105_table_t *table, uint8_t port_num, bool ingress);
sja1105_status_t SJA1105_MACConfTableGetEgress(const sja1105_table_t *table, uint8_t port_num, bool *egress);
sja1105_status_t SJA1105_MACConfTableSetEgress(sja1105_table_t *table, uint8_t port_num, bool egress);
sja1105_status_t SJA1105_MACConfTableGetDynLearn(const sja1105_table_t *table, uint8_t port_num, bool *dyn_learn);
sja1105_status_t SJA1105_MACConfTableSetDynLearn(sja1105_table_t *table, uint8_t port_num, bool dyn_learn);

sja1105_status_t SJA1105_L2ForwardingTableRead(sja1105_handle_t *dev, uint8_t index);

sja1105_status_t SJA1105_GeneralParamsTableCheck(sja1105_handle_t *dev, const sja1105_table_t *table);
sja1105_status_t SJA1105_GetMACFilters(sja1105_handle_t *dev, sja1105_mac_filters_t *mac_filters);

sja1105_status_t SJA1105_xMIIModeTableCheck(sja1105_handle_t *dev, const sja1105_table_t *table);

#ifdef __cplusplus
}
#endif

#endif /* SJA1105_INC_SJA1105_TABLES_H_ */
