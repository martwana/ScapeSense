variable "project_id" {
  type = string
}

variable "location" {
  description = "Firestore database location (europe-west2 = London)."
  type        = string
  default     = "europe-west2"
}

variable "rules_file" {
  description = "Absolute or root-relative path to firestore.rules, deployed via Firestore Security Rules resources."
  type        = string
}
