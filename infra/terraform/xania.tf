resource "aws_vpc" "xania" {
  cidr_block = "172.33.0.0/16"
  enable_dns_hostnames = true
  enable_dns_support = true
  instance_tenancy = "default"

  tags = {
    Name = "Xania"
    Site = "mud.xania.org"
  }
}

resource "aws_subnet" "xania-1a" {
  vpc_id = aws_vpc.xania.id
  cidr_block = "172.33.0.0/24"
  availability_zone = "us-east-1a"
  map_public_ip_on_launch = true

  tags = {
    Name = "Xania-1a"
    Site = "mud.xania.org"
  }
}

resource "aws_internet_gateway" "xania" {
  vpc_id = aws_vpc.xania.id
  tags = {
    Name = "Xania"
    Site = "mud.xania.org"
  }
}

resource "aws_route_table" "xania" {
  vpc_id = aws_vpc.xania.id
  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = aws_internet_gateway.xania.id
  }
}

resource "aws_route_table_association" "xania" {
  subnet_id = aws_subnet.xania-1a.id
  route_table_id = aws_route_table.xania.id
}

resource "aws_network_acl" "xania" {
  vpc_id = aws_vpc.xania.id
  egress {
    action = "allow"
    from_port = 0
    protocol = "all"
    rule_no = 100
    to_port = 0
    cidr_block = "0.0.0.0/0"
  }
  ingress {
    action = "allow"
    from_port = 0
    protocol = "all"
    rule_no = 100
    to_port = 0
    cidr_block = "0.0.0.0/0"
  }
}

resource "aws_security_group" "xania" {
  vpc_id = aws_vpc.xania.id
  name = "XaniaSecGroup"
  description = "Security for Xania"
}

resource "aws_security_group_rule" "mosh" {
  security_group_id = aws_security_group.xania.id
  type = "ingress"
  from_port = 60000
  to_port = 61000
  cidr_blocks = [
    "0.0.0.0/0"]
  ipv6_cidr_blocks = [
    "::/0"]
  protocol = "udp"
  description = "Allow MOSH from anywhere"
}

resource "aws_security_group_rule" "ssh" {
  security_group_id = aws_security_group.xania.id
  type = "ingress"
  from_port = 22
  to_port = 22
  cidr_blocks = [
    "0.0.0.0/0"]
  ipv6_cidr_blocks = [
    "::/0"]
  protocol = "tcp"
  description = "Allow SSH from anywhere"
}

resource "aws_security_group_rule" "xania-tcp" {
  security_group_id = aws_security_group.xania.id
  type = "ingress"
  from_port = 9000
  to_port = 9000
  cidr_blocks = [
    "0.0.0.0/0"]
  ipv6_cidr_blocks = [
    "::/0"]
  protocol = "tcp"
  description = "Allow direct access to xania port 9000 from anywhere"
}


resource "aws_security_group_rule" "EgressToAnywhere" {
  security_group_id = aws_security_group.xania.id
  type = "egress"
  from_port = 0
  to_port = 65535
  cidr_blocks = [
    "0.0.0.0/0"]
  ipv6_cidr_blocks = [
    "::/0"]
  protocol = "-1"
  description = "Allow egress to anywhere"
}

resource "aws_iam_instance_profile" "xania" {
  name = "XaniaInstance"
  role = aws_iam_role.xania.name
}

resource "aws_iam_role" "xania" {
  name = "XaniaRole"
  description = "XaniaRole node role"
  assume_role_policy = data.aws_iam_policy_document.InstanceAssumeRolePolicy.json
}

data "aws_iam_policy_document" "xania-backup" {
  statement {
    sid = "S3AccessSid"
    actions = [
      "s3:*"]
    resources = [
      "${aws_s3_bucket.xania.arn}/*",
      aws_s3_bucket.xania.arn
    ]
  }
}

resource "aws_iam_policy" "xania-backup" {
  name = "xania-backup"
  description = "Can read and write the xania s3 backup"
  policy = data.aws_iam_policy_document.xania-backup.json
}

resource "aws_s3_bucket" "xania" {
  bucket = "mud.xania.org"
  acl = "private"
  tags = {
    Site = "mud.xania.org"
  }
  lifecycle_rule {
    id = "backups_to_cold"
    enabled = true
    prefix = "backups"
    transition {
      days = 30
      storage_class = "STANDARD_IA"
    }
  }
  lifecycle_rule {
    id = "releases_to_cold_then_delete"
    enabled = true
    prefix = "releases"
    transition {
      days = 30
      storage_class = "STANDARD_IA"
    }
    expiration {
      days = 120
    }
  }

}

resource "aws_iam_role_policy_attachment" "xania_attach_policy" {
  role = aws_iam_role.xania.name
  policy_arn = aws_iam_policy.xania-backup.arn
}

resource "aws_instance" "XaniaNode" {
  ami = "ami-03bd6c18a631bc24a"
  instance_type = "t3a.small"
  iam_instance_profile = aws_iam_instance_profile.xania.name
  monitoring = false
  key_name = "mattgodbolt"
  subnet_id = aws_subnet.xania-1a.id
  vpc_security_group_ids = [
    aws_security_group.xania.id]
  associate_public_ip_address = true
  source_dest_check = true

  root_block_device {
    volume_type = "gp2"
    volume_size = 24
    delete_on_termination = false
  }

  tags = {
    Name = "Xania MUD"
    Site = "mud.xania.org"
  }
}
