variable "template_name" {
  description = "AWS IoT fleet provisioning template name."
  type        = string
  default     = "scapesense-fleet-provisioning"
}

variable "device_policy_name" {
  description = "IoT policy attached to provisioned device certificates (from iot-core)."
  type        = string
}

variable "thing_type_name" {
  description = "Thing Type assigned to provisioned devices (from iot-core)."
  type        = string
}

variable "thing_group_name" {
  description = "Thing Group assigned to provisioned devices (from iot-core)."
  type        = string
}

variable "claim_policy_name" {
  description = "Policy attached to short-lived fleet-provisioning claim certificates."
  type        = string
  default     = "scapesense-provisioning-claim-policy"
}

variable "pre_provisioning_hook_lambda_arn" {
  description = "Optional Lambda ARN invoked to validate the manufacturing identity before provisioning (Phase 4)."
  type        = string
  default     = null
}
