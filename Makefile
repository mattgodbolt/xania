.PHONY: default
default: start

.PHONY: help
help: # with thanks to Ben Rady
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

PORT?=9000
CMAKE?=$(shell which cmake || echo .cmake-not-found)
CURL?=$(shell which curl || echo .curl-not-found)
CURL_OPTIONS:=-sL --fail -m 120 --connect-timeout 3 --retry 3 --retry-max-time 360
BUILD_TYPE?=debug
BUILD_ROOT:=$(CURDIR)/cmake-build-$(BUILD_TYPE)
INSTALL_DIR=$(CURDIR)/install
TOOLS_DIR=$(CURDIR)/.tools
CLANG_VERSION?=10
CLANG_FORMAT:=$(TOOLS_DIR)/clang-format-$(CLANG_VERSION)
CONDA_VERSION?=4.7.12
CONDA_ROOT:=$(TOOLS_DIR)/conda-$(CONDA_VERSION)
CONDA_INSTALLER=$(TOOLS_DIR)/conda-$(CONDA_VERSION)/installer.sh
CONDA:=$(CONDA_ROOT)/bin/conda
PIP:=$(CONDA_ROOT)/bin/pip
CONAN:=$(CONDA_ROOT)/bin/conan
SOURCE_FILES:=$(shell find src -type f -name \*.c -o -name \*.h -o -name \*.cpp -o -name \*.C)

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
build: $(BUILD_ROOT)/CMakeCache.txt  ## Build Xania source
	$(CMAKE) --build $(BUILD_ROOT)

# Grr older cmakes don't support --install and --prefix
install: build
	@mkdir -p $(INSTALL_DIR)
	$(CMAKE) --build $(BUILD_ROOT) --target install

.PHONY: dirs
dirs:
	@mkdir -p gods player log

$(CONDA): $(CURL)
	@mkdir -p $(CONDA_ROOT)
	@echo "Installing conda locally..."
	$(CURL) $(CURL_OPTIONS) https://repo.anaconda.com/miniconda/Miniconda3-${CONDA_VERSION}-Linux-x86_64.sh -o $(CONDA_INSTALLER)
	@chmod +x $(CONDA_INSTALLER)
	$(CONDA_INSTALLER) -u -b -p $(CONDA_ROOT)
$(PIP): $(CONDA) # ideally would specify two outputs in $(CONDA) but make -j fails with that

$(CONAN): $(PIP)
	@echo "Installing conan locally..."
	$(PIP) install conan==1.27.1

.PHONY: conda conan
conda: $(CONDA)
conan: $(CONAN)

.PHONY: deps cmake-print-deps
deps: conda conan $(CLANG_FORMAT)
cmake-print-deps: deps Makefile
	@echo "# Automatically created by the Makefile - DO NOT EDIT" > $(CMAKE_CONFIG_FILE)
	@echo "set(CMAKE_PROGRAM_PATH $(CONDA_ROOT)/bin $(CMAKE_PROGRAM_PATH))" >> $(CMAKE_CONFIG_FILE)


# ideally would check the sha512 here. TODO: This
$(CLANG_FORMAT): $(CURL)
	@mkdir -p $(dir $@)
	@echo "Installing clang format static binary locally..."
	$(CURL) $(CURL_OPTIONS) https://github.com/muttleyxd/clang-format-static-binaries/releases/download/master-5b56bb49/clang-format-$(CLANG_VERSION)_linux-amd64 -o $@
	@chmod +x $@

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
reformat-code: $(CLANG_FORMAT)  ## Reformat all the code to conform to the clang-tidy settings
	$(CLANG_FORMAT) -i $(SOURCE_FILES)

.PHONY: check-format
check-format: $(CLANG_FORMAT)  ## Check that the code conforms to the format
	$(CLANG_FORMAT) --dry-run -Werror $(SOURCE_FILES)

.PHONY: test
test: build  ## Build and run tests
	$(CMAKE) --build $(BUILD_ROOT) -- test