variable "job_template_id" {
  description = "Job template used for signed OTA rollouts."
  type        = string
  default     = "scapesense-ota-update"
}

variable "firmware_bucket_name" {
  description = "S3 bucket holding release manifests and firmware (from storage module)."
  type        = string
}

variable "aws_region" {
  description = "Region of the firmware bucket (for the presigned URL template)."
  type        = string
  default     = "eu-west-2"
}
