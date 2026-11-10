output "cloudflare_zone_id" {
  value = module.cloudflare.zone_id
}

output "cloudflare_name_servers" {
  description = "Cloudflare-assigned nameservers — already authoritative (domain registered at Cloudflare), no registrar step."
  value       = module.cloudflare.name_servers
}

output "firebase_project_id" {
  value = module.firebase.project_id
}

output "firebase_project_number" {
  value = module.firebase.project_number
}

output "firebase_web_app_id" {
  description = "Firebase web app ID for the portal's firebaseConfig."
  value       = module.firebase.web_app_id
}

output "hosting_default_url" {
  value = module.hosting.default_url
}

output "rtdb_url" {
  value = module.realtime_database.database_url
}

output "functions_service_account" {
  description = "Deploy Cloud Functions with this runtime service account."
  value       = module.iam.functions_service_account_email
}

output "iot_data_endpoint" {
  description = "ESP32 MQTT/TLS endpoint (port 8883)."
  value       = module.iot_core.iot_data_endpoint
}

output "iot_device_policy" {
  value = module.iot_core.policy_name
}

output "provisioning_template" {
  value = module.iot_provisioning.template_name
}

output "claim_policy" {
  value = module.iot_provisioning.claim_policy_name
}

output "firmware_bucket" {
  value = module.storage.firmware_bucket_name
}

output "ota_job_template" {
  value = module.iot_jobs.job_template_id
}

output "create_job_template_command" {
  description = "Run once post-apply to register the AWS IoT Jobs OTA template."
  value       = module.iot_jobs.create_job_template_command
}

output "bridge_secret_arn" {
  description = "Phase 3: store the Firebase integration credential here."
  value       = module.iot_bridge.integration_secret_arn
}

output "firebase_required_dns_updates" {
  description = "After custom_domain_enabled applies, the exact DNS records Firebase wants for app.scapesense.io."
  value       = module.hosting.required_dns_updates
}
