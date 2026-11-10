# Cloudflare DNS for scapesense.io.
#
# The domain is registered at Cloudflare, so the zone already exists in the
# account and its nameservers are already authoritative — Terraform does NOT
# create the zone, it only manages records inside it (see the
# cloudflare_dns_record resources in the environment).

data "cloudflare_zone" "main" {
  filter = {
    name = var.domain_name
  }
}
