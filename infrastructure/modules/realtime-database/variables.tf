variable "project_id" {
  type = string
}

variable "instance_id" {
  description = "RTDB instance ID (lowercase; becomes <id>.<region>.firebasedatabase.app)."
  type        = string
}

variable "region" {
  description = "Realtime Database region. RTDB does NOT support europe-west2 — valid choices include europe-west1, us-central1, asia-southeast1."
  type        = string
  default     = "europe-west1"
}
