output "job_template_id" {
  value = var.job_template_id
}

output "job_document" {
  description = "OTA job document with the presigned-manifest-URL substitution AWS performs per job."
  value       = local.job_document
}

output "create_job_template_command" {
  description = "Run once after apply to register the job template (provider lacks a resource for it)."
  value       = local.create_command
}
