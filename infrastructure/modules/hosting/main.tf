# Firebase Hosting site for the ScapeSense PWA. The custom domain resource is
# created after registrar delegation; its `required_dns_updates` output tells
# us exactly which Route 53 records Firebase wants — nothing is hardcoded.

resource "google_firebase_hosting_site" "portal" {
  provider = google-beta
  project  = var.project_id
  site_id  = var.site_id
}

resource "google_firebase_hosting_custom_domain" "app" {
  provider = google-beta
  count    = var.custom_domain_enabled ? 1 : 0

  project       = var.project_id
  site_id       = google_firebase_hosting_site.portal.site_id
  custom_domain = var.custom_domain

  # DNS records are created in a later apply from required_dns_updates;
  # verification can't complete before they exist.
  wait_dns_verification = false
}
