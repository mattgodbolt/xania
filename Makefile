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
	rm -rf cmake-build-* $(INSTALL_DIR)
