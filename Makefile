.PHONY: default
default: install

.PHONY: help
help: # with thanks to Ben Rady
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

ifeq ($(shell which g++-11),)
CXX?=g++-10
CC?=gcc-10
else
CXX?=g++-11
CC?=gcc-11
endif

CMAKE?=$(shell which cmake || echo .cmake-not-found)
CURL?=$(shell which curl || echo .curl-not-found)
CURL_OPTIONS:=-sL --fail -m 120 --connect-timeout 3 --retry 3 --retry-max-time 360
BUILD_TYPE?=debug
BUILD_ROOT:=$(CURDIR)/cmake-build-$(BUILD_TYPE)
INSTALL_DIR=$(CURDIR)/install
TOOLS_DIR=$(CURDIR)/.tools
CLANG_VERSION?=10
CLANG_FORMAT:=$(TOOLS_DIR)/clang-format-$(CLANG_VERSION)
TOP_SRC_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCE_FILES:=$(shell find $(TOP_SRC_DIR)/src -type f -name \*.c -o -name \*.h -o -name \*.cpp -o -name *.hpp)

ifeq ($(shell which ninja),)
CMAKE_GENERATOR_FLAGS?=
else
CMAKE_GENERATOR_FLAGS?=-GNinja
endif

.%-not-found:
	@echo "-----------------------"
	@echo "Xania needs $(@:.%-not-found=%) to build. Please apt install it "
	@echo "-----------------------"
	@exit 1

.PHONY: build
build: ## Build Xania source
	PATH=${PATH}:$(CONDA_ROOT)/bin \
	  $(CMAKE) -S $(TOP_SRC_DIR)/conan -B $(BUILD_ROOT) $(CMAKE_GENERATOR_FLAGS) \
	                    -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	$(CMAKE) --build $(BUILD_ROOT)

# Grr older cmakes don't support --install and --prefix
install: build dirs  ## Install to 'install' (overridable with INSTALL_DIR)
	@mkdir -p $(INSTALL_DIR)
	$(CMAKE) --build $(BUILD_ROOT) --target install

.PHONY: dirs
dirs:
	@mkdir -p gods player system log
	@PROJ_ROOT=$(CURDIR) ./scripts/validate-symlinks.sh

# ideally would check the sha512 here. TODO: This
$(CLANG_FORMAT): $(CURL)
	@mkdir -p $(dir $@)
	@echo "Installing clang format static binary locally..."
	$(CURL) $(CURL_OPTIONS) https://github.com/muttleyxd/clang-format-static-binaries/releases/download/master-5b56bb49/clang-format-$(CLANG_VERSION)_linux-amd64 -o $@
	@chmod +x $@

.PHONY: start
start: install 	  ## Build and start Xania
	./start-mud.sh

.PHONY: stop
stop: dirs  ## Stop Xania
	@echo "Stopping Xania..."
	-killall --quiet --wait $(INSTALL_DIR)/bin/xania
	@echo "Stopping doorman..."
	-killall --quiet --wait $(INSTALL_DIR)/bin/doorman

.PHONY: restart
restart: stop start  ## Restart Xania

.PHONY: distclean
distclean:  ## Clean up everything
	rm -rf cmake-build-* $(TOOLS_DIR) $(INSTALL_DIR)

.PHONY: clean
clean:  ## Clean up built stuff.
	rm -rf cmake-build-* $(INSTALL_DIR)

.PHONY: reformat-code
reformat-code: $(CLANG_FORMAT)  ## Reformat all the code to conform to the clang-tidy settings
	$(CLANG_FORMAT) -i $(SOURCE_FILES)

.PHONY: check-format
check-format: $(CLANG_FORMAT)  ## Check that the code conforms to the format
	$(CLANG_FORMAT) --dry-run -Werror $(SOURCE_FILES)

.PHONY: test
test: build  ## Build and run tests
	$(CMAKE) --build $(BUILD_ROOT) -- test
