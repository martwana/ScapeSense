# AWS IoT Jobs OTA template.
#
# The AWS Terraform provider has no job-template resource; job templates are
# created with `aws iot create-job-template`. This module renders the job
# document (with the firmware bucket baked in) and outputs it plus the CLI
# command — run it once after apply, and again if the bucket name changes.

# IoT Jobs assumes this role to presign firmware S3 URLs for devices.
data "aws_caller_identity" "current" {}
data "aws_partition" "current" {}

resource "aws_iam_role" "jobs_presign" {
  name = "scapesense-iot-jobs-presign"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "iot.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_iam_role_policy" "jobs_presign" {
  name = "scapesense-iot-jobs-presign"
  role = aws_iam_role.jobs_presign.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect   = "Allow"
      Action   = "s3:GetObject"
      Resource = "arn:${data.aws_partition.current.partition}:s3:::${var.firmware_bucket_name}/*"
    }]
  })
}

locals {
  job_document = jsonencode({
    operation = "ota-update"
    # AWS substitutes the presigned URL inside job documents at execution
    # time. The manifest path points at the release manifest chosen when each
    # job is created (operator updates `latest.json` per rollout).
    manifestUrl = "$${aws:iot:s3-presigned-url:https://s3.${var.aws_region}.amazonaws.com/${var.firmware_bucket_name}/manifests/latest.json}"
  })

  create_command = join(" ", [
    "aws iot create-job-template",
    "--job-template-id ${var.job_template_id}",
    "--region ${var.aws_region}",
    "--description 'ScapeSense signed OTA update'",
    "--document '${local.job_document}'",
    "--abort-config 'criteriaList=[{action=CANCEL,failureType=FAILED,minNumberOfExecutedThings=1,thresholdPercentage=100}]'",
    "--timeout-config 'inProgressTimeoutInMinutes=30'",
    "--presigned-url-config 'roleArn=${aws_iam_role.jobs_presign.arn},expiresInSec=3600'",
  ])
}
