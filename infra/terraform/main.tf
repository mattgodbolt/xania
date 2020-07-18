provider "aws" {
  region  = "us-east-1"
  version = "~> 2.15"
}

terraform {
  required_version = "~> 0.12"
  backend "s3" {
    bucket = "terraform.godbolt.org"
    key    = "xania.tfstate"
    region = "us-east-1"
  }
}
