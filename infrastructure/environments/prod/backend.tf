# Remote state: encrypted + versioned S3 bucket with native S3 locking
# (use_lockfile replaces the old DynamoDB lock table; needs Terraform >= 1.10).
#
# Values live in backend.hcl (gitignored — copy backend.hcl.example first):
#   terraform init -backend-config=backend.hcl
terraform {
  backend "s3" {
    key          = "scapesense/prod/terraform.tfstate"
    encrypt      = true
    use_lockfile = true
  }
}
