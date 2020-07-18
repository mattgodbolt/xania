data "aws_acm_certificate" "godbolt-org" {
  domain      = "*.xania.org"
  most_recent = true
}

data "aws_iam_policy_document" "InstanceAssumeRolePolicy" {
  statement {
    actions = ["sts:AssumeRole"]
    principals {
      identifiers = ["ec2.amazonaws.com"]
      type        = "Service"
    }
  }
}
