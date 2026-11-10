# AWS IoT Fleet Provisioning ("by claim"): factory flashed claim certificates
# are short-lived and only allowed to call the provisioning APIs. The template
# mints each device's real operational certificate and creates its Thing.

data "aws_caller_identity" "current" {}
data "aws_region" "current" {}
data "aws_partition" "current" {}

locals {
  iot_arn = "arn:${data.aws_partition.current.partition}:iot:${data.aws_region.current.region}:${data.aws_caller_identity.current.account_id}"
}

# Claim certificates can only reach the certificate-creation and provisioning
# topics — they cannot publish telemetry or touch shadows.
resource "aws_iot_policy" "provisioning_claim" {
  name = var.claim_policy_name

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = "iot:Connect"
        Resource = "${local.iot_arn}:client/scapesense-prov-*"
      },
      {
        Effect = "Allow"
        Action = ["iot:Publish", "iot:Receive"]
        Resource = [
          "${local.iot_arn}:topic/$aws/certificates/create/*",
          "${local.iot_arn}:topic/$aws/provisioning-templates/${var.template_name}/provision/*",
        ]
      },
      {
        Effect = "Allow"
        Action = "iot:Subscribe"
        Resource = [
          "${local.iot_arn}:topicfilter/$aws/certificates/create/*",
          "${local.iot_arn}:topicfilter/$aws/provisioning-templates/${var.template_name}/provision/*",
        ]
      },
    ]
  })
}

resource "aws_iam_role" "provisioning" {
  name = "scapesense-iot-provisioning"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "iot.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_iam_role_policy" "provisioning" {
  name = "scapesense-iot-provisioning"
  role = aws_iam_role.provisioning.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect = "Allow"
      Action = [
        "iot:AttachPrincipal",
        "iot:AttachPolicy",
        "iot:AttachThingPrincipal",
        "iot:CreateCertificateAndAttachToThing",
        "iot:CreateThing",
        "iot:DescribeThing",
        "iot:UpdateThing",
        "iot:AddThingToThingGroup",
        "iot:UpdateCertificate",
        "iot:RegisterThing",
      ]
      Resource = [
        "${local.iot_arn}:thing/scapesense-*",
        "${local.iot_arn}:cert/*",
        "${local.iot_arn}:policy/*",
        "${local.iot_arn}:thinggroup/${var.thing_group_name}",
      ]
    }]
  })
}

resource "aws_iot_provisioning_template" "scapesense" {
  name                  = var.template_name
  description           = "ScapeSense fleet provisioning: SerialNumber parameter is the eFuse-backed manufacturing identity."
  provisioning_role_arn = aws_iam_role.provisioning.arn
  enabled               = true
  type                  = "FLEET_PROVISIONING"

  # When set (Phase 4), the hook validates SerialNumber against the
  # manufacturing registry before a certificate is issued.
  dynamic "pre_provisioning_hook" {
    for_each = var.pre_provisioning_hook_lambda_arn == null ? [] : [1]
    content {
      target_arn      = var.pre_provisioning_hook_lambda_arn
      payload_version = "2020-04-01"
    }
  }

  template_body = jsonencode({
    Parameters = {
      SerialNumber                = { Type = "String" }
      "AWS::IoT::Certificate::Id" = { Type = "String" }
    }
    Resources = {
      thing = {
        Type = "AWS::IoT::Thing"
        Properties = {
          ThingName     = { Ref = "SerialNumber" }
          ThingTypeName = var.thing_type_name
          ThingGroups   = [var.thing_group_name]
          AttributePayload = {
            product = "scapesense"
          }
        }
        OverrideSettings = {
          AttributePayload = "MERGE"
          ThingTypeName    = "REPLACE"
          ThingGroups      = "DO_NOTHING"
        }
      }
      certificate = {
        Type = "AWS::IoT::Certificate"
        Properties = {
          CertificateId = { Ref = "AWS::IoT::Certificate::Id" }
          Status        = "Active"
        }
      }
      policy = {
        Type = "AWS::IoT::Policy"
        Properties = {
          PolicyName = var.device_policy_name
        }
      }
    }
  })
}
