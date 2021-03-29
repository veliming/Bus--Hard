Q := @

.PHONY: prepare all clean test sanity

all: prepare $(OUT_DIR)/$(LIB_SDK_TARGET)

prog: all $(PROG_TARGET)

test: all $(TEST_TARGET)

prepare:
	-$(Q)mkdir -p $(OUT_DIR)

$(OUT_DIR)/$(LIB_SDK_TARGET): $(LIB_OBJ_FILES)
	$(Q)echo "o Archiving $@ ..."
	$(Q)$(AIOT_AR) -rcs $@ $^

$(PROG_TARGET): $(EXT_OBJ_FILES) $(OUT_DIR)/$(LIB_SDK_TARGET)
	$(Q)echo "+ Linking $(OUT_DIR)/$(notdir $@) ..."
	$(Q)mkdir -p $(dir $@)
	$(Q)$(AIOT_CC) -o $@ \
	    $(patsubst $(OUT_DIR)/%,%,$(subst -,_,$(addsuffix .c,$@))) \
	    $(BLD_CFLAGS) $(BLD_LDFLAGS) \
	    $^
	$(Q)mv $@ $(OUT_DIR)

$(TEST_TARGET): $(TST_OBJ_FILES) $(EXT_OBJ_FILES) $(OUT_DIR)/$(LIB_SDK_TARGET)
	$(Q)echo "+ Linking $(OUT_DIR)/$(notdir $@) ..."
	$(Q)$(AIOT_CC) -o $@ \
	    $(BLD_CFLAGS) $(BLD_LDFLAGS) \
	    $^

$(OUT_DIR)/%.o: %.c
	$(Q)echo ": Compiling $< ..."
	$(Q)mkdir -p $(OUT_DIR)/$(dir $<)
	$(Q)$(AIOT_CC) -o $@ -c $< $(BLD_CFLAGS)

stat:
	$(Q)bash host-tools/rom_stat.sh . $(OUT_DIR)

sanity:
	@echo -e "\nBelow file(s) contain 'return -1' !\n"|grep --color ".*"
	@grep -l 'return *-[0-9]' $(LIB_SRC_FILES) $(EXT_SRC_FILES) | grep -v 'external/mbedtls' | awk '{ print "    . "$$0 }'
	@echo -e "\nBelow file(s) not end with newline !\n"|grep --color ".*"
	@for iter in $$(find $(SRC_DIR) -name "*.[ch]"); do \
	    if [ "$$(sed -n '$$p' $${iter}|cat -e)" != "$$" ]; then \
	        echo "    . $${iter}"; \
	    fi; \
	done
	@echo -e "\nBelow file(s) missed 'extern \"C\"' !\n"|grep --color ".*"
	@for iter in $$(find $(filter-out external,$(SRC_DIR)) -name "*.h"); do \
	    if ! grep -q 'extern "C"' $${iter}; then \
	        echo "    . $${iter}"; \
	    fi; \
	done
	@echo -e "\nBelow file(s) contain duplicated state_code !\n"|grep --color ".*"
	@for iter in $$(find $(filter-out external,$(SRC_DIR)) -name "*_api.c"); do \
	    L=$$(grep -o 'STATE_[_A-Z]*' $${iter}|sort -u|grep -v STATE_SUCCESS); \
	    for state in $${L}; do \
	        echo $${state}|grep -q '_LOG_' && continue; \
	        C=$$(grep -c $${state} $${iter}); \
	        if [ "$${C}" != "1" ]; then \
	            echo "    . [$${iter}] $${state} ($${C}) times!"|grep --color "[0-9]*"; \
	        fi \
	    done \
	done
	@echo ""

doc:
	$(Q)doxygen host-tools/Doxyfile > /dev/null && echo -e "\nDocument generation completed\n"
	$(Q)cd $(OUT_DIR) && zip -qr html.zip html

clean:
	$(Q)rm -rf $(OUT_DIR)

