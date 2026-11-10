variable "thing_type_name" {
  description = "AWS IoT Thing Type shared by all ScapeSense units."
  type        = string
  default     = "scapesense-device"
}

variable "thing_group_name" {
  description = "AWS IoT Thing Group containing all ScapeSense units."
  type        = string
  default     = "scapesense-devices"
}

variable "policy_name" {
  description = "Name of the least-privilege per-device IoT policy."
  type        = string
  default     = "scapesense-device-policy"
}

variable "log_retention_days" {
  description = "CloudWatch retention for AWS IoT logs."
  type        = number
  default     = 30
}
