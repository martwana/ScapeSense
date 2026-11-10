output "zone_id" {
  description = "Cloudflare zone ID for scapesense.io."
  value       = data.cloudflare_zone.main.zone_id
}

output "name_servers" {
  description = "Cloudflare-assigned nameservers — already authoritative because the domain is registered at Cloudflare. No registrar step needed."
  value       = data.cloudflare_zone.main.name_servers
}
