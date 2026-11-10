output "template_name" {
  description = "Fleet provisioning template devices call to mint their operational certificate."
  value       = aws_iot_provisioning_template.scapesense.name
}

output "claim_policy_name" {
  description = "Attach short-lived provisioning claim certificates to this policy."
  value       = aws_iot_policy.provisioning_claim.name
}

output "role_arn" {
  value = aws_iam_role.provisioning.arn
}
