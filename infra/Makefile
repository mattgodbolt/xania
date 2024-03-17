.PHONY: default
default: help

.PHONY: help
help: # with thanks to Ben Rady
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

PACKER_VERSION:=1.6.0
TERRAFORM_VERSION:=1.7.4

APPS_DIR=$(CURDIR)/.apps
TERRAFORM=$(APPS_DIR)/terraform-${TERRAFORM_VERSION}/terraform
PACKER=$(APPS_DIR)/packer-${PACKER_VERSION}/packer
PYTHON:=$(shell which python3 || echo .python3-not-found)

$(PYTHON):
	echo "Unable to find a python installation"
	exit 1

# Perhaps consider macros if it wasn't so painful
$(TERRAFORM):
	@mkdir -p $(dir $(TERRAFORM))
	curl -sL -o $(APPS_DIR)/terraform-$(TERRAFORM_VERSION).zip https://releases.hashicorp.com/terraform/$(TERRAFORM_VERSION)/terraform_$(TERRAFORM_VERSION)_linux_amd64.zip
	unzip $(APPS_DIR)/terraform-$(TERRAFORM_VERSION).zip -d $(dir $(TERRAFORM))

$(PACKER):
	@mkdir -p $(dir $(PACKER))
	curl -sL -o $(APPS_DIR)/packer-$(PACKER_VERSION).zip https://releases.hashicorp.com/packer/$(PACKER_VERSION)/packer_$(PACKER_VERSION)_linux_amd64.zip
	unzip $(APPS_DIR)/packer-$(PACKER_VERSION).zip -d $(dir $(PACKER))

$(TERRAFORM).init: $(TERRAFORM) terraform/main.tf
	$(TERRAFORM) -chdir=terraform init
	touch $@

.PHONY: terraform-init
terraform-init: $(TERRAFORM).init

.PHONY: terraform
terraform: terraform-init $(TERRAFORM)  ## Apply any pending infrastructure changes
	$(TERRAFORM) -chdir=terraform apply

PACKER_CONFIG=$(APPS_DIR)/packer-config.json

$(PACKER_CONFIG): make_json.py $(HOME)/.aws/credentials | $(PYTHON)
	$(PYTHON) make_json.py $(PACKER_CONFIG)

.PHONY: packer
packer: $(PACKER) $(PACKER_CONFIG)  ## Create a new Xania base image (AWS AMI image)
	$(PACKER) build -timestamp-ui -var-file=$(PACKER_CONFIG) packer.json