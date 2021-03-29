include build.settings

LIB_SDK_TARGET := libaiot_sdk.a

EXT_DIR := external
PORT_DIR := portfiles/aiot_port
OUT_DIR := output

SRC_DIR := core
SRC_DIR += $(shell [ -d components ] && find components -maxdepth 1 -mindepth 1 -type d)

HDR_DIR := $(shell find $(SRC_DIR) $(EXT_DIR) $(PORT_DIR) -not -path "*/.git*" -type d)
HDR_DIR += host-tools

BLD_CFLAGS += $(EXT_CFLAGS)
BLD_CFLAGS += -Wall -Werror
BLD_CFLAGS += -Os
BLD_CFLAGS += $(addprefix -I,$(HDR_DIR))

LIB_SRC_FILES := $(shell find $(SRC_DIR) \
    -not -path "*/demos/*.c" \
    -not -path "*/tests/*.c" \
    -name "*.c")
LIB_OBJ_FILES := $(LIB_SRC_FILES:.c=.o)
LIB_OBJ_FILES := $(addprefix $(OUT_DIR)/,$(LIB_OBJ_FILES))

EXT_SRC_FILES := $(foreach D,$(EXT_DIR),$(wildcard $(D)/*.c $(D)/*/*.c $(D)/*/*/*.c))
EXT_SRC_FILES += $(shell find $(PORT_DIR) \
    -not -path "*/*/demos/*.c" \
    -not -path "*/*/tests/*.c" \
    -name "*.c")
EXT_OBJ_FILES := $(EXT_SRC_FILES:.c=.o)
EXT_OBJ_FILES := $(addprefix $(OUT_DIR)/,$(EXT_OBJ_FILES))

TST_SRC_FILES := $(wildcard */*_test.c */*/*_test.c components/*/*/*_test.c portfiles/aiot_port/*/*_test.c)
TST_OBJ_FILES := $(TST_SRC_FILES:.c=.o)
TST_OBJ_FILES := $(addprefix $(OUT_DIR)/,$(TST_OBJ_FILES))

PROG_TARGET += $(subst _,-,$(patsubst %.c,%,$(wildcard components/*/demos/*_demo.c)))
PROG_TARGET += $(subst _,-,$(patsubst %.c,%,$(wildcard */demos/*_demo.c)))
PROG_TARGET := $(foreach V,$(basename $(PROG_TARGET)),$(OUT_DIR)/$(V))

TEST_TARGET := $(OUT_DIR)/sdk-testsuite

include host-tools/rules.mk
