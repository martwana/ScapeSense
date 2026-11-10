output "state_bucket_name" {
  description = "S3 bucket to use in environments/*/backend.hcl."
  value       = aws_s3_bucket.state.bucket
}

output "aws_region" {
  description = "Region of the state bucket."
  value       = var.aws_region
}
