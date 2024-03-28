resource "aws_iam_user" "faramir" {
  name = "faramir"
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

  statement {
    actions = [
      "ec2:*",
      "iam:ListVirtualMFADevices",
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
