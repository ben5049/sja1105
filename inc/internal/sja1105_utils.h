/*
 * sja1105_utils.h
 *
 *  Created on: May 17, 2026
 *      Author: bens1
 */

#ifndef SJA1105_INC_SJA1105_UTILS_H_
#define SJA1105_INC_SJA1105_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stddef.h"

#include "sja1105.h"


#define MIN(a, b)                 ((a) < (b) ? (a) : (b))
#define MAX(a, b)                 ((a) > (b) ? (a) : (b))
#define CONSTRAIN(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define ABS(x)                    ((x) > 0 ? (x) : -(x))

#define U32_BYTE_0(reg)           ((uint8_t) (((uint32_t) (reg) >> (0 * 8)) & 0xff))
#define U32_BYTE_1(reg)           ((uint8_t) (((uint32_t) (reg) >> (1 * 8)) & 0xff))
#define U32_BYTE_2(reg)           ((uint8_t) (((uint32_t) (reg) >> (2 * 8)) & 0xff))
#define U32_BYTE_3(reg)           ((uint8_t) (((uint32_t) (reg) >> (3 * 8)) & 0xff))

#define SJA1105_GET_TIME_MS()     dev->callbacks->callback_get_time_ms(dev->callback_context)
#define SJA1105_DELAY_NS(ns)      dev->callbacks->callback_delay_ns((ns), dev->callback_context)
#define SJA1105_DELAY_MS(ms)      dev->callbacks->callback_delay_ms((ms), dev->callback_context)

#define SJA1105_LOCK                                                                               \
    do {                                                                                           \
        status = dev->callbacks->callback_take_mutex(dev->config->timeout, dev->callback_context); \
        if (status != SJA1105_OK) return status;                                                   \
    } while (0)

#define SJA1105_UNLOCK                               dev->callbacks->callback_give_mutex(dev->callback_context)

#define SJA1105_ENTER_CRITICAL                       dev->callbacks->callback_enter_critical(dev->callback_context)
#define SJA1105_EXIT_CRITICAL                        dev->callbacks->callback_exit_critical(dev->callback_context)

#define SJA1105_ALLOCATE(memory_ptr, size)           dev->callbacks->callback_allocate((memory_ptr), (size), dev->callback_context)
#define SJA1105_FREE(memory_ptr)                     dev->callbacks->callback_free((memory_ptr), dev->callback_context)
#define SJA1105_FREE_ALL()                           dev->callbacks->callback_free_all(dev->callback_context)

#define SJA1105_CRC_RESET()                          dev->callbacks->callback_crc_reset(dev->callback_context)
#define SJA1105_CRC_ACCUMULATE(buffer, size, result) dev->callbacks->callback_crc_accumulate((buffer), (size), (result), dev->callback_context)

#if SJA1105_LOGGING_ENABLED
#define SJA1105_LOG(format, ...) dev->callbacks->callback_write_log("%s:%u (id=%u) " format, __FILE_NAME__, __LINE__, dev->config->switch_id, ##__VA_ARGS__)
#else
#define SJA1105_LOG(format, ...)
#endif


static inline sja1105_handle_t *SJA1105_GetCasc(sja1105_handle_t *dev) {
    if (dev->config->casc_port >= SJA1105_NUM_PORTS) {
        return NULL;
    } else {
        return dev->config->ports[dev->config->casc_port].connected_switch_handle;
    }
}


#ifdef __cplusplus
}
#endif

#endif /* SJA1105_INC_SJA1105_UTILS_H_ */
