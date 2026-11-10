output "function_name" {
  value = aws_lambda_function.bridge.function_name
}

output "function_arn" {
  value = aws_lambda_function.bridge.arn
}

output "integration_secret_arn" {
  description = "Populate this secret with the Firebase integration credential during Phase 3."
  value       = aws_secretsmanager_secret.firebase_integration.arn
}
