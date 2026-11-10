# Firestore: (default) Native database in europe-west2, the expiresAt TTL
# override from Firebase/firestore.indexes.json, and the deny-all client rules
# deployed through Firestore Security Rules resources (equivalent to
# `firebase deploy --only firestore:rules`).

resource "google_firestore_database" "default" {
  project                           = var.project_id
  name                              = "(default)"
  location_id                       = var.location
  type                              = "FIRESTORE_NATIVE"
  concurrency_mode                  = "OPTIMISTIC"
  app_engine_integration_mode       = "DISABLED"
  point_in_time_recovery_enablement = "POINT_IN_TIME_RECOVERY_DISABLED"

  # Prevents `terraform destroy` wiping event/device data.
  delete_protection_state = "DELETE_PROTECTION_ENABLED"
}

# TTL on devices/{deviceId}/events/{eventId}.expiresAt — mirrors
# fieldOverrides in Firebase/firestore.indexes.json.
resource "google_firestore_field" "events_expires_at_ttl" {
  project    = var.project_id
  database   = google_firestore_database.default.name
  collection = "events"
  field      = "expiresAt"

  index_config {}
  ttl_config {}
}

resource "google_firebaserules_ruleset" "firestore" {
  project = var.project_id

  source {
    files {
      name    = "firestore.rules"
      content = file(var.rules_file)
    }
  }
}

resource "google_firebaserules_release" "firestore" {
  name         = "cloud.firestore"
  ruleset_name = google_firebaserules_ruleset.firestore.name
  project      = var.project_id
}
