# ScapeSense prod environment — the only cloud environment for now.
# Adding a dev environment later = a sibling directory reusing these modules
# with a different state key + variables.

locals {
  app_domain = "app.${var.domain_name}"
  authorized_domains = [
    "localhost",
    var.firebase_project_id == "" ? "" : "${var.firebase_project_id}.web.app",
    var.firebase_project_id == "" ? "" : "${var.firebase_project_id}.firebaseapp.com",
    local.app_domain,
  ]
}

# ---------- DNS (AWS) ----------

module "route53" {
  source      = "../../modules/route53"
  domain_name = var.domain_name
}

# ---------- Firebase / GCP ----------

module "firebase" {
  source             = "../../modules/firebase"
  project_id         = var.firebase_project_id
  project_name       = "ScapeSense"
  billing_account_id = var.gcp_billing_account_id
}

module "iam" {
  source     = "../../modules/iam"
  project_id = module.firebase.apis_ready
}

module "firestore" {
  source     = "../../modules/firestore"
  project_id = module.firebase.apis_ready
  location   = var.firebase_region
  rules_file = "${path.module}/../../../Firebase/firestore.rules"
}

module "realtime_database" {
  source      = "../../modules/realtime-database"
  project_id  = module.firebase.apis_ready
  instance_id = "${var.firebase_project_id}-default-rtdb"
  region      = var.rtdb_region
}

module "authentication" {
  source             = "../../modules/authentication"
  project_id         = module.firebase.apis_ready
  authorized_domains = local.authorized_domains
}

module "hosting" {
  source                = "../../modules/hosting"
  project_id            = module.firebase.apis_ready
  site_id               = var.hosting_site_id
  custom_domain         = local.app_domain
  custom_domain_enabled = var.custom_domain_enabled
}

# ---------- AWS IoT ----------

module "iot_core" {
  source = "../../modules/iot-core"
}

module "iot_provisioning" {
  source             = "../../modules/iot-provisioning"
  device_policy_name = module.iot_core.policy_name
  thing_type_name    = module.iot_core.thing_type_name
  thing_group_name   = module.iot_core.thing_group_name
}

module "storage" {
  source               = "../../modules/storage"
  firmware_bucket_name = var.firmware_bucket_name
}

module "iot_jobs" {
  source               = "../../modules/iot-jobs"
  firmware_bucket_name = module.storage.firmware_bucket_name
  aws_region           = var.aws_region
}

module "iot_bridge" {
  source                = "../../modules/lambda"
  firebase_project_id   = module.firebase.project_id
  firebase_api_base_url = var.custom_domain_enabled ? "https://${local.app_domain}/api" : ""
}

# ---------- Monitoring ----------

module "monitoring" {
  source                 = "../../modules/monitoring"
  project_id             = module.firebase.apis_ready
  project_number         = module.firebase.project_number
  billing_account_id     = var.gcp_billing_account_id
  alert_emails           = var.alert_emails
  aws_monthly_budget_usd = var.aws_monthly_budget_usd
  gcp_monthly_budget_gbp = var.gcp_monthly_budget_gbp
}

# ---------- Custom-domain DNS records (stage C) ----------
#
# After the custom domain exists, Firebase reports the exact records it needs
# (site-verification TXT + hosting A/AAAA or CNAME) via
#   terraform output firebase_required_dns_updates
# Resource attributes can't feed for_each at plan time, so copy those actual
# values into `app_dns_records` in terraform.tfvars and re-apply — the records
# still come from Firebase, nothing is hardcoded here.

resource "aws_route53_record" "firebase_custom_domain" {
  for_each = { for r in var.app_dns_records : "${r.name}|${r.type}|${r.rdata}" => r }

  zone_id = module.route53.zone_id
  name    = trimsuffix(each.value.name, ".")
  type    = each.value.type
  ttl     = 300
  records = [each.value.rdata]
}
