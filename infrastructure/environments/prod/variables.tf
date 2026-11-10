variable "aws_region" {
  description = "AWS region for IoT/Lambda/S3 resources."
  type        = string
  default     = "eu-west-2"
}

variable "domain_name" {
  description = "Public apex domain managed in the existing Cloudflare zone."
  type        = string
  default     = "scapesense.io"
}

variable "cloudflare_api_token" {
  description = "Scoped Cloudflare API token (Edit zone DNS on scapesense.io). Supply via terraform.tfvars or CLOUDFLARE_API_TOKEN env var."
  type        = string
  sensitive   = true
  default     = null
}

variable "firebase_project_id" {
  description = "Globally unique GCP/Firebase project ID (e.g. scapesense-prod-1234)."
  type        = string
}

variable "gcp_billing_account_id" {
  description = "GCP billing account ID (XXXXXX-XXXXXX-XXXXXX). Required — Functions need Blaze."
  type        = string
}

variable "firebase_region" {
  description = "Primary GCP region (Firestore + Functions region is europe-west2)."
  type        = string
  default     = "europe-west2"
}

variable "rtdb_region" {
  description = "Realtime Database region — europe-west2 is NOT available; use europe-west1."
  type        = string
  default     = "europe-west1"
}

variable "hosting_site_id" {
  description = "Firebase Hosting site ID (<site_id>.web.app)."
  type        = string
  default     = "scapesense-app"
}

variable "firmware_bucket_name" {
  description = "Globally unique S3 bucket for signed firmware artifacts."
  type        = string
}

variable "alert_emails" {
  description = "Addresses receiving AWS + GCP budget alerts."
  type        = list(string)
}

variable "aws_monthly_budget_usd" {
  type    = number
  default = 10
}

variable "gcp_monthly_budget_gbp" {
  type    = number
  default = 5
}

variable "custom_domain_enabled" {
  description = "Stage B: create the Firebase custom domain. Cloudflare DNS is already authoritative, so this can be enabled immediately."
  type        = bool
  default     = false
}

variable "app_dns_records" {
  description = "Stage C: DNS records Firebase requires for app.scapesense.io — copy each entry from `terraform output firebase_required_dns_updates`."
  type = list(object({
    name  = string
    type  = string
    rdata = string
  }))
  default = []
}
