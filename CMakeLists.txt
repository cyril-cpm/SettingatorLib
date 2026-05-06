idf_component_register(SRC_DIRS "src"
					INCLUDE_DIRS "include"
					REQUIRES driver esp_wifi nvs_flash esp_timer log freertos esp_system
				)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
