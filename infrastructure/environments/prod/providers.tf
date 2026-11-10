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
# (`gcloud auth application-default login` plus
# `gcloud auth application-default set-quota-project <project>`). User ADC
# credentials require a quota/billing project header on APIs like
# firebasedatabase.googleapis.com, so the providers send x-goog-user-project.
provider "google" {
  region                = var.firebase_region
  billing_project       = var.firebase_project_id
  user_project_override = true
}

provider "google-beta" {
  region                = var.firebase_region
  billing_project       = var.firebase_project_id
  user_project_override = true
}
