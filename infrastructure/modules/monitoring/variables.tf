variable "project_id" {
  description = "GCP project for the Firebase-side budget."
  type        = string
}

variable "project_number" {
  description = "GCP project number (budget filters use projects/<number>)."
  type        = string
}

variable "billing_account_id" {
  description = "GCP billing account to attach the budget to."
  type        = string
}

variable "alert_emails" {
  description = "Email addresses receiving AWS/GCP budget alerts."
  type        = list(string)
}

variable "aws_monthly_budget_usd" {
  description = "AWS cost budget per month. Alerts only — not a spending cap."
  type        = number
  default     = 10
}

variable "gcp_monthly_budget_gbp" {
  description = "GCP/Firebase cost budget per month. Alerts only — not a spending cap."
  type        = number
  default     = 5
}
