add_library(sja1105 STATIC
    Src/sja1105.c
    Src/sja1105_acu.c
    Src/sja1105_cgu.c
    Src/sja1105_init.c
    Src/sja1105_io.c
    Src/sja1105_static_conf.c
    Src/sja1105_tables.c
)

target_include_directories(sja1105 PUBLIC
    Inc
)

target_compile_options(sja1105 PRIVATE
    -ffunction-sections
    -fdata-sections
)
