# AWS IoT -> Firebase bridge.
#
# src/index.mjs is currently a validating stub. Phase 3 replaces it with the
# real integration (telemetry -> Realtime Database, events -> Firestore/FCM)
# using short-lived credentials from the Secret Manager secret below —
# never static keys committed to source.

data "archive_file" "bridge" {
  type        = "zip"
  source_dir  = "${path.module}/src"
  output_path = "${path.module}/build/bridge.zip"
}

resource "aws_cloudwatch_log_group" "bridge" {
  name              = "/aws/lambda/${var.function_name}"
  retention_in_days = var.log_retention_days
}

resource "aws_iam_role" "bridge" {
  name = "${var.function_name}-exec"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "lambda.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_iam_role_policy" "bridge_logs" {
  name = "logs"
  role = aws_iam_role.bridge.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect   = "Allow"
      Action   = ["logs:CreateLogStream", "logs:PutLogEvents"]
      Resource = "${aws_cloudwatch_log_group.bridge.arn}:*"
    }]
  })
}

resource "aws_iam_role_policy" "bridge_secret" {
  name = "firebase-secret-read"
  role = aws_iam_role.bridge.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect   = "Allow"
      Action   = ["secretsmanager:GetSecretValue"]
      Resource = aws_secretsmanager_secret.firebase_integration.arn
    }]
  })
}

# Populated during Phase 3 setup with the integration credential (e.g. a GCP
# workload-identity-federation config or a signed-JWT service-account document).
# Created empty on purpose — a placeholder string would end up in state anyway,
# so the operator writes the real value with the AWS CLI/console.
resource "aws_secretsmanager_secret" "firebase_integration" {
  name        = "scapesense/firebase-integration"
  description = "Credential material for Lambda -> Firebase API writes. Set during Phase 3; never commit."
}

resource "aws_lambda_function" "bridge" {
  function_name    = var.function_name
  role             = aws_iam_role.bridge.arn
  runtime          = "nodejs20.x"
  handler          = "index.handler"
  filename         = data.archive_file.bridge.output_path
  source_code_hash = data.archive_file.bridge.output_base64sha256
  timeout          = 15
  memory_size      = 128

  environment {
    variables = {
      FIREBASE_PROJECT_ID   = var.firebase_project_id
      FIREBASE_API_BASE_URL = var.firebase_api_base_url
      FIREBASE_SECRET_ARN   = aws_secretsmanager_secret.firebase_integration.arn
    }
  }

  depends_on = [aws_cloudwatch_log_group.bridge]
}

resource "aws_iot_topic_rule" "telemetry" {
  name        = "scapesense_telemetry"
  description = "Device telemetry snapshots -> Firebase Realtime Database"
  enabled     = true
  sql         = "SELECT *, topic(2) AS deviceId, timestamp() AS receivedAt FROM 'scapesense/+/telemetry'"
  sql_version = "2016-03-23"

  lambda {
    function_arn = aws_lambda_function.bridge.arn
  }
}

resource "aws_iot_topic_rule" "events" {
  name        = "scapesense_events"
  description = "Device alert/event transitions -> Firestore + FCM"
  enabled     = true
  sql         = "SELECT *, topic(2) AS deviceId, timestamp() AS receivedAt FROM 'scapesense/+/events'"
  sql_version = "2016-03-23"

  lambda {
    function_arn = aws_lambda_function.bridge.arn
  }
}

resource "aws_lambda_permission" "iot_telemetry" {
  statement_id  = "AllowIoTTelemetryRule"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.bridge.function_name
  principal     = "iot.amazonaws.com"
  source_arn    = aws_iot_topic_rule.telemetry.arn
}

resource "aws_lambda_permission" "iot_events" {
  statement_id  = "AllowIoTEventsRule"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.bridge.function_name
  principal     = "iot.amazonaws.com"
  source_arn    = aws_iot_topic_rule.events.arn
}
