idf_component_register(SRCS "Buffer.cpp"
							"UARTCore.cpp"
							"UARTCommunicator.cpp"
							"ESPNowCore.cpp"
							"ESPNowCommunicator.cpp"
							"Slave.cpp"
							"STR.cpp"
							"CommunicatorBridge.cpp"
							"Setting.cpp"
							"Settingator.cpp"
                    INCLUDE_DIRS "include"
					REQUIRES driver esp_wifi nvs_flash esp_timer log freertos esp_system
				)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
