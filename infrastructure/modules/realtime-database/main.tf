# Firebase Realtime Database — carries live device metrics for portal
# subscriptions. Server-side writes only; client reads gated by RTDB rules
# (deployed via `firebase deploy --only database`, like firestore.rules).

resource "google_firebase_database_instance" "default" {
  provider    = google-beta
  project     = var.project_id
  region      = var.region
  instance_id = var.instance_id
  type        = "DEFAULT_DATABASE"
}
