# Bootstrap: creates the remote Terraform state backend.
#
# This stack is applied ONCE with local state (no backend block on purpose).
# Afterwards, copy the printed backend values into
# environments/prod/backend.hcl and run `terraform init` there.
#
# Uses S3 native state locking (use_lockfile = true), which needs
# Terraform >= 1.10 and removes the need for a DynamoDB lock table.

provider "aws" {
  region = var.aws_region

  default_tags {
    tags = {
      Project   = "scapesense"
      ManagedBy = "terraform"
      Stack     = "bootstrap"
    }
  }
}

data "aws_caller_identity" "current" {}

resource "aws_s3_bucket" "state" {
  bucket = "${var.state_bucket_prefix}-${data.aws_caller_identity.current.account_id}"

  # Never let `terraform destroy` remove every environment's state.
  lifecycle {
    prevent_destroy = true
  }
}

resource "aws_s3_bucket_versioning" "state" {
  bucket = aws_s3_bucket.state.id
  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "state" {
  bucket = aws_s3_bucket.state.id
  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_public_access_block" "state" {
  bucket                  = aws_s3_bucket.state.id
  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}
