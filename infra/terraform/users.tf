resource "aws_iam_user" "faramir" {
  name = "faramir"
}

resource "aws_iam_user" "themoog" {
  name = "themoog"
}

resource "aws_iam_group" "admin" {
  name = "xania-admin"
}

data "aws_iam_policy_document" "admin" {
  statement {
    actions = [
      "s3:*"
    ]

    resources = [
      "${aws_s3_bucket.xania.arn}/*",
      aws_s3_bucket.xania.arn
    ]
  }

  // For convenience in the web UI at least allow listing 
  // (NB leaks matt's personal bucket names but yolo)
  statement {
    actions = [
      "s3:ListBuckets",
      "s3:ListAllMyBuckets"
    ]

    resources = [
      "*"
    ]
  }

  statement {
    actions = [
      "ec2:*",
      "iam:GetAccountSummary",
      "iam:ListVirtualMFADevices",
      "iam:ListAccountAliases",
    ]

    resources = [
      "*"
    ]
  }

  statement {
    actions = ["iam:ListUsers"]
    resources = [
    "arn:aws:iam::*:*"]
  }

  statement {
    actions = ["iam:CreateVirtualMFADevice"]
    resources = [
    "arn:aws:iam::*:mfa/*"]
  }

  statement {
    actions = [
      "iam:*AccessKey*",
      "iam:DeactivateMFADevice",
      "iam:EnableMFADevice",
      "iam:GetUser",
      "iam:GetMFADevice",
      "iam:ListMFADevices",
      "iam:ResyncMFADevice"

    ]
    resources = [
      "arn:aws:iam::*:user/$${aws:username}"
    ]
  }
}

resource "aws_iam_group_policy" "admin" {
  name   = "xania-admins"
  group  = aws_iam_group.admin.name
  policy = data.aws_iam_policy_document.admin.json
}

resource "aws_iam_group_membership" "admins" {
  name  = "xania-admins"
  users = ["faramir", "themoog"]
  group = aws_iam_group.admin.name
}