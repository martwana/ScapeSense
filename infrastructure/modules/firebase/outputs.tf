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

output "web_app_name" {
  value = google_firebase_web_app.portal.name
}
