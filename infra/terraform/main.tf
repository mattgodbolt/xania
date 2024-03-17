provider "aws" {
  region  = "us-east-1"
}

terraform {
  required_version = "~> 1.7"
  backend "s3" {
    bucket = "terraform.godbolt.org"
    key    = "xania.tfstate"
    region = "us-east-1"
  }
  required_providers {
    aws = {
      version = "~> 5.41"
    }
  }
}
