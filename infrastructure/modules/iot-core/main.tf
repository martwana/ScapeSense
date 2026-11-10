# AWS IoT Core infrastructure shared by every ScapeSense device.
#
# Individual manufactured devices are NOT Terraform resources — they join via
# Fleet Provisioning (see iot-provisioning) and are tracked in Firestore.

data "aws_caller_identity" "current" {}
data "aws_region" "current" {}
data "aws_partition" "current" {}

locals {
  iot_arn = "arn:${data.aws_partition.current.partition}:iot:${data.aws_region.current.region}:${data.aws_caller_identity.current.account_id}"
  thing   = "$${iot:Connection.Thing.ThingName}"
}

resource "aws_iot_thing_type" "scapesense" {
  name = var.thing_type_name

  properties {
    searchable_attributes = ["model", "hwRevision", "manufacturedAt"]
  }
}

resource "aws_iot_thing_group" "devices" {
  name = var.thing_group_name

  properties {
    attribute_payload {
      attributes = { product = "scapesense" }
    }
  }
}

# Least-privilege device policy: a Thing may only connect as itself, publish to
# its own telemetry/events/responses and shadow topics, and read its own
# commands/shadow/jobs topics. IoT policy variables scope every ARN to the
# attached Thing.
resource "aws_iot_policy" "device" {
  name = var.policy_name

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect    = "Allow"
        Action    = "iot:Connect"
        Resource  = "${local.iot_arn}:client/${local.thing}"
        Condition = { Bool = { "iot:Connection.Thing.IsAttached" = "true" } }
      },
      {
        Effect = "Allow"
        Action = "iot:Publish"
        Resource = concat(
          [for s in ["telemetry", "events", "responses"] :
          "${local.iot_arn}:topic/scapesense/${local.thing}/${s}"],
          ["${local.iot_arn}:topic/$aws/things/${local.thing}/shadow/*",
          "${local.iot_arn}:topic/$aws/things/${local.thing}/jobs/*"]
        )
      },
      {
        Effect = "Allow"
        Action = "iot:Subscribe"
        Resource = [
          "${local.iot_arn}:topicfilter/scapesense/${local.thing}/commands",
          "${local.iot_arn}:topicfilter/$aws/things/${local.thing}/shadow/*",
          "${local.iot_arn}:topicfilter/$aws/things/${local.thing}/jobs/*",
        ]
      },
      {
        Effect = "Allow"
        Action = "iot:Receive"
        Resource = [
          "${local.iot_arn}:topic/scapesense/${local.thing}/commands",
          "${local.iot_arn}:topic/$aws/things/${local.thing}/shadow/*",
          "${local.iot_arn}:topic/$aws/things/${local.thing}/jobs/*",
        ]
      },
    ]
  })
}

# IoT → CloudWatch logging (connection/auth errors, rule failures).
resource "aws_cloudwatch_log_group" "iot" {
  name              = "/aws/iot/scapesense"
  retention_in_days = var.log_retention_days
}

resource "aws_iam_role" "iot_logging" {
  name = "scapesense-iot-logging"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "iot.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_iam_role_policy" "iot_logging" {
  name = "scapesense-iot-logging"
  role = aws_iam_role.iot_logging.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect = "Allow"
      Action = [
        "logs:CreateLogStream",
        "logs:DescribeLogStreams",
        "logs:PutLogEvents",
        "logs:PutRetentionPolicy",
      ]
      Resource = "${aws_cloudwatch_log_group.iot.arn}:*"
    }]
  })
}

resource "aws_iot_logging_options" "main" {
  default_log_level = "INFO"
  role_arn          = aws_iam_role.iot_logging.arn
}
