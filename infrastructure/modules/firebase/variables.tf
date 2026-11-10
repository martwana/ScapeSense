variable "project_id" {
  description = "Globally unique GCP/Firebase project ID (e.g. scapesense-prod-1234)."
  type        = string
}

variable "project_name" {
  description = "Display name for the project."
  type        = string
  default     = "ScapeSense"
}

variable "billing_account_id" {
  description = "GCP billing account ID (XXXXXX-XXXXXX-XXXXXX). Blaze plan is required for Cloud Functions."
  type        = string
}

variable "org_id" {
  description = "Optional GCP organisation ID the project is created under."
  type        = string
  default     = null
}
