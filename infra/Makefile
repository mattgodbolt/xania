.PHONY: default
default: help

.PHONY: help
help: # with thanks to Ben Rady
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

TERRAFORM_VERSION:=0.12.28

APPS_DIR=$(CURDIR)/.apps
TERRAFORM=$(APPS_DIR)/terraform-${TERRAFORM_VERSION}/terraform

$(TERRAFORM):
	@mkdir -p $(dir $(TERRAFORM))
	curl -sL -o $(APPS_DIR)/terraform-$(TERRAFORM_VERSION).zip https://releases.hashicorp.com/terraform/$(TERRAFORM_VERSION)/terraform_$(TERRAFORM_VERSION)_linux_amd64.zip
	unzip $(APPS_DIR)/terraform-$(TERRAFORM_VERSION).zip -d $(dir $(TERRAFORM))

$(TERRAFORM).init: $(TERRAFORM) terraform/main.tf
	$(TERRAFORM) init terraform
	touch $@

.PHONY: terraform-init
terraform-init: $(TERRAFORM).init

.PHONY: terraform
terraform: terraform-init $(TERRAFORM)  ## Apply any pending infrastructure changes
	$(TERRAFORM) apply terraform