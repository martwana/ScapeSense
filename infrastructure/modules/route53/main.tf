resource "aws_route53_zone" "main" {
  name    = var.domain_name
  comment = "ScapeSense public zone. Registrar delegation is a manual step: set these nameservers at the domain registrar."
}
