# GCP-side IAM. Dedicated runtime service account for Cloud Functions instead
# of the default compute SA, with least-privilege bindings.

resource "google_service_account" "functions" {
  project      = var.project_id
  account_id   = "scapesense-functions"
  display_name = "ScapeSense Functions runtime"
  description  = "Runtime identity for the ScapeSense API and notification functions."
}

resource "google_project_iam_member" "functions_firestore" {
  project = var.project_id
  role    = "roles/datastore.user"
  member  = "serviceAccount:${google_service_account.functions.email}"
}

resource "google_project_iam_member" "functions_auth" {
  project = var.project_id
  role    = "roles/firebaseauth.admin"
  member  = "serviceAccount:${google_service_account.functions.email}"
}

resource "google_project_iam_member" "functions_rtdb" {
  project = var.project_id
  role    = "roles/firebasedatabase.admin"
  member  = "serviceAccount:${google_service_account.functions.email}"
}

# FCM send via the Admin SDK uses the project's firebase.managementServiceAgent-
# style roles; if sendEachForMulticast returns auth errors at deploy time,
# additionally bind roles/firebasenotifications.admin to this account.
resource "google_project_iam_member" "functions_fcm" {
  project = var.project_id
  role    = "roles/firebasenotifications.admin"
  member  = "serviceAccount:${google_service_account.functions.email}"
}
