variable "project_id" {
  type = string
}

variable "site_id" {
  description = "Firebase Hosting site ID for the portal (produces <site_id>.web.app)."
  type        = string
  default     = "scapesense-app"
}

variable "custom_domain" {
  description = "Customer portal domain (e.g. app.scapesense.io)."
  type        = string
}

variable "custom_domain_enabled" {
  description = "Create the Firebase custom-domain resource. Enable only AFTER registrar delegation to Route 53 nameservers."
  type        = bool
  default     = false
}
