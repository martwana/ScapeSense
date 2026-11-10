variable "project_id" {
  type = string
}

variable "authorized_domains" {
  description = "Firebase Auth authorized domains: localhost, the project's web.app/firebaseapp.com names and app.scapesense.io."
  type        = list(string)
}
