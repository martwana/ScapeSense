variable "function_name" {
  description = "Lambda bridging AWS IoT messages into Firebase."
  type        = string
  default     = "scapesense-iot-bridge"
}

variable "firebase_project_id" {
  description = "Firebase/GCP project the bridge writes to (passed as an env var)."
  type        = string
}

variable "firebase_api_base_url" {
  description = "Authenticated integration endpoint the bridge posts events to (Phase 3; e.g. https://app.scapesense.io/api)."
  type        = string
  default     = ""
}

variable "log_retention_days" {
  type    = number
  default = 30
}
