output "functions_service_account_email" {
  description = "Set as the Cloud Functions runtime service account during deployment."
  value       = google_service_account.functions.email
}
