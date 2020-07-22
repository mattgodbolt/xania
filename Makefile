.PHONY: default
default: start

.PHONY: help
help: # with thanks to Ben Rady
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

PORT?=9000
CMAKE?=$(shell which cmake || .cmake-not-found)
BUILD_TYPE?=debug
BUILD_ROOT:=$(CURDIR)/cmake-build-$(BUILD_TYPE)
INSTALL_DIR=$(CURDIR)/install
TOOLS_DIR=$(CURDIR)/.tools
CLANG_VERSION?=10
CLANG_TIDY:=$(TOOLS_DIR)/clang-tidy-$(CLANG_VERSION)
SOURCE_FILES:=$(shell find src -type f -name \*.c -o -name \*.h -o -name \*.cpp -o -name \*.C)

ifeq ($(shell which ninja),)
CMAKE_GENERATOR_FLAGS?=
else
CMAKE_GENERATOR_FLAGS?=-GNinja
endif

.cmake-not-found:
	@echo "Xania needs cmake to build"
	exit 1

.PHONY: build
build: $(BUILD_ROOT)/CMakeCache.txt  ## Build Xania source
	$(CMAKE) --build $(BUILD_ROOT)

# Grr older cmakes don't support --install and --prefix
install: build
	@mkdir -p $(INSTALL_DIR)
	$(CMAKE) --build $(BUILD_ROOT) --target install

.PHONY: dirs
dirs:
	@mkdir -p gods player log

$(CLANG_TIDY):
	@mkdir -p $(dir $@)
	@echo "Installing clang format static binary locally..."
	curl -sL --fail https://github.com/muttleyxd/clang-format-static-binaries/releases/download/master-5b56bb49/clang-format-$(CLANG_VERSION)_linux-amd64 -o $@
	# ideally would check the sha512 here. TODO: This
	chmod +x $@

.PHONY: start
start: install dirs  ## Build and start Xania
	@rm -f area/shutdown.txt
	@echo "Starting Xania on port $(PORT)}"
	(cd src && ./mudmgr -s $(PORT))
	@sleep 3
	@echo "All being well, telnet localhost $(PORT) to log in"

.PHONY: stop
stop: install dirs  ## Stop Xania
	@echo "Stopping Xania"
	(cd src && ./mudmgr -d)

.PHONY: restart
restart: install dirs  ## Restart Xania
	@echo "Restarting Xania"
	(cd src && ./mudmgr -r $(PORT))

# Grr older cmakes don't support -S -B
$(BUILD_ROOT)/CMakeCache.txt:
	@mkdir -p $(BUILD_ROOT)
	cd $(BUILD_ROOT) && $(CMAKE) .. $(CMAKE_GENERATOR_FLAGS) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)

.PHONY: clean
clean:  ## Clean up everything
	rm -rf cmake-build-* $(TOOLS_DIR) $(INSTALL_DIR)

.PHONY: reformat-code
reformat-code: $(CLANG_TIDY)  ## Reformat all the code to conform to the clang-tidy settings
	$(CLANG_TIDY) -i $(SOURCE_FILES)

.PHONY: check-format
check-format: $(CLANG_TIDY)  ## Check that the code conforms to the format
	$(CLANG_TIDY) --dry-run -Werror $(SOURCE_FILES)