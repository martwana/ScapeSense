output "project_id" {
  value = google_project.this.project_id
}

output "project_number" {
  description = "Used for IAM principals, budget filters and Firebase config."
  value       = google_project.this.number
}

output "web_app_id" {
  description = "Firebase web app ID — part of the browser firebaseConfig."
  value       = google_firebase_web_app.portal.app_id
}

# Downstream modules must take `apis_ready` as their project_id input so
# Terraform only provisions them after every required API is enabled —
# otherwise Identity Platform/Firestore resources race API enablement.
output "apis_ready" {
  description = "Project ID, resolved only after firebase_project + all APIs."
  value       = google_firebase_project.this.project
}

output "web_app_name" {
  value = google_firebase_web_app.portal.name
}
