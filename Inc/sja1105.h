/*
 * sja1105.h
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 *
 */

#ifndef SJA1105_INC_SJA1105_H_
#define SJA1105_INC_SJA1105_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes*/
#include "stdint.h"
#include "stdbool.h"
#include "stdatomic.h"


/* Defines */

#ifndef MAC_ADDR_SIZE
#define MAC_ADDR_SIZE (6)
#elif (MAC_ADDR_SIZE) != 6
#error "MAC_ADDR_SIZE must be 6 bytes long"
#endif


/* General */
#define SJA1105_NUM_PORTS             (5)
#define SJA1105_NUM_TABLES            (25)
#define SJA1105_FIXED_BUFFER_SIZE     (274) /* Size of the fixed length table buffer */
#define SJA1105_NUM_MGMT_SLOTS        (4)
#define SJA1105_MAX_ATTEMPTS          (10)  /* Maximum number of attempts to try anything. E.g. polling a flag with timeout = 100ms will result in 10 reads 10ms apart. Must be > 0 */
#define SJA1105_L2ADDR_LU_ENTRY_SIZE  (5)
#define SJA1105_L2ADDR_LU_NUM_ENTRIES (1024)

/* Timings */
#define SJA1105_T_RST            (5000)   /* Reset pin pulse width 5000ns (5us) */
#define SJA1105_T_RST_STARTUP_HW (329000) /* 329000ns (329us) */
#define SJA1105_T_RST_STARTUP_SW (2000)   /* 2000ns (2us) */
#define SJA1105_T_SPI_WR         (130)    /* ns */
#define SJA1105_T_SPI_CTRL_DATA  (64)     /* Time between writing the command frame and reading data in ns */
#define SJA1105_T_SPI_LEAD       (40)     /* ns */
#define SJA1105_T_SPI_LAG        (40)     /* ns */


#ifndef SJA1105_PORTS_START_ENABLED
#define SJA1105_PORTS_START_ENABLED true /* Override to "false" to disable port ingress, egress and learning at startup */
#endif

#define SJA1105_SPEED_MBPS_TO_ENUM(mbps) (((mbps) == 10) ? SJA1105_SPEED_10M : (((mbps) == 100) ? SJA1105_SPEED_100M : (((mbps) == 1000) ? SJA1105_SPEED_1G : SJA1105_SPEED_INVALID)))

#ifndef UNUSED
#define UNUSED(x) ((void) (x))
#endif

/* Enable and disable log statements */
#ifndef SJA1105_LOGGING_ENABLED
#define SJA1105_LOGGING_ENABLED (1)
#endif

/* Enable and disable argument checking in functions */
#ifndef SJA1105_CHECKS_ENABLED
#define SJA1105_CHECKS_ENABLED (1)
#endif


/* Typedefs */

typedef struct sja1105_handle_t sja1105_handle_t;

typedef enum {
    SJA1105_PIN_SET,
    SJA1105_PIN_RESET
} sja1105_pinstate_t;

typedef enum {
    SJA1105_OK,
    SJA1105_ERROR,
    SJA1105_BUSY,
    SJA1105_TIMEOUT,
    SJA1105_PARAMETER_ERROR,
    SJA1105_ALREADY_CONFIGURED_ERROR, /* Error when attempting to init an already initialised device or configure a port that has already been configured. */
    SJA1105_NOT_CONFIGURED_ERROR,
    SJA1105_SPI_ERROR,                /* Error occured during SPI transaction, typically set in the SPI user callbacks */
    SJA1105_ID_ERROR,
    SJA1105_STATIC_CONF_ERROR,        /* The static config being processed contains an error */
    SJA1105_MISSING_TABLE_ERROR,      /* A required table or a dependancy for a loaded table is missing */
    SJA1105_CRC_ERROR,                /* CRC Error detected in a static configuration, either by the driver or by the chip */
    SJA1105_RAM_PARITY_ERROR,         /* RAM Parity error detected in one of the chip's memories, must be immediately reset */
    SJA1105_NOT_IMPLEMENTED_ERROR,
    SJA1105_MUTEX_ERROR,              /* Serious mutex error, will normally just return SJA1105_BUSY if it tries to take a mutex held by another thread */
    SJA1105_DYNAMIC_MEMORY_ERROR,     /* Attempted to re-allocate without free, or free after free */
    SJA1105_DYNAMIC_RECONFIG_ERROR,   /* VALID bit not set after performing a dynamic reconfiguration */
    SJA1105_REVERT_ERROR,             /* Catastrophic error has occured such as an error while fixing another error */
    SJA1105_NO_FREE_MGMT_ROUTES_ERROR,
    SJA1105_MEMORY_ERROR,
    SJA1105_STATIC_CONF_FLAGS_READ_ERROR,
    SJA1105_INVALID_VALUE_ERROR,
} sja1105_status_t;

typedef enum {
    VARIANT_SJA1105E        = 0x0,
    VARIANT_SJA1105T        = 0x1,
    VARIANT_SJA1105P        = 0x2,
    VARIANT_SJA1105Q        = 0x3,
    VARIANT_SJA1105R        = 0x4,
    VARIANT_SJA1105S        = 0x5,
    VARIANT_SJA1105_INVALID = 0x6 /* Dummmy value for argument checking */
} sja1105_variant_t;

typedef enum {
    SJA1105_INTERFACE_MII     = 0x0,
    SJA1105_INTERFACE_RMII    = 0x1,
    SJA1105_INTERFACE_RGMII   = 0x2,
    SJA1105_INTERFACE_SGMII   = 0x3,
    SJA1105_INTERFACE_INVALID = 0x4 /* Dummmy value for argument checking */
} sja1105_interface_t;

typedef enum {
    SJA1105_MODE_MAC     = 0x0, /* The port acts like a MAC */
    SJA1105_MODE_PHY     = 0x1, /* The port acts like a PHY and must connect to a true MAC */
    SJA1105_MODE_INVALID = 0x2  /* Dummmy value for argument checking */
} sja1105_mode_t;

typedef enum {
    SJA1105_SPEED_DYNAMIC = 0x0, /* Used for ports that need to negotiate a speed */
    SJA1105_SPEED_1G      = 0x1,
    SJA1105_SPEED_100M    = 0x2,
    SJA1105_SPEED_10M     = 0x3,
    SJA1105_SPEED_INVALID = 0x4 /* Dummmy value for argument checking */
} sja1105_speed_t;

typedef enum {
    SJA1105_IO_VOLTAGE_UNSPECIFIED = 0x0,
    SJA1105_IO_1V8                 = 0x1,
    SJA1105_IO_2V5                 = 0x2,
    SJA1105_IO_3V3                 = 0x3,
    SJA1105_IO_V_INVALID           = 0x4 /* Dummmy value for argument checking */
} sja1105_io_voltage_t;

typedef enum {
    SJA1105_RGMII_ID_NONE      = 0x0,
    SJA1105_RGMII_ID_TX_1NS    = 0x1,
    SJA1105_RGMII_ID_RX_1NS    = 0x2,
    SJA1105_RGMII_ID_TX_RX_1NS = 0x3,
    SJA1105_RGMII_ID_INVALID   = 0x4 /* Dummmy value for argument checking */
} sja1105_rgmii_delay_mode_t;

typedef struct {
    uint8_t                    port_num;
    sja1105_speed_t            speed;
    sja1105_interface_t        interface;
    sja1105_mode_t             mode;
    sja1105_io_voltage_t       voltage;
    sja1105_rgmii_delay_mode_t rgmii_id_mode;             /* Internal delays on RGMII TX/RX clocks */
    bool                       output_rmii_refclk;        /* Only used when interface = SJA1105_INTERFACE_RMII and mode = SJA1105_MODE_PHY */
    bool                       rx_error_unused;           /* Only used when interface is MII or RMII. Controls whether or not the RX_ERR input is pulled down */
    sja1105_handle_t          *connected_switch_handle;   /* For cascaded SJA1105s. Pointer to switch connected on this port */
    uint8_t                    connected_switch_port_num; /* For cascaded SJA1105s. Port number of switch connected on this port */
    bool                       configured;
} sja1105_port_t;

typedef struct {
    sja1105_variant_t variant;
    sja1105_port_t    ports[SJA1105_NUM_PORTS];
    uint32_t          timeout;      /* Timeout in ms for doing anything with a timeout (read, write, take mutex etc) */
    uint32_t          mgmt_timeout; /* Time in ms after creating a manamegement route that it can be overwriten if it hasn't been used */
    uint8_t           host_port;
    uint8_t           casc_port;    /* Port connected to another switch downstream from the host */
    bool              skew_clocks;  /* Make xMII clocks use different phases (where possible) to improve EMC performance */
    uint8_t           switch_id;    /* Used to identify the switch that trapped a frame */
} sja1105_config_t;

typedef struct {
    bool      in_use;         /* Flag set when a table is allocated. Variable length tables not in use will not be written. Fixed length tables will be written regardless of this flag if they have been allocated */
    uint8_t  *id;             /* Table block ID */
    uint32_t *size;           /* Number of uint32_t in data */
    uint32_t *header_crc;     /* Every time the header changes it's CRC should be changed immediately (hence no header_crc_valid) */
    uint32_t *data;           /* Array of uint32_t */
    uint32_t *data_crc;       /* CRC of data */
    bool      data_crc_valid; /* When the data is changed the CRC doesn't have to be recalculated immediately (to prevent recalculation multiple times e.g. when configuring multiple ports at the same time). Instead this flag can be set and the CRC will be calulated prior to writing */
} sja1105_table_t;

typedef enum {
    SJA1105_TABLE_ID_INVALID      = 0x00,
    SJA1105_TABLE_FIXED_LENGTH    = 0x01,
    SJA1105_TABLE_VARIABLE_LENGTH = 0x02,
} sja1105_table_type_t;

typedef struct {
    union {
        struct {
            sja1105_table_t schedule;
            sja1105_table_t schedule_entry_points;
            sja1105_table_t vl_lookup;
            sja1105_table_t vl_policing;
            sja1105_table_t vl_forwarding;
            sja1105_table_t l2_address_lookup;
            sja1105_table_t l2_policing;
            sja1105_table_t vlan_lookup;
            sja1105_table_t l2_forwarding;
            sja1105_table_t mac_configuration;
            sja1105_table_t schedule_parameters;
            sja1105_table_t schedule_entry_point_parameters;
            sja1105_table_t vl_forwarding_parameters;
            sja1105_table_t l2_lookup_parameters;
            sja1105_table_t l2_forwarding_parameters;
            sja1105_table_t clock_synchronisation_parameters;
            sja1105_table_t avb_parameters;
            sja1105_table_t general_parameters;
            sja1105_table_t retagging;
            sja1105_table_t credit_based_shaping;
            sja1105_table_t xmii_mode_parameters;
            sja1105_table_t cgu_config_parameters;
            sja1105_table_t rgu_config_parameters;
            sja1105_table_t acu_config_parameters;
            sja1105_table_t sgmii_config_parameters;
        };
        sja1105_table_t by_index[SJA1105_NUM_TABLES];
    };
    union {
        uint32_t *device_id; /* Also the pointer to the start of the fixed length portion of the generic loader structure */
        uint32_t *fixed_length_buffer;
    };
    uint32_t *first_free; /* Starts at device_id + 1. Every time a table is added it is moved to the first pointer after the table */
    uint32_t  global_crc;
    bool      global_crc_valid;
} sja1105_tables_t;

typedef struct {
    uint8_t mac_fltres0[MAC_ADDR_SIZE];
    uint8_t mac_flt0[MAC_ADDR_SIZE];
    bool    send_meta0;
    bool    incl_srcpt0;
    uint8_t mac_fltres1[MAC_ADDR_SIZE];
    uint8_t mac_flt1[MAC_ADDR_SIZE];
    bool    send_meta1;
    bool    incl_srcpt1;
} sja1105_mac_filters_t;

/* Stores information about driver events */
typedef struct {
    uint32_t static_conf_uploads;
    uint32_t resets;
    uint32_t words_read;
    uint32_t words_written;
    uint32_t crc_errors;
    uint32_t spi_errors;
    uint32_t mgmt_frames_sent;
    uint32_t mgmt_entries_dropped;
    uint32_t frames_dropped[SJA1105_NUM_PORTS];
} sja1105_event_counters_t;

/* Stores information about management routes */
typedef struct {
    bool     slot_taken[SJA1105_NUM_MGMT_SLOTS]; /* true = slot has been taken */
    uint32_t timestamps[SJA1105_NUM_MGMT_SLOTS]; /* Time when route was created, used to evict old rules (TODO) */
    void    *contexts[SJA1105_NUM_MGMT_SLOTS];   /* Context set by SJA1105_ManagementRouteCreate() caller so they can tell if their entry has been evicted. */
} sja1105_mgmt_routes_t;

typedef void (*sja1105_callback_write_cs_pin_t)(sja1105_pinstate_t state, void *context);
typedef sja1105_status_t (*sja1105_callback_spi_transmit_t)(const uint32_t *data, uint16_t size, uint32_t timeout, void *context);
typedef sja1105_status_t (*sja1105_callback_spi_receive_t)(uint32_t *data, uint16_t size, uint32_t timeout, void *context);
typedef sja1105_status_t (*sja1105_callback_spi_transmit_receive_t)(const uint32_t *tx_data, uint32_t *rx_data, uint16_t size, uint32_t timeout, void *context);
typedef uint32_t (*sja1105_callback_get_time_ms_t)(void *context);
typedef void (*sja1105_callback_delay_ms_t)(uint32_t ms, void *context);
typedef void (*sja1105_callback_delay_ns_t)(uint32_t ns, void *context);
typedef sja1105_status_t (*sja1105_callback_take_mutex_t)(uint32_t timeout, void *context);
typedef sja1105_status_t (*sja1105_callback_give_mutex_t)(void *context);
typedef sja1105_status_t (*sja1105_callback_allocate_t)(uint32_t **memory_ptr, uint32_t size, void *context);
typedef sja1105_status_t (*sja1105_callback_free_t)(uint32_t *memory_ptr, void *context);
typedef sja1105_status_t (*sja1105_callback_free_all_t)(void *context);
typedef sja1105_status_t (*sja1105_callback_crc_reset_t)(void *context);
typedef sja1105_status_t (*sja1105_callback_crc_accumulate_t)(const uint32_t *buffer, uint32_t size, uint32_t *result, void *context);
typedef void (*sja1105_callback_write_log_t)(const char *format, ...);

typedef struct {
    sja1105_callback_write_cs_pin_t         callback_write_cs_pin;         /* Write change the state of the chip select (CS) pin */
    sja1105_callback_spi_transmit_t         callback_spi_transmit;         /* Write data via SPI */
    sja1105_callback_spi_receive_t          callback_spi_receive;          /* Read data via SPI */
    sja1105_callback_spi_transmit_receive_t callback_spi_transmit_receive; /* Write and read data via SPI */
    sja1105_callback_get_time_ms_t          callback_get_time_ms;          /* Get time in ms */
    sja1105_callback_delay_ms_t             callback_delay_ms;             /* Non-blocking delay in ms */
    sja1105_callback_delay_ns_t             callback_delay_ns;             /* Blocking delay in ns */
    sja1105_callback_take_mutex_t           callback_take_mutex;           /* Take the mutex protecting the device */
    sja1105_callback_give_mutex_t           callback_give_mutex;           /* Give the mutex protecting the device */
    sja1105_callback_allocate_t             callback_allocate;             /* Allocate a given number of 32-bit words */
    sja1105_callback_free_t                 callback_free;                 /* Free memory */
    sja1105_callback_free_all_t             callback_free_all;             /* Free all allocated memory */
    sja1105_callback_crc_reset_t            callback_crc_reset;            /* Reset the CRC state before starting */
    sja1105_callback_crc_accumulate_t       callback_crc_accumulate;       /* Compute the CRC over new data, and all previous data since the last reset */
    sja1105_callback_write_log_t            callback_write_log;            /* Write a log message */
} sja1105_callbacks_t;

struct sja1105_handle_t {
    const sja1105_config_t    *config;
    const sja1105_callbacks_t *callbacks;
    void                      *callback_context;
    sja1105_tables_t           tables;
    sja1105_event_counters_t   events;
    sja1105_mgmt_routes_t      management_routes;
    atomic_bool                initialised;
};

/* Stores informations from device status registers */
typedef struct {
    uint64_t tx_bytes[SJA1105_NUM_PORTS];
    uint64_t rx_bytes[SJA1105_NUM_PORTS];
    uint32_t dropped_frames[SJA1105_NUM_PORTS];
} sja1105_statistics_t;


/* Functions */

/* Initialisation */
sja1105_status_t SJA1105_PortConfigure(sja1105_config_t *config, const sja1105_port_t *port_config, bool cascaded);
sja1105_status_t SJA1105_Init(sja1105_handle_t *dev, const sja1105_config_t *config, const sja1105_callbacks_t *callbacks, void *callback_context, uint32_t fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE], const uint32_t *static_conf, uint32_t static_conf_size);
sja1105_status_t SJA1105_DeInit(sja1105_handle_t *dev, bool hard, bool clear_counters);
sja1105_status_t SJA1105_ReInit(sja1105_handle_t *dev, const uint32_t *static_conf, uint32_t static_conf_size);

/* Dynamic reconfiguration */
sja1105_status_t SJA1105_PortGetState(sja1105_handle_t *dev, uint8_t port_num, bool *state);
sja1105_status_t SJA1105_PortGetSpeed(sja1105_handle_t *dev, uint8_t port_num, sja1105_speed_t *speed);
sja1105_status_t SJA1105_PortSetSpeed(sja1105_handle_t *dev, uint8_t port_num, sja1105_speed_t speed);
sja1105_status_t SJA1105_PortSetLearning(sja1105_handle_t *dev, uint8_t port_num, bool enable);
sja1105_status_t SJA1105_PortGetForwarding(sja1105_handle_t *dev, uint8_t port_num, bool *forwarding);
sja1105_status_t SJA1105_PortSetForwarding(sja1105_handle_t *dev, uint8_t port_num, bool enable);
sja1105_status_t SJA1105_PortSleep(sja1105_handle_t *dev, uint8_t port_num);
sja1105_status_t SJA1105_PortWake(sja1105_handle_t *dev, uint8_t port_num);

/* Maintenance */
sja1105_status_t SJA1105_ReadTemperature(sja1105_handle_t *dev, float *temp);
sja1105_status_t SJA1105_CheckStatusRegisters(sja1105_handle_t *dev);
sja1105_status_t SJA1105_MACAddrTrapTest(sja1105_handle_t *dev, const uint8_t *addr, bool *trapped, bool *send_meta, bool *incl_srcpt);
sja1105_status_t SJA1105_ReadAllTables(sja1105_handle_t *dev);
sja1105_status_t SJA1105_ReadStatistics(sja1105_handle_t *dev, sja1105_statistics_t *stats);

/* Utilities */
sja1105_status_t SJA1105_L2EntryReadByIndex(sja1105_handle_t *dev, uint16_t index, bool managment, uint32_t entry[SJA1105_L2ADDR_LU_ENTRY_SIZE]);
sja1105_status_t SJA1105_ManagementRouteCreate(sja1105_handle_t *dev, const uint8_t dst_addr[MAC_ADDR_SIZE], uint8_t dst_ports, bool takets, bool tsreg, void *context);
sja1105_status_t SJA1105_ManagementRouteFree(sja1105_handle_t *dev, bool force);
sja1105_status_t SJA1105_FlushTCAM(sja1105_handle_t *dev);


#ifdef __cplusplus
}
#endif

#endif /* SJA1105_INC_SJA1105_H_ */
