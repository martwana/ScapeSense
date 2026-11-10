output "policy_name" {
  description = "Least-privilege policy attached to every device certificate."
  value       = aws_iot_policy.device.name
}

output "thing_type_name" {
  value = aws_iot_thing_type.scapesense.name
}

output "thing_group_name" {
  value = aws_iot_thing_group.devices.name
}

# `aws_iot_endpoint` is deprecated but still provides the ATS data endpoint
# needed by firmware. If removed in a future provider release, retrieve it with:
#   aws iot describe-endpoint --endpoint-type iot:Data-ATS
data "aws_iot_endpoint" "ats" {
  endpoint_type = "iot:Data-ATS"
}

output "iot_data_endpoint" {
  description = "MQTT/TLS endpoint devices connect to (port 8883)."
  value       = data.aws_iot_endpoint.ats.endpoint_address
}
