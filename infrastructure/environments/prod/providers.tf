provider "aws" {
  region = var.aws_region

  default_tags {
    tags = {
      Project     = "scapesense"
      Environment = "prod"
      ManagedBy   = "terraform"
    }
  }
}

# Google credentials come from Application Default Credentials
# (`gcloud auth application-default login`). The project is created by
# Terraform, so provider blocks do not pin a project.
provider "google" {
  region = var.firebase_region
}

provider "google-beta" {
  region = var.firebase_region
}
