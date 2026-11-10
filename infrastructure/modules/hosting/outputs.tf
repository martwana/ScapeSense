output "site_id" {
  value = google_firebase_hosting_site.portal.site_id
}

output "default_url" {
  value = "https://${google_firebase_hosting_site.portal.site_id}.web.app"
}

output "required_dns_updates" {
  description = "DNS records Firebase requires for the custom domain (empty until custom_domain_enabled applies)."
  value       = var.custom_domain_enabled ? google_firebase_hosting_custom_domain.app[0].required_dns_updates : []
}
