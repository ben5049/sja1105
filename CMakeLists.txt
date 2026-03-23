add_library(sja1105 STATIC
    src/sja1105.c
    src/sja1105_acu.c
    src/sja1105_cgu.c
    src/sja1105_init.c
    src/sja1105_io.c
    src/sja1105_static_conf.c
    src/sja1105_tables.c
)

target_include_directories(sja1105 PUBLIC
    inc
)

target_compile_options(sja1105 PRIVATE
    -ffunction-sections
    -fdata-sections
)
