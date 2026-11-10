# GCP project + Firebase attachment + required APIs.
#
# The project is created here rather than in the Firebase console so the whole
# stack is reproducible. Blaze billing is attached via billing_account_id —
# required for Cloud Functions.

resource "google_project" "this" {
  name            = var.project_name
  project_id      = var.project_id
  billing_account = var.billing_account_id
  org_id          = var.org_id

  labels = {
    product     = "scapesense"
    environment = "prod"
    managed_by  = "terraform"
  }

  # Keep PREVENT: `terraform destroy` cannot remove the project, which also
  # protects the Firestore data. Change deliberately if ever tearing down.
  deletion_policy = "PREVENT"
}

locals {
  apis = [
    "serviceusage.googleapis.com",
    "cloudresourcemanager.googleapis.com",
    "cloudbilling.googleapis.com",
    "iam.googleapis.com",
    "firebase.googleapis.com",
    "firebasehosting.googleapis.com",
    "firebaserules.googleapis.com",
    "firestore.googleapis.com",
    "firebasedatabase.googleapis.com",
    "identitytoolkit.googleapis.com",
    "securetoken.googleapis.com",
    "fcm.googleapis.com",
    # Cloud Functions v2 rides on Cloud Run + Eventarc + Cloud Build + AR.
    "cloudfunctions.googleapis.com",
    "run.googleapis.com",
    "eventarc.googleapis.com",
    "cloudbuild.googleapis.com",
    "artifactregistry.googleapis.com",
    "secretmanager.googleapis.com",
  ]
}

resource "google_project_service" "apis" {
  for_each           = toset(local.apis)
  project            = google_project.this.project_id
  service            = each.value
  disable_on_destroy = false
}

resource "google_firebase_project" "this" {
  provider = google-beta
  project  = google_project.this.project_id

  depends_on = [google_project_service.apis]
}

resource "google_firebase_web_app" "portal" {
  provider     = google-beta
  project      = google_project.this.project_id
  display_name = "ScapeSense Portal"

  depends_on = [google_firebase_project.this]
}
