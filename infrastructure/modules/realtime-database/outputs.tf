output "database_url" {
  description = "RTDB URL used by portal subscriptions and the AWS bridge."
  value       = "https://${var.instance_id}.${var.region}.firebasedatabase.app"
}

output "instance_id" {
  value = google_firebase_database_instance.default.name
}
