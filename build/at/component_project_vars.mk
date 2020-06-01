# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/components/at/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/at -L $(PROJECT_PATH)/components/at/lib -lat_core
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/components/at/lib/libat_core.a
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += at
component-at-build: 
