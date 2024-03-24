provider "aws" {
  region  = "us-east-2"
  profile = "xania"
}

terraform {
  required_version = "~> 1.7"
  backend "s3" {
    bucket  = "terraform.xania.org"
    key     = "xania.tfstate"
    region  = "us-east-2"
    profile = "xania"
  }
  required_providers {
    aws = {
      version = "~> 5.41"
    }
  }
}
