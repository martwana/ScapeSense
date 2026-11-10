variable "aws_region" {
  description = "AWS region hosting the Terraform state bucket."
  type        = string
  default     = "eu-west-2"
}

variable "state_bucket_prefix" {
  description = "Prefix for the Terraform state bucket; the AWS account ID is appended for global uniqueness."
  type        = string
  default     = "scapesense-terraform-state"
}
